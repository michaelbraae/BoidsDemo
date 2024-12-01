#pragma once

#include "RoadMarker.h"
#include "SpaceshipNPC.h"
#include "Async/TrafficTraversalWorker.h"
#include "Components/TrafficEntity.h"
#include "Components/BillboardComponent.h"

#include "TrafficManager.generated.h"

UCLASS(Blueprintable)
class BOIDSDEMO_API ATrafficManager : public AActor
{
	GENERATED_BODY()
public:
	ATrafficManager();
	virtual ~ATrafficManager() override;
	
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere)
    UBillboardComponent* Billboard;
	
private:
	int IdentifyRoads(const FVector& StartingLocation, TArray<FVector>& RoadMarkerLocations, float& MinimumDistance) const;
	
	static void SortRoadMarkersByDistance(const FVector& StartingLocation, TArray<ARoadMarker*>& RoadMarkers, float& MarkerMinDistance);
	
// entity behaviour
public:
	UPROPERTY(EditAnywhere, Category = "Traffic|Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 2000.0f;
	
	UPROPERTY(EditAnywhere, Category = "Traffic|Movement", meta = (ClampMin = "0.0"))
	float MinSpeed = 1000.0f;
	
// Spawning
	UPROPERTY(EditAnywhere, Category = "Traffic|Spawn")
	int32 NumberToSpawn;
	
	UPROPERTY(EditAnywhere, Category = "Traffic|Spawn")
	TSubclassOf<ASpaceshipNPC> TrafficType;
	
	void RegisterTrafficEntity(UTrafficEntity* TrafficEntity);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn))
	bool bForceLowLOD = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road Marker", meta = (ExposeOnSpawn))
	int32 RoadNumber = 0;
	
private:
	int32 SpawnCount = 0;
	
	bool bSpawningTraffic;
	
	TArray<FVector> RoadLocations;
	
	TArray<FVector> SpawnLocations;
	
	float MinDistance;
	
	void SpawnTraffic(int32 ToSpawn);
	
	void DestroyTraffic();
	
	void DestroyWorkers();
	
// Worker
	TArray<TArray<UTrafficEntity*>> TrafficEntityGroups;
	
	TUniquePtr<FTrafficTraversalWorker> RegisterNewTrafficWorker();
	
	const int BatchSize = 100;
	
	TArray<TUniquePtr<FTrafficTraversalWorker>> TrafficWorkers;
	
// Lodding
public:
	UPROPERTY(EditAnywhere, Category = "Traffic|Spawn", meta = (Description = "The distance where the traffic entities will be culled or spawned"))
	float CullDistance;
	
private:
	UPROPERTY()
	ULODSetter* LODSetter;

	UFUNCTION()
	void OnLODChanged(int NewLODLevel);
};
