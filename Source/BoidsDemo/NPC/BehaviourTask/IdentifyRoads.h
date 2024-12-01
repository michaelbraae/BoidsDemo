#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"
#include "BoidsDemo/NPC/RoadMarker.h"

#include "IdentifyRoads.generated.h"

UCLASS()
class BOIDSDEMO_API UIdentifyRoads : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UPROPERTY()
	APawn* OwningPawn;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
private:
	void SortVectorsByProximity(TArray<FVector>& Vectors) const;
	void SortMeshBoundsByProximityWithOverlap(TArray<FBoxSphereBounds>& BoxSphereBoundses) const;
	void SortRoadMarkersByDistance(TArray<ARoadMarker*>& RoadMarkers) const;

	static TArray<FVector> IdentifyBranches(TArray<FVector>& SortedVectors);
};