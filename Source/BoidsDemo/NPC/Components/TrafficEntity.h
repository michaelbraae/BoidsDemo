#pragma once

#include "LODSetter.h"
#include "SteeringBehaviour.h"
// #include "BehaviorTree/BehaviorTree.h"
#include "AIController.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
// #include "SpaceCowboy/NPC/TrafficManager.h"

#include "TrafficEntity.generated.h"

class ATrafficManager;

UCLASS(ClassGroup=(NPC), meta=(BlueprintSpawnableComponent))
class UTrafficEntity : public  UActorComponent
{
	GENERATED_BODY()
public:
	
	UTrafficEntity();
	virtual void BeginPlay() override;
	
	UPROPERTY()
	APawn* OwningPawn;
	
	UPROPERTY()
	AAIController* AIController;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Data", meta = (ExposeOnSpawn = true))
	bool bShouldRegisterSelf = false;
	
	UFUNCTION()
	void OnLODChanged(int NewLODLevel);
	
	UPROPERTY()
	UFloatingPawnMovement* FloatingPawnMovement;
	
	UPROPERTY()
	FVector TargetInputVector;
	
	UPROPERTY()
	FVector LowLodVector;
	
	UPROPERTY()
	FRotator TargetRotation;
	
	UPROPERTY()
	float MinDistance;

	bool bForceLowLOD = false;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Data", meta = (ExposeOnSpawn = true))
	FVector TargetLocation;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traffic Data", meta = (ExposeOnSpawn = true))
	TArray<FVector> RoadLocations;
	
	int32 CurrentIndex;
	
	FVector NextRoadLocation;
	
	UPROPERTY()
	ULODSetter* LODSetter;
	
	UPROPERTY()
	USteeringBehaviour* SteeringBehaviour;
	
	void CheckDistanceAndUpdateTarget(FVector PawnLocation);
	
	UPROPERTY()
	ATrafficManager* TrafficManager;
	
private:
	
	UPROPERTY()
	int32 RoadNumber = 0;
	
	UPROPERTY()
	APawn* PlayerPawn;
	
	bool bReturnJourney;
};
