#pragma once
#include "BoidWorkerInitialisationData.h"
#include "BoidsDemo/NPC/Boids/Boid.h"

class FBoidWorker : public FRunnable
{
public:
	FBoidWorker(FBoidWorkerInitialisationData Data);
	virtual ~FBoidWorker() override;
	
	void EnqueueWork(ABoid* Boid);
	
	virtual uint32 Run() override;
	
	void Start();
	
	virtual void Stop() override;
	
private:
// boid behaviour parameters
	TArray<FVector> AvoidanceSensors;
	
	float SensorRadius = 100;
	
	float MinSpeed = 100;
	float MaxSpeed = 100;
	
	float SeparationFOV = 1.0;
	float AlignmentFOV = 1.0;
	float CohesionFOV = 1.0;
	
	float SeparationStrength = 1.0;
	float AlignmentStrength = 1.0;
	float CohesionStrength = 1.0;
	float AvoidanceStrength = 1.0;

// thread stuff
	TArray<ABoid*> WorkQueue;
	
	FCriticalSection QueueCriticalSection;
	
	FRunnableThread* Thread;
	
	FThreadSafeBool bIsRunning;
	
// boid translation functions
	void ApplyBoidMovement(ABoid* Boid, float DeltaTime);
	
	static void UpdateMeshRotation(ABoid* Boid, float DeltaTime);
	
	FVector Separate(ABoid* Boid, TArray<AActor*> Flock) const;
	
	FVector Align(ABoid* Boid, TArray<AActor*> Flock) const;
	
	FVector GroupUp(ABoid* Boid, TArray<AActor*> Flock);
	
	bool IsObstacleAhead(ABoid* Boid);
	
	FVector AvoidObstacle(ABoid* Boid);
};
