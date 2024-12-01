//includes
#include "Boid.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "FlockManager.h"
#include "DrawDebugHelpers.h"
#include "BoidsDemo/BoidsDemo.h"

ABoid::ABoid()
{
	//enable actor ticking
	PrimaryActorTick.bCanEverTick = true;
	
	//setup boid collision component and set as root
	BoidCollision = CreateDefaultSubobject<USphereComponent>(TEXT("Boid Collision Component"));
	RootComponent = BoidCollision;
	BoidCollision->SetCollisionObjectType(ECC_Pawn);
	BoidCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoidCollision->SetCollisionResponseToAllChannels(ECR_Overlap);
	
	//setup boid mesh component & attach to root
	// BoidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Boid Mesh Component"));
	// BoidMesh->SetupAttachment(RootComponent);
	// BoidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// BoidMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	
	//setup cohesion sensor component
	PerceptionSensor = CreateDefaultSubobject<USphereComponent>(TEXT("Perception Sensor Component"));
	PerceptionSensor->SetupAttachment(RootComponent);
	PerceptionSensor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PerceptionSensor->SetCollisionResponseToAllChannels(ECR_Ignore);
	PerceptionSensor->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PerceptionSensor->SetSphereRadius(300.0f);
	
	//set default velocity
	BoidVelocity = FVector::ZeroVector;
}

void ABoid::BeginPlay()
{
	Super::BeginPlay();
	
	//check if boid is owned by a flock manager
	AFlockManager* BoidOwner = Cast<AFlockManager>(this->GetOwner());
	if (BoidOwner)
	{
		//set flock manager
		FlockManager = BoidOwner;
		
		//set velocity based on spawn rotation and flock speed settings
		BoidVelocity = this->GetActorForwardVector();
		BoidVelocity.Normalize();
		BoidVelocity *= FMath::FRandRange(FlockManager->GetMinSpeed(), FlockManager->GetMaxSpeed());
		
		//set current mesh rotation
		CurrentRotation = this->GetActorRotation();
	}
}

void ABoid::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	SetActorLocation(GetActorLocation() + (BoidVelocity * DeltaTime));
	
	SetActorRotation(FQuat::Slerp(GetActorRotation().Quaternion(), BoidVelocity.ToOrientationQuat(), 0.1f));
}
