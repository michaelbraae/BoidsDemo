#include "IdentifyRoads.h"

#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "BoidsDemo/NPC/RoadMarker.h"
#include "BoidsDemo/NPC/Components/TrafficEntity.h"

/**
 * @param OwnerComp 
 * @param NodeMemory 
 * @return 
 */
EBTNodeResult::Type UIdentifyRoads::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	TArray<UStaticMesh*> RoadMeshes;
	
	UWorld* World = GetWorld();
	
	TArray<FVector> Locations;

	TArray<FBoxSphereBounds> LocationBoxes;
	
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		OwningPawn = AIController->GetPawn();
		
		TArray<AActor*> FoundRoadMarkers;
		TArray<ARoadMarker*> RoadMarkers;

		
		UGameplayStatics::GetAllActorsOfClass(World, ARoadMarker::StaticClass(), FoundRoadMarkers);

		for (UObject* Marker : FoundRoadMarkers)
		{
			RoadMarkers.Add(Cast<ARoadMarker>(Marker));
		}
		
		SortRoadMarkersByDistance(RoadMarkers);
		
		for (int32 i = 0; i < RoadMarkers.Num(); ++i)
		{
			FVector MarkerPos = RoadMarkers[i]->GetActorLocation();
			FVector NewLocation = FVector(
				MarkerPos.X + FMath::RandRange(-300, 300),
				MarkerPos.Y + FMath::RandRange(-300, 300),
				MarkerPos.Z + FMath::RandRange(800, 2000)
			); // 1000
			Locations.Add(NewLocation);
			// float t = static_cast<float>(i) / (RoadMarkers.Num() - 1);
			// FLinearColor Color = FMath::Lerp(FLinearColor::Green, FLinearColor::Red, t);
			// DrawDebugString(GetWorld(), NewLocation, FString::FromInt(i), nullptr, FColor::White, 100, false, 3);
			// DrawDebugSphere(GetWorld(), NewLocation, 100, 12, Color.ToFColor(true), true, 1.0);
		}
		
		// get all the "roads" ie: static meshes with the skimmable tag
		// TArray<AActor*> FoundActors;
		// UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), FoundActors);
		// for (const UObject* Mesh : FoundActors)
		// {
		// 	if (const AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Mesh))
		// 	{
		// 		if (MeshActor->GetStaticMeshComponent()->ComponentHasTag("Skimmable"))
		// 		{
		// 			// FVector NewRoadLocation = FVector(MeshActor->GetActorLocation().X, MeshActor->GetActorLocation().Y, MeshActor->GetActorLocation().Z + 1000);
		// 			FVector MeshBoundsOrigin = MeshActor->GetStaticMeshComponent()->Bounds.Origin;
		// 			FVector NewRoadLocation = FVector(MeshBoundsOrigin.X, MeshBoundsOrigin.Y, MeshBoundsOrigin.Z + 1000);
		// 			FBoxSphereBounds MeshBox = MeshActor->GetStaticMeshComponent()->Bounds;
		//
		// 			LocationBoxes.Add(MeshBox);
		// 			Locations.Add(NewRoadLocation);
		// 		}
		// 	}
		// }
		//
		// SortVectorsByProximity(Locations);
		//
		// SortMeshBoundsByProximityWithOverlap(LocationBoxes);
		//
		// for (int32 i = 0; i < LocationBoxes.Num(); ++i)
		// {
		// 	float t = static_cast<float>(i) / (LocationBoxes.Num() - 1);
		// 	FLinearColor Color = FMath::Lerp(FLinearColor::Green, FLinearColor::Red, t);
		// 	DrawDebugString(GetWorld(), LocationBoxes[i].Origin, FString::FromInt(i), nullptr, FColor::White, 100, false, 3);
		// 	DrawDebugSphere(GetWorld(), LocationBoxes[i].Origin, 100, 12, Color.ToFColor(true), true, 1.0);
		// }
		//
		// TArray<FVector> BranchedVectors = IdentifyBranches(Locations);

		// Locations.RemoveAll([&BranchedVectors](const FVector& Vector)
		// {
		// 	return BranchedVectors.Contains(Vector);
		// });

		// for (int32 i = 0; i < BranchedVectors.Num(); ++i)
		// {
		// 	DrawDebugSphere(GetWorld(), FVector(BranchedVectors[i].X, BranchedVectors[i].Y, BranchedVectors[i].Z + 100), 100, 12, FColor::Purple, true, 1.0);
		// }
		//
		// from green to red, we sort the vectors
		// for (int32 i = 0; i < Locations.Num(); ++i)
		// {
		// 	float t = static_cast<float>(i) / (Locations.Num() - 1);
		// 	FLinearColor Color = FMath::Lerp(FLinearColor::Green, FLinearColor::Red, t);
		// 	DrawDebugString(GetWorld(), Locations[i], FString::FromInt(i), nullptr, FColor::White, 100, false, 3);
		// 	DrawDebugSphere(GetWorld(), Locations[i], 100, 12, Color.ToFColor(true), true, 1.0);
		// }
		
		int ClosestVectorIndex = 0;
		float ClosestVectorDistance = FLT_MAX;
		
		for (int i = 0; i < Locations.Num(); i++)
		{
			FVector Location = Locations[i];
			if (FVector::Dist(Location, AIController->GetPawn()->GetActorLocation()) < ClosestVectorDistance)
			{
				ClosestVectorDistance = FVector::Dist(Location, AIController->GetPawn()->GetActorLocation());
				ClosestVectorIndex = i;
			}
		}
		
		AIController->GetPawn()->GetComponentByClass<UTrafficEntity>()->RoadLocations = Locations;
		AIController->GetPawn()->GetComponentByClass<UTrafficEntity>()->CurrentIndex = ClosestVectorIndex;
		
		if (UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent())
		{
			BlackboardComp->SetValueAsBool(FName("HasIdentifiedRoads"), true);
			return EBTNodeResult::Succeeded;
		}
	}
	
	return EBTNodeResult::Failed;
}

/**
 *	Sort the given vectors array from the spawn point of the AI
 *	Identify and branches and decide if we want to take them or not
 * 
 * @param Vectors 
 */
void UIdentifyRoads::SortVectorsByProximity(TArray<FVector>& Vectors) const
{
	if (Vectors.Num() <= 1)
	{
		return;
	}

	// Result array to store sorted vectors
	TArray<FVector> SortedVectors;
	
	// Start with the vector closest to the actor
	FVector Origin = OwningPawn->GetActorLocation();

	// if the difference in distance between A -> B and A -> C is less than this amount, we can assume it's a branch
	float BranchDistanceThreshold = FMath::Square(4000.0f);
	
	// Find the vector closest to the origin
	int32 ClosestIndex = 0;
	int32 SecondClosestIndex = 0;
	float MinDistSquared = FLT_MAX;

	for (int32 i = 1; i < Vectors.Num(); ++i)
	{
		float DistSquared = FVector::DistSquared(Origin, Vectors[i]);
		if (DistSquared < MinDistSquared)
		{
			MinDistSquared = DistSquared;
			ClosestIndex = i;
		}
	}
	
	SortedVectors.Add(Vectors[ClosestIndex]);
	Vectors.RemoveAt(ClosestIndex);
	
	while (Vectors.Num() > 0)
	{
		FVector LastSortedVector = SortedVectors.Last();
		
		ClosestIndex = 0;
		SecondClosestIndex = 0;
		
		MinDistSquared = FVector::DistSquared(LastSortedVector, Vectors[0]);
		float SecondMinDistSquared = FLT_MAX;
		
		// find the next closest vector
		// ensure the expanded box overlaps with current
		for (int32 i = 1; i < Vectors.Num(); ++i)
		{
			float DistSquared = FVector::DistSquared(LastSortedVector, Vectors[i]);
			if (DistSquared < MinDistSquared)
			{
				MinDistSquared = DistSquared;
				ClosestIndex = i;
			}
		}
		
		// find the second closest vector
		for (int32 i = 1; i < Vectors.Num(); ++i)
		{
			float DistSquared = FVector::DistSquared(LastSortedVector, Vectors[i]);
			if (i != ClosestIndex && DistSquared < SecondMinDistSquared)
			{
				SecondMinDistSquared = DistSquared;
				SecondClosestIndex = i;
			}
		}

		// We determine a branch if the distance between the branching paths and the current one
		// is below a threshold
		// if (FMath::Abs(MinDistSquared - SecondMinDistSquared) < BranchDistanceThreshold)
		if (false)
		{
			// we randomly decide to take the branching path or stay on the main track
			const bool bUseMainPath = FMath::RandBool();
			
			const int32 SelectedPathIndex = bUseMainPath ? ClosestIndex : SecondClosestIndex;
			SortedVectors.Add(Vectors[SelectedPathIndex]);

			// we need to adjust the indexes if we remove an element that is before one
			// we are about to read from
			if (ClosestIndex < SecondClosestIndex)
			{
				Vectors.RemoveAt(SecondClosestIndex);	
				Vectors.RemoveAt(ClosestIndex);	
			}
			else
			{
				Vectors.RemoveAt(ClosestIndex);	
				Vectors.RemoveAt(SecondClosestIndex);	
			}
		}
		else
		{
			// if we aren't at a branch, just continue as normal
			SortedVectors.Add(Vectors[ClosestIndex]);
			Vectors.RemoveAt(ClosestIndex);	
		}
	}
	
	Vectors = MoveTemp(SortedVectors);
}

void UIdentifyRoads::SortMeshBoundsByProximityWithOverlap(TArray<FBoxSphereBounds>& BoxSphereBoundses) const
{
	if (BoxSphereBoundses.Num() <= 1)
	{
		return;
	}

	// Result array to store sorted vectors
	TArray<FBoxSphereBounds> SortedBoundses;
	
	// Start with the vector closest to the actor
	FVector Origin = OwningPawn->GetActorLocation();

	// Find the vector closest to the origin
	int32 ClosestIndex = 0;
	float MinDistSquared = FLT_MAX;

	for (int32 i = 1; i < BoxSphereBoundses.Num(); ++i)
	{
		float DistSquared = FVector::DistSquared(Origin, BoxSphereBoundses[i].Origin);
		if (DistSquared < MinDistSquared)
		{
			MinDistSquared = DistSquared;
			ClosestIndex = i;
		}
	}

	// We've found the first one, yay!
	SortedBoundses.Add(BoxSphereBoundses[ClosestIndex]);
	BoxSphereBoundses.RemoveAt(ClosestIndex);
	
	while (BoxSphereBoundses.Num() > 0)
	{
		FVector LastSortedVector = SortedBoundses.Last().Origin;

		// set the previous bounds
		FBox LastSortedBox = SortedBoundses.Last().GetBox();
		LastSortedBox = LastSortedBox.ExpandBy(100);
		
		ClosestIndex = 0;
		
		MinDistSquared = FVector::DistSquared(LastSortedVector, BoxSphereBoundses[0].Origin);
		
		// find the next closest vector
		// ensure the expanded box overlaps with current
		for (int32 i = 1; i < BoxSphereBoundses.Num(); ++i)
		{
			float DistSquared = FVector::DistSquared(LastSortedVector, BoxSphereBoundses[i].Origin);
			FBox CurrentBox = BoxSphereBoundses[i].GetBox();
			CurrentBox = CurrentBox.ExpandBy(100);

			// check the distance is the closest and it overlaps with the last sorted box
			if (DistSquared < MinDistSquared && CurrentBox.Intersect(LastSortedBox))
			{
				MinDistSquared = DistSquared;
				ClosestIndex = i;
			}
		}
		
		SortedBoundses.Add(BoxSphereBoundses[ClosestIndex]);
		BoxSphereBoundses.RemoveAt(ClosestIndex);
	}
	
	BoxSphereBoundses = MoveTemp(SortedBoundses);
}

void UIdentifyRoads::SortRoadMarkersByDistance(TArray<ARoadMarker*>& RoadMarkers) const
{
	if (RoadMarkers.Num() <= 1)
	{
		return;
	}
	
	// Result array to store sorted vectors
	TArray<ARoadMarker*> SortedRoadMarkers;
	
	// Start with the vector closest to the actor
	FVector Origin = OwningPawn->GetActorLocation();
	
	// Find the vector closest to the origin
	int32 ClosestIndex = 0;
	int32 SecondClosestIndex = 0;
	float MinDistSquared = FLT_MAX;
	
	for (int32 i = 1; i < RoadMarkers.Num(); ++i)
	{
		float DistSquared = FVector::DistSquared(Origin, RoadMarkers[i]->GetActorLocation());
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
	
	RoadMarkers = MoveTemp(SortedRoadMarkers);
}

/**
 *	Identify all of the sorted vectors that deviate too far from the main pathway, they must be branching paths
 * 
 * @param SortedVectors 
 * @return 
 */
TArray<FVector> UIdentifyRoads::IdentifyBranches(TArray<FVector>& SortedVectors)
{
	float BranchingThreshold = 8000;
	
	TArray<FVector> BranchingVectors;

	bool bInBranch = false;
	for (int i = 1; i < SortedVectors.Num(); i++)
	{
		float Distance = FVector::Dist(SortedVectors[i - 1], SortedVectors[i]);

		if (Distance > BranchingThreshold)
		{
			BranchingVectors.Add(SortedVectors[i]);
			bInBranch = true;
		}
		else if (bInBranch)
		{
			BranchingVectors.Add(SortedVectors[i]);
		}
	}

	return BranchingVectors;
}
