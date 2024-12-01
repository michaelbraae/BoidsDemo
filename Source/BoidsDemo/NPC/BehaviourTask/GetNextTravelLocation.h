#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

#include "GetNextTravelLocation.generated.h"

UCLASS()
class BOIDSDEMO_API UGetNextTravelLocation : public UBTTaskNode
{
	GENERATED_BODY()
	
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetLocationKey;
private:
	static FVector GetClosestVectorInFrontOfPawn(const APawn* Pawn, const TArray<FVector>& Vectors);

};