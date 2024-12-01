#include "FlockManager.h"
#include "Components/BillboardComponent.h"
#include "Boid.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BoidsDemo/NPC/Async/Boids/BoidWorker.h"
#include "BoidsDemo/NPC/Async/Boids/BoidWorkerInitialisationData.h"

AFlockManager::AFlockManager()
{
	// disable ticking
	PrimaryActorTick.bCanEverTick = true;
	
	// setup billboard visual component
	FlockManagerBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("FlockManager Billboard Component"));
	RootComponent = FlockManagerBillboard;
	BoundaryCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundaryCollisionComponent"));
	BoundaryCollision->SetupAttachment(RootComponent);
	
	// implement the LOD setter and it's event handler
	LODSetter = CreateDefaultSubobject<ULODSetter>(TEXT("LODSetter"));
	
	// setup boundary cage collision component
	BoundaryCollision->SetBoxExtent(FVector(2500));
	BoundaryCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoundaryCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoundaryCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BoundaryCollision->SetLineThickness(100.0f);
	
	LODSetter->OnLODChanged.AddDynamic(this, &AFlockManager::OnLODChanged);
	
	BuildAvoidanceSensors();
}

AFlockManager::~AFlockManager()
{
	if (BoidWorker)
	{
		BoidWorker->Stop();
	}
}

void AFlockManager::BeginPlay()
{
	Super::BeginPlay();
	
	//bind collision component overlap event to delegate that gets called to perform logic
	if (BoundaryCollision)
	{
		BoundaryCollision->OnComponentEndOverlap.AddDynamic(this, &AFlockManager::OnCageOverlapEnd);
	}
	
	LODSetter->LODLevel = ULODSetter::LowLOD;
	LODSetter->SetComponentTickInterval(1.0f);
	
	LODSetter->HighLodThreshold = CullDistance;
	LODSetter->MediumLodThreshold = CullDistance + 1;

	CreateWorker();
}

void AFlockManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	// if we are trying to spawn boids, and we have BoidsToSpawn, lets create one this tick!
	if (!BoidsToSpawn.IsEmpty() && bSpawningBoids)
	{
		AddBoidToFlock(GetWorld()->SpawnActor<ABoid>(
			BoidsToSpawn[0].BoidType,
			BoidsToSpawn[0].SpawnLocation,
			BoidsToSpawn[0].SpawnRotation,
			BoidsToSpawn[0].SpawnParams
		));
		
		// remove it from the queue
		BoidsToSpawn.RemoveAt(0);
	}
	
	// If bSpawningBoids is false, we are de-spawning
	else if (!BoidsInFlock.IsEmpty() && !bSpawningBoids)
	{
		BoidsInFlock[0]->Destroy();
		BoidsInFlock.RemoveAt(0);
	}
	
	// if true, we've reached the end of the despawn logic, and we can disable the tick for this actor
	if (BoidsInFlock.IsEmpty() && !bSpawningBoids)
	{
		SetActorTickEnabled(false);
		// if (BoidWorker) BoidWorker->Stop();
		return;
	}
	
	// if we made it here, we enqueue the boids into the worker and let the worker handle the calculations
	for (ABoid* Boid : BoidsInFlock)
	{
		BoidWorker->EnqueueWork(Boid);	
	}
}

void AFlockManager::CreateWorker()
{
	FBoidWorkerInitialisationData InitData {
		AvoidanceSensors,
		SensorRadius,
		MinSpeed,
		MaxSpeed,
		SeparationFOV,
		AlignmentFOV,
		CohesionFOV,
		SeparationStrength,
		AlignmentStrength,
		CohesionStrength,
		AvoidanceStrength
	};
	
	BoidWorker = MakeUnique<FBoidWorker>(InitData);
	
	if (!BoidWorker) return;
	
	SetActorTickEnabled(true);
	BoidWorker->Start();
}

/**
 * Called when the dependency, LODSetter emits an FOnLODLevelChanged event
 * We only care about HighLOD, all others, we de-spawn
 * 
 * @param NewLODLevel 
 */
void AFlockManager::OnLODChanged(int NewLODLevel)
{
	if (NewLODLevel == ULODSetter::HighLOD)
	{
		if (bSpawningBoids) return;
		SpawnBoids(NumBoidsToSpawn);
		BoidWorker->Start();
		SetActorTickEnabled(true);
	}
	else
	{
		DestroyBoids();
	}
}

void AFlockManager::AddBoidToFlock(ABoid* Boid)
{
	if (Boid)
	{
		BoidsInFlock.AddUnique(Boid);
	}
}

void AFlockManager::RemoveBoidFromFlock(ABoid* Boid)
{
	if (Boid)
	{
		BoidsInFlock.Remove(Boid);
	}
}

void AFlockManager::DestroyBoids()
{
	bSpawningBoids = false;
}

/**
 * Create evenly distributed vectors around the boid, so it can detect other boids, the player and the environment
 */
void AFlockManager::BuildAvoidanceSensors()
{
	AvoidanceSensors.Empty();
	
	// theta angle of rotation on xy plane around z axis (yaw) around sphere
	float theta;
	// phi angle of rotation (~pitch) around sphere
	float phi;
	
	FVector SensorDirection;
	
	// NumSensors can be set via BP
	// this is not an ideal solution long term, and may be heavy
	for (int32 i = 0; i < NumSensors; ++i)
	{
		// calculate the spherical coordinates of the direction vectors endpoint
		theta = 2 * UKismetMathLibrary::GetPI() * GoldenRatio * i;
		phi = FMath::Acos(1 - (2 * float(i) / NumSensors));
		
		// convert point on sphere to cartesian coordinates xyz
		SensorDirection.X = FMath::Cos(theta)*FMath::Sin(phi);
		SensorDirection.Y = FMath::Sin(theta)*FMath::Sin(phi);
		SensorDirection.Z = FMath::Cos(phi);
		
		AvoidanceSensors.Emplace(SensorDirection);
	}
}

/**
 * Called when a Boid leaves the Boundary
 * If this happens, we just turn em around by inverting their target velocity
 * 
 * @param OverlappedComponent 
 * @param OtherActor 
 * @param OtherComp 
 * @param OtherBodyIndex 
 */
void AFlockManager::OnCageOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//check if actor leaving cage is a valid boid and not being destroyed
	ABoid* EscapingBoid = Cast<ABoid>(OtherActor);
	if (EscapingBoid)
	{
		if (EscapingBoid->IsActorBeingDestroyed()) { return; }
		
		// if a boid leaves the boundary, just turn it around
		EscapingBoid->BoidVelocity = EscapingBoid->BoidVelocity * -1;
	}
}

/**
 * Begin the spawning process
 * We dont spawn them here, but we populate the BoidsToSpawn array
 * This is then handled by spawning one boid per tick, in order to keep the load on any one frame to a minimum
 * 
 * @param NumBoids 
 */
void AFlockManager::SpawnBoids(int32 NumBoids)
{
	//check BoidType is set in BP
	if (BoidType == nullptr) return;
	
	if (!BoidWorker) CreateWorker();
	
	// Create the spawn parameters, and populate the BoidsToSpawn array
	FVector SpawnLocation = FVector::ZeroVector;
	FRotator SpawnRotation = FRotator::ZeroRotator;
	FActorSpawnParameters BoidSpawnParams;
	BoidSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	BoidSpawnParams.Owner = this;
	
	for (int i = 0; i < NumBoids; ++i)
	{
		if (BoidsToSpawn.Num() + BoidsInFlock.Num() >= NumBoids)
		{
			bSpawningBoids = true;
			return;
		}
		
		// Spawn location is random within the boundary collision
		SpawnLocation.X = FMath::FRandRange(-BoundaryCollision->GetScaledBoxExtent().X, BoundaryCollision->GetScaledBoxExtent().X);
		SpawnLocation.Y = FMath::FRandRange(-BoundaryCollision->GetScaledBoxExtent().Y, BoundaryCollision->GetScaledBoxExtent().Y);
		SpawnLocation.Z = FMath::FRandRange(-BoundaryCollision->GetScaledBoxExtent().Z, BoundaryCollision->GetScaledBoxExtent().Z);
		SpawnLocation += this->GetActorLocation();
		
		// spawn rotation should be random except we clamp the pitch so they dont spawn going straight up and down
		float RandomYaw = FMath::FRandRange(0.f, 360.f);
		float RandomPitch = FMath::FRandRange(-45.f, 45.f);
		float RandomRoll = FMath::FRandRange(0.f, 360.f);
		SpawnRotation = FRotator(RandomPitch, RandomYaw, RandomRoll);
		
		FBoidSpawnData SpawnData = {
			BoidType,
			SpawnLocation,
			SpawnRotation,
			BoidSpawnParams
		};
		
		BoidsToSpawn.Add(SpawnData);
	}
	
	// when true, we look for BoidsToSpawn on tick
	bSpawningBoids = true;
}
