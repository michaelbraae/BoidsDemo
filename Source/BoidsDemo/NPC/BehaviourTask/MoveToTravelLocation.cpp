#include "MoveToTravelLocation.h"

#include "BoidsDemo/NPC/Components/TrafficEntity.h"

class UTrafficEntity;


EBTNodeResult::Type UMoveToTravelLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
		{
			bNotifyTick = 1;
			Pawn = AIController->GetPawn();
			TrafficEntityComponent = Pawn->GetComponentByClass<UTrafficEntity>();
			
			TargetLocation = BlackboardComp->GetValueAsVector(TargetLocationKey.SelectedKeyName);
			// UCurveFloat* FloatCurve = AIController->GetPawn()->GetComponentByClass<UTrafficEntity>()->TraversalCurve;

			float DistanceToTarget = FVector::Dist(Pawn->GetActorLocation(), TargetLocation);
			
			// if (!TargetLocation.IsZero() && TrafficEntityComponent->TraversalTimeline)
			// {
			// 	OwningComponent = &OwnerComp;
			//
			// 	TimelineFinished.BindUFunction(this, FName("OnTimelineFinished"));
			// 	
			// 	TrafficEntityComponent->TraversalTimeline->SetPlayRate(1.0 / DistanceToTarget);
			// 	// TrafficEntityComponent->TraversalTimeline->SetPlayRate(1.0);
			// 	TrafficEntityComponent->TraversalTimeline->SetPlaybackPosition(0.0f, false);
			// 	TrafficEntityComponent->TraversalTimeline->SetTimelineFinishedFunc(TimelineFinished);
			// 	TrafficEntityComponent->TraversalTimeline->PlayFromStart();
			// 	
			// 	return EBTNodeResult::InProgress;
			// }
		}
	}
	return EBTNodeResult::Failed;
}

void UMoveToTravelLocation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	if (FVector::Dist(Pawn->GetActorLocation(), TargetLocation) < 10)
	{
		// TrafficEntityComponent->TraversalTimeline->Stop();
		OnTimelineFinished();
	}
}

void UMoveToTravelLocation::OnTimelineFinished() const
{
	// FinishLatentTask(*OwningComponent, EBTNodeResult::Succeeded);
}
