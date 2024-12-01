#include "GetNextTravelLocation.h"

#include "BoidsDemo/NPC/Components/TrafficEntity.h"

EBTNodeResult::Type UGetNextTravelLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
		{
			const TArray<FVector> RoadLocations = AIController->GetPawn()->GetComponentByClass<UTrafficEntity>()->RoadLocations;
			
			// find the next location that is infront of the AI
			FVector BestNextLocation = GetClosestVectorInFrontOfPawn(AIController->GetPawn(), RoadLocations);
			
			BlackboardComp->SetValueAsVector(TargetLocationKey.SelectedKeyName, BestNextLocation);
			
			AIController->GetPawn()->GetComponentByClass<UTrafficEntity>()->TargetLocation = BestNextLocation;
			
			DrawDebugSphere(GetWorld(), BestNextLocation, 100, 12, FColor::Purple, false, 0.0);
			
			return EBTNodeResult::Succeeded;
		}
	}
	return EBTNodeResult::Failed;
}

/**
 * Get the vector from the locations array that is the closest and in front of the pawn
 * @param Pawn 
 * @param Vectors 
 * @return 
 */
FVector UGetNextTravelLocation::GetClosestVectorInFrontOfPawn(const APawn* Pawn, const TArray<FVector>& Vectors)
{
	if (!Pawn || Vectors.Num() == 0)
	{
		return FVector::ZeroVector;
	}
	
	const FVector PawnLocation = Pawn->GetActorLocation();
	const FVector PawnForward = Pawn->GetActorForwardVector();
    
	FVector ClosestVector = FVector::ZeroVector;
	float MinDistance = FLT_MAX;
	
	for (const FVector& Vec : Vectors)
	{
		const FVector DirectionToVec = (Vec - PawnLocation).GetSafeNormal();
		const float DotProduct = FVector::DotProduct(PawnForward, DirectionToVec);

		float Distance = FVector::Dist(PawnLocation, Vec);

		if (Distance <= 100) continue;
		
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			ClosestVector = Vec;
		}
		
		// Check if the vector is in front of the pawn
		if (DotProduct > 0.0f)
		{
			// const float DistanceSquared = FVector::DistSquared(PawnLocation, Vec);
			Distance = FVector::Dist(PawnLocation, Vec);
			if (Distance < MinDistance)
			{
				MinDistance = Distance;
				ClosestVector = Vec;
			}
		}
	}
	
	return ClosestVector;
}