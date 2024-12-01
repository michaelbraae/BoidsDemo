#pragma once

#include "BoidsDemo/NPC/Components/TrafficEntity.h"

class FTrafficTraversalWorker : public FRunnable
{
public:
	FTrafficTraversalWorker();
	virtual ~FTrafficTraversalWorker() override;
	
	void EnqueueWork(UTrafficEntity* TrafficEntity);
	
	void Start();
	
	virtual void Stop() override;
	
	void StopTask();
	
protected:
	virtual uint32 Run() override;
	
private:
	TArray<UTrafficEntity*> WorkQueue;
	
	TArray<FVector> ResultQueue;
	
	FCriticalSection QueueCriticalSection;
	
	FRunnableThread* Thread;
	
	FThreadSafeBool bIsRunning;
	
	static FVector ComputeMovementVector(UTrafficEntity* TrafficEntity, FRotator& CalculatedRotation, FVector& LowLodVector);
	
	static FVector HandleHighLODTrafficBehaviour(
		const FVector& NextRoadLocation,
		const FVector& PawnLocation,
		const USteeringBehaviour* SteeringBehaviour,
		UFloatingPawnMovement* FloatingPawnMovement
	);
	
	static FVector HandleMediumLODTrafficBehaviour(const FVector& NextRoadLocation, const FVector& PawnLocation, const APawn* OwningPawn);
	
	static FVector HandleLowLODTrafficBehaviour(const FVector& NextRoadLocation, const FVector& PawnLocation);
	
	static FRotator RotateTrafficEntityTowardsVelocity(const FVector& PawnLocation, const APawn* OwningPawn);
};
