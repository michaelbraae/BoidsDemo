#include "TrafficManager.h"

#include "RoadMarker.h"

ATrafficManager::ATrafficManager()
{
	PrimaryActorTick.bCanEverTick = true;
	
	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("FlockManager Billboard Component"));
	RootComponent = Billboard;
	
	// set up the lod handling
	LODSetter = CreateDefaultSubobject<ULODSetter>(TEXT("LODSetter"));
	
	LODSetter->OnLODChanged.AddDynamic(this, &ATrafficManager::OnLODChanged);
}

ATrafficManager::~ATrafficManager()
{
	for (int i = 0; i < TrafficWorkers.Num(); i++)
	{
		if (FTrafficTraversalWorker* Worker = TrafficWorkers[i].Get())
		{
			Worker->Stop();
		}
	}
}

void ATrafficManager::BeginPlay()
{
	Super::BeginPlay();
	
	// the manager needs the road locations so it can spawn entities along the road
	IdentifyRoads(GetActorLocation(), RoadLocations, MinDistance);
	
	LODSetter->LODLevel = ULODSetter::LowLOD;
	LODSetter->HighLodThreshold = CullDistance;
	LODSetter->MediumLodThreshold = CullDistance + 1;
	LODSetter->SetComponentTickInterval(1.0f);
	
	// we dont wanna tick at first, until we hit a lod level which needs it
	SetActorTickEnabled(false);
}

TUniquePtr<FTrafficTraversalWorker> ATrafficManager::RegisterNewTrafficWorker()
{
	TUniquePtr<FTrafficTraversalWorker> NewWorker = MakeUnique<FTrafficTraversalWorker>();
	
	NewWorker->Start();
	TrafficWorkers.Add(MoveTemp(NewWorker));
	
	return NewWorker;
}

void ATrafficManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	for (int i = 0; i < TrafficWorkers.Num(); i++)
	{
		if (FTrafficTraversalWorker* Worker = TrafficWorkers[i].Get())
		{
			Worker->Stop();
		}
	}
}

void ATrafficManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// we spawn the traffic entities once per tick
	// instead of all at once
	if (!SpawnLocations.IsEmpty() && bSpawningTraffic)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		SpawnParams.Owner = this;
		
		ASpaceshipNPC* NewShip = GetWorld()->SpawnActor<ASpaceshipNPC>(
			TrafficType,
			SpawnLocations[0],
			FRotator::ZeroRotator,
			SpawnParams
		);
		
		UTrafficEntity* NewTrafficEntity = NewShip->GetComponentByClass<UTrafficEntity>();
		
		NewTrafficEntity->TrafficManager = this;
		NewTrafficEntity->bForceLowLOD = bForceLowLOD;
		
		SpawnLocations.RemoveAt(0);
		SpawnCount++;
		
		RegisterTrafficEntity(NewTrafficEntity);
	}
	// we despawn once per tick too
	else if (!TrafficEntityGroups.IsEmpty() && !bSpawningTraffic)
	{
		TrafficEntityGroups.Last().Last()->GetOwner()->Destroy();
		TrafficEntityGroups.Last().RemoveAt(TrafficEntityGroups.Last().Num() - 1);
		
		SpawnCount--;
		
		if (TrafficEntityGroups.Last().IsEmpty())
		{
			TrafficEntityGroups.RemoveAt(TrafficEntityGroups.Num() - 1);
		}
	}

	// if we are de-spawning, but we've reached the end
	// we pause ticking, reset and destroy the workers
	if (TrafficEntityGroups.IsEmpty() && !bSpawningTraffic)
	{
		SpawnCount = 0;
		// if (!TrafficWorkers.IsEmpty())
		// {
			
			// TrafficWorkers.RemoveAt(0);
		// }
		// else
		// {
		SetActorTickEnabled(false);
		// }
		// DestroyWorkers();
		return;
	}
	
	// if we made it this far, we enqueue entities in the workers
	for (int i = 0; i < TrafficEntityGroups.Num(); i++)
	{
		for (UTrafficEntity* Entity : TrafficEntityGroups[i])
		{
			TrafficWorkers[i]->EnqueueWork(Entity);
		}
	}
}

/**
 * Begin the spawning process
 * We define a number of spawn locations at even points along the road
 * and set bSpawningTraffic to true, to allow the spawn to happen once per tick
 * 
 * @param ToSpawn 
 */
void ATrafficManager::SpawnTraffic(int32 ToSpawn)
{
	if (!TrafficType || RoadLocations.IsEmpty()) return;
	
	bSpawningTraffic = true;
	int32 SpawnCountPerLocation = ToSpawn / RoadLocations.Num();
	
	for (int i = 0; i < RoadLocations.Num(); i++)
	{
		for (int j = 0; j < SpawnCountPerLocation; j++)
		{
			// dont spawn more than the limit
			if (SpawnLocations.Num() + SpawnCount >= ToSpawn)
			{
				SetActorTickEnabled(true);
				return;
			}
			
			FVector SpawnLocation = RoadLocations[i];
			
			// spread the spawn locations out between the road markers
			if (RoadLocations.IsValidIndex(i + 1))
			{
				float SpawnAlpha = static_cast<float>(j) / static_cast<float>(SpawnCountPerLocation);
				SpawnLocation = FMath::Lerp(RoadLocations[i], RoadLocations[i + 1], SpawnAlpha);
			}
			
			SpawnLocations.Add(SpawnLocation);
		}
	}
	
	SetActorTickEnabled(true);
}

void ATrafficManager::DestroyTraffic()
{
	bSpawningTraffic = false;
}

void ATrafficManager::DestroyWorkers()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Destroy TrafficTraversalWorkers!"));
	// empty calls each workers destructor so we wont have any floating threads

	// while (TrafficWorkers.Num() > 0)
	// {
		// TUniquePtr<FTrafficTraversalWorker> TrafficWorker = TrafficWorkers[0];
	// }
	// Async(EAsyncExecution::Thread, []()
	// {
		// while (Traff)		
		// delete Runnable;  // Deallocate on another thread
	// });
		
	TrafficWorkers.Empty();
	
}

/**
 * Adds a traffic entity to our worker
 * spawns a new worker if the new entity would exceed the worker Batch Size
 * 
 * @param Entity 
 */
void ATrafficManager::RegisterTrafficEntity(UTrafficEntity* Entity)
{
	if (Entity)
	{
		float MinimumDistance = 0;
		IdentifyRoads(Entity->GetOwner()->GetActorLocation(), Entity->RoadLocations, MinimumDistance);
		Entity->MinDistance = MinimumDistance;
		Entity->NextRoadLocation = Entity->RoadLocations[Entity->CurrentIndex];
		Entity->FloatingPawnMovement->MaxSpeed = FMath::RandRange(MinSpeed, MaxSpeed);
		
		// if this is the first entity, we need to create a new batch, and a new worker
		if (TrafficEntityGroups.Num() < 1)
		{
			TArray<UTrafficEntity*> FirstGroup;
			FirstGroup.Add(Entity);
			TrafficEntityGroups.Add(FirstGroup);
			
			if (TrafficWorkers.IsEmpty())
			{
				RegisterNewTrafficWorker();
			}
			
			return;
		}
		
		// find the first group with a free slot and add the entity to it
		for (TArray<UTrafficEntity*>& EntityGroup : TrafficEntityGroups)
		{
			if (EntityGroup.Num() < BatchSize)
			{
				EntityGroup.Add(Entity);
				return;
			}
		}
		
		// if we made it this far, we need to create a new batch
		TArray<UTrafficEntity*> NewGroup;
		NewGroup.Add(Entity);
		TrafficEntityGroups.Add(NewGroup);
		
		// also create a new worker for the new batch
		RegisterNewTrafficWorker();
	}
}

/**
 * Sets the ref params for a given entity, this is also called by the manager.
 * The ref RoadMarkerLocations is then used by the entities to navigate
 * 
 * @param StartingLocation 
 * @param RoadMarkerLocations 
 * @param MinimumDistance 
 * @return 
 */
int ATrafficManager::IdentifyRoads(const FVector& StartingLocation, TArray<FVector>& RoadMarkerLocations, float& MinimumDistance) const
{
	TArray<UStaticMesh*> RoadMeshes;
	
	UWorld* World = GetWorld();
	
	TArray<FVector> Locations;
	
	TArray<FBoxSphereBounds> LocationBoxes;
	TArray<AActor*> FoundRoadMarkers;
	TArray<ARoadMarker*> RoadMarkers;
	
	UGameplayStatics::GetAllActorsOfClass(World, ARoadMarker::StaticClass(), FoundRoadMarkers);
	
	for (UObject* Marker : FoundRoadMarkers)
	{
		if (ARoadMarker* RoadMarker = Cast<ARoadMarker>(Marker))
		{
			if (RoadMarker->RoadNumber == RoadNumber)
			{
				RoadMarkers.Add(RoadMarker);
			}
		}
	}
	
	float MarkerMinDistance = 0;
	SortRoadMarkersByDistance(StartingLocation, RoadMarkers, MarkerMinDistance);
	
	for (int32 i = 0; i < RoadMarkers.Num(); ++i)
	{
		FVector MarkerPos = RoadMarkers[i]->GetActorLocation();
		FVector NewLocation = FVector(
			MarkerPos.X + FMath::RandRange(-1000, 1000),
			MarkerPos.Y + FMath::RandRange(-1000, 1000),
			MarkerPos.Z + FMath::RandRange(800, 2000)
		); // 1000
		Locations.Add(NewLocation);
	}
	
	int ClosestVectorIndex = 0;
	float ClosestVectorDistance = FLT_MAX;
	
	for (int i = 0; i < Locations.Num(); i++)
	{
		FVector Location = Locations[i];
		if (FVector::Dist(Location, StartingLocation) < ClosestVectorDistance)
		{
			ClosestVectorDistance = FVector::Dist(Location, StartingLocation);
			ClosestVectorIndex = i;
		}
	}
	
	RoadMarkerLocations = MoveTemp(Locations);
	MinimumDistance = MarkerMinDistance;
	return ClosestVectorIndex;
}

/**
 * Sort the road markers one after the other in a linear way based on the next closest one
 * Starting location is used here to initiate the sorting
 * ie: the first road marker in the RoadMarkers array after sorting will be the closest one to the StartingLocation
 * 
 * @param StartingLocation 
 * @param RoadMarkers 
 * @param MarkerMinDistance 
 */
void ATrafficManager::SortRoadMarkersByDistance(const FVector& StartingLocation, TArray<ARoadMarker*>& RoadMarkers, float& MarkerMinDistance)
{
	if (RoadMarkers.Num() <= 1)
	{
		return;
	}
	
	// Result array to store sorted vectors
	TArray<ARoadMarker*> SortedRoadMarkers;
	
	// Find the vector closest to the origin
	int32 ClosestIndex = 0;
	int32 SecondClosestIndex = 0;
	float MinDistSquared = FLT_MAX;
	
	for (int32 i = 1; i < RoadMarkers.Num(); ++i)
	{
		float DistSquared = FVector::DistSquared(StartingLocation, RoadMarkers[i]->GetActorLocation());
		if (DistSquared < MinDistSquared)
		{
			MinDistSquared = DistSquared;
			ClosestIndex = i;
		}
	}
	
	SortedRoadMarkers.Add(RoadMarkers[ClosestIndex]);
	RoadMarkers.RemoveAt(ClosestIndex);
	
	while (RoadMarkers.Num() > 0)
	{
		ARoadMarker* LastSortedRoadMarker = SortedRoadMarkers.Last();
		
		ClosestIndex = 0;
		SecondClosestIndex = 0;
		
		MinDistSquared = FVector::DistSquared(LastSortedRoadMarker->GetActorLocation(), RoadMarkers[0]->GetActorLocation());
		float SecondMinDistSquared = FLT_MAX;
		
		// find the next closest vector
		// ensure the expanded box overlaps with current
		for (int32 i = 1; i < RoadMarkers.Num(); ++i)
		{
			float DistSquared = FVector::DistSquared(LastSortedRoadMarker->GetActorLocation(), RoadMarkers[i]->GetActorLocation());
			if (DistSquared < MinDistSquared)
			{
				MinDistSquared = DistSquared;
				ClosestIndex = i;
			}
		}
		
		// Identify if a branch is occuring, bIsBranch is set on the RoadMarkers themselves when placed in the game world
		if (RoadMarkers[ClosestIndex]->bIsBranch)
		{
			// find the second closest vector
			for (int32 i = 1; i < RoadMarkers.Num(); ++i)
			{
				float DistSquared = FVector::DistSquared(LastSortedRoadMarker->GetActorLocation(), RoadMarkers[i]->GetActorLocation());
				if (i != ClosestIndex && DistSquared < SecondMinDistSquared)
				{
					SecondMinDistSquared = DistSquared;
					SecondClosestIndex = i;
				}
			}
			const bool bUseMainPath = FMath::RandBool();
			
			const int32 SelectedPathIndex = bUseMainPath ? ClosestIndex : SecondClosestIndex;
			
			SortedRoadMarkers.Add(RoadMarkers[SelectedPathIndex]);
			
			// we need to adjust the indexes if we remove an element that is before one
			// we are about to read from
			if (ClosestIndex < SecondClosestIndex)
			{
				RoadMarkers.RemoveAt(SecondClosestIndex);	
				RoadMarkers.RemoveAt(ClosestIndex);	
			}
			else
			{
				RoadMarkers.RemoveAt(ClosestIndex);	
				RoadMarkers.RemoveAt(SecondClosestIndex - 1);	
			}
		}
		else
		{
			// if we aren't at a branch, just continue as normal
			SortedRoadMarkers.Add(RoadMarkers[ClosestIndex]);
			RoadMarkers.RemoveAt(ClosestIndex);	
		}
	}
	
	MarkerMinDistance = FMath::Sqrt(MinDistSquared);
	RoadMarkers = MoveTemp(SortedRoadMarkers);
}

/**
 * Called when the dependency, LODSetter emits an FOnLODLevelChanged event
 * We only care about the HighLOD level (0), anything else we despawn all the entities and tidy up the workers
 * 
 * @param NewLODLevel 
 */
void ATrafficManager::OnLODChanged(int NewLODLevel)
{
	if (NewLODLevel == ULODSetter::HighLOD)
	{
		if (bSpawningTraffic) return;
		SpawnTraffic(NumberToSpawn);
	}
	else
	{
		DestroyTraffic();
	}
}
