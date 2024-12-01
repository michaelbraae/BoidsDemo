#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/TimelineComponent.h"
#include "BoidsDemo/NPC/Components/TrafficEntity.h"

#include "MoveToTravelLocation.generated.h"

UCLASS()
class BOIDSDEMO_API UMoveToTravelLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
	FOnTimelineEvent TimelineFinished{};
	
	UFUNCTION()
	void OnTimelineFinished() const;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetLocationKey;

private:
	UPROPERTY()
	APawn* Pawn;
	
	UPROPERTY()
	UBehaviorTreeComponent* OwningComponent;

	UPROPERTY()
	UTrafficEntity* TrafficEntityComponent;
	
	FVector TargetLocation;
};