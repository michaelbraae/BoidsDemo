#include "TrafficTraversalWorker.h"

#include "Kismet/KismetMathLibrary.h"

FTrafficTraversalWorker::FTrafficTraversalWorker()
	: Thread(nullptr)
	, bIsRunning(false)
{
}

FTrafficTraversalWorker::~FTrafficTraversalWorker()
{
	StopTask();
	// Stop();
	// if (Thread)
	// {
	// 	delete Thread;
	// 	Thread = nullptr;
	// }
}

void FTrafficTraversalWorker::Start()
{
	// get the RoadLocations here too
	bIsRunning = true;
	Thread = FRunnableThread::Create(this, TEXT("TrafficWorkerThread"), 0, TPri_BelowNormal);
}

void FTrafficTraversalWorker::Stop()
{
	if (bIsRunning)
	{
		bIsRunning = false;
		// Optionally, add a brief sleep to give the thread time to check the flag and exit
		FPlatformProcess::Sleep(0.1f);
		
		if (Thread)
		{
			Thread->WaitForCompletion();
		}
	}
}

void FTrafficTraversalWorker::EnqueueWork(UTrafficEntity* Entity)
{
	// Lock access to the queue to prevent race conditions
	FScopeLock Lock(&QueueCriticalSection);
	WorkQueue.Add(Entity);
}

uint32 FTrafficTraversalWorker::Run()
{
	while (bIsRunning)
	{
		// Lock access to the queue
		TArray<UTrafficEntity*> LocalWorkQueue;
		{
			FScopeLock Lock(&QueueCriticalSection);
			LocalWorkQueue = WorkQueue;
			WorkQueue.Empty();
			
			// Process each NPC in the local queue
			for (UTrafficEntity* Entity : LocalWorkQueue)
			{
				if (Entity && Entity->OwningPawn)
				{
					FRotator CalculatedRotation;
					FVector LowLodVector;
					FVector CalculatedVector = ComputeMovementVector(Entity, CalculatedRotation, LowLodVector);
					
					Entity->TargetInputVector = CalculatedVector;
					Entity->TargetRotation = CalculatedRotation;
					Entity->LowLodVector = LowLodVector;
					
					if (!Entity->IsComponentTickEnabled())
					{
						// Defer tick enabling to game thread
						AsyncTask(ENamedThreads::GameThread, [Entity]()
						{
							Entity->SetComponentTickEnabled(true);
						});
					}
				}
			}
		}
		
		// Sleep for a short duration to yield CPU
		FPlatformProcess::Sleep(0.1f);
	}
	
	return 0;
}

void FTrafficTraversalWorker::StopTask()
{
	if (Thread)
	{
		Stop();
		delete Thread;
		Thread = nullptr;
	}
}

FVector FTrafficTraversalWorker::ComputeMovementVector(UTrafficEntity* TrafficEntity, FRotator& CalculatedRotation, FVector& LowLodVector)
{
	TArray<FVector> RoadLocations = TrafficEntity->RoadLocations;
	
	if (!RoadLocations.IsValidIndex(TrafficEntity->CurrentIndex) || RoadLocations.Num() < 2)
	{
		return FVector::Zero();
	}
	
	APawn* OwningPawn = TrafficEntity->OwningPawn;
	FVector NextRoadLocation = TrafficEntity->NextRoadLocation;
	FVector PawnLocation = OwningPawn->GetActorLocation();
	
	// get the current LODLevel from the LODSetter component
	const int LODLevel = TrafficEntity->LODSetter->LODLevel;
	
	// if we aren't rendering the pawn or we're at the lowest LOD level;
	// we use a different method for traversal which doesn't rely on FloatingPawnMovement or SteeringBehaviour
	if (LODLevel == ULODSetter::LowLOD || TrafficEntity->bForceLowLOD)
	{
		LowLodVector = HandleLowLODTrafficBehaviour(NextRoadLocation, PawnLocation);
		return FVector::Zero();
	}
	
	// if we made it past low lod, we wanna be rotating towards velocity
	CalculatedRotation = RotateTrafficEntityTowardsVelocity(PawnLocation, OwningPawn);
	
	// we're at medium LOD, we want to rotate the mesh, but we dont care about steering
	if (LODLevel == ULODSetter::MediumLOD)
	{
		return HandleMediumLODTrafficBehaviour(NextRoadLocation, PawnLocation, OwningPawn);
	}
	
	UFloatingPawnMovement* FloatingPawnMovement = TrafficEntity->FloatingPawnMovement;
	USteeringBehaviour* SteeringBehaviour = TrafficEntity->SteeringBehaviour;
	
	return HandleHighLODTrafficBehaviour(NextRoadLocation, PawnLocation, SteeringBehaviour, FloatingPawnMovement);
}

/**
 * The highest level of detail for the traffic behaviours, should only be used at the highest lod level
 * Handles steering behaviour to ensure entities navigate around each other nicely
 */
FVector FTrafficTraversalWorker::HandleHighLODTrafficBehaviour(const FVector& NextRoadLocation, const FVector& PawnLocation, const USteeringBehaviour* SteeringBehaviour, UFloatingPawnMovement* FloatingPawnMovement)
{
	FVector DesiredDirection = (NextRoadLocation - PawnLocation).GetSafeNormal();
	FVector SteeringForce = DesiredDirection;
	
	// If there's an interest direction from SteeringBehaviour, lets lerp to it
	// it's more efficient to use a boolean here than to compare the InterestDirection to FVector::Zero()
	if (SteeringBehaviour->bHasInterestDirection)
	{
		FVector InterestDirection = SteeringBehaviour->InterestDirection;
		FVector ZAvoidance = FMath::Lerp(FVector(0, 0, PawnLocation.Z), FVector(0, 0, InterestDirection.Z), 0.3f);
		FloatingPawnMovement->AddInputVector(ZAvoidance.GetSafeNormal());
		
		FVector AvoidanceDirection = InterestDirection.GetSafeNormal();
		SteeringForce = FMath::Lerp(DesiredDirection, AvoidanceDirection, 0.7f); // 0.4 was aight
	}
	
	return SteeringForce;
}

/**
 * Medium level of detail for the traffic behaviour, similar to the highest, but with no consideration for steering behaviour
 */
FVector FTrafficTraversalWorker::HandleMediumLODTrafficBehaviour(const FVector& NextRoadLocation, const FVector& PawnLocation, const APawn* OwningPawn)
{
	FVector DesiredDirection = (NextRoadLocation - PawnLocation).GetSafeNormal();
	
	return DesiredDirection;
}

/**
 * Used when the entity is not being rendered or we are at the lowest LOD
 * A more performant way to move along the route which doesn't use FloatingPawnMovement or SteeringBehaviour
 */
FVector FTrafficTraversalWorker::HandleLowLODTrafficBehaviour(const FVector& NextRoadLocation, const FVector& PawnLocation)
{
	// the closer we are, the have a slightly smoother transition
	float alpha = 0.03;
	if (FVector::Dist(PawnLocation, NextRoadLocation) < 5000) alpha = 0.07;
	
	FVector NewLocation = FMath::Lerp(PawnLocation, NextRoadLocation, alpha);
	
	return NewLocation;
}

FRotator FTrafficTraversalWorker::RotateTrafficEntityTowardsVelocity(const FVector& PawnLocation, const APawn* OwningPawn)
{
	// Rotate the pawn to face it's velocity, we do it at a constant rate here because we dont care when in medium
	FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(PawnLocation, OwningPawn->GetVelocity() + PawnLocation);
	return UKismetMathLibrary::RLerp(OwningPawn->GetActorRotation(), TargetRotation, 0.1f, true);
}