#pragma once

#include "SteeringBehaviour.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"

#include "LODSetter.generated.h"

/** Signature of function to handle a timeline 'event' */
DECLARE_DYNAMIC_DELEGATE( FOnLODChanged );

UCLASS(ClassGroup=(NPC), meta=(BlueprintSpawnableComponent))
class ULODSetter : public UActorComponent
{
	GENERATED_BODY()
public:
	ULODSetter();
	// 0 is highest, 2 is lowest
	UPROPERTY()
	int LODLevel = HighLOD;
	
	// define our lod levels
	static constexpr int HighLOD = 0;
	static constexpr int MediumLOD = 1;
	static constexpr int LowLOD = 2;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn, Description = "if true, this LODSetter will go to low LOD if the owning actor was not recently rendered"))
	bool bListenForRenderChanges = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HighLodThreshold = 20000;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MediumLodThreshold = 40000;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLODLevelChanged, int, NewLODLevel);
	
	UPROPERTY(BlueprintAssignable)
	FOnLODLevelChanged OnLODChanged;
private:
	UPROPERTY()
	APawn* PlayerPawn;
	
	UPROPERTY()
	AActor* OwningActor;
	
	UPROPERTY()
	UFloatingPawnMovement* FloatingPawnMovement;
	
	UPROPERTY()
	USteeringBehaviour* SteeringBehaviour;
	
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void UpdateLODLevel(const float DistanceToPlayer);
};
