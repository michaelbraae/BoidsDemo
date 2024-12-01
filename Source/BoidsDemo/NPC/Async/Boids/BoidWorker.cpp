#include "BoidWorker.h"

#include "Components/SphereComponent.h"
#include "BoidsDemo/BoidsDemo.h"

FBoidWorker::FBoidWorker(FBoidWorkerInitialisationData InitData) : Thread(nullptr), bIsRunning(false)
{
	AvoidanceSensors = InitData.AvoidanceSensors;
	SensorRadius = InitData.SensorRadius;
	
	MinSpeed = InitData.MinSpeed;
	MaxSpeed = InitData.MaxSpeed;
	
	SeparationFOV = InitData.SeparationFOV;
	AlignmentFOV = InitData.AlignmentFOV;
	CohesionFOV = InitData.CohesionFOV;
	
	SeparationStrength = InitData.SeparationStrength;
	AlignmentStrength = InitData.AlignmentStrength;
	CohesionStrength = InitData.CohesionStrength;
	AvoidanceStrength = InitData.AvoidanceStrength;
}

FBoidWorker::~FBoidWorker()
{
	Stop();
	if (Thread)
	{
		delete Thread;
		Thread = nullptr;
	}
}

void FBoidWorker::Start()
{
	bIsRunning = true;
	Thread = FRunnableThread::Create(this, TEXT("BoidWorkerThread"), 0, TPri_BelowNormal);
	WorkQueue.Empty();
}

void FBoidWorker::Stop()
{
	if (bIsRunning)
	{
		bIsRunning = false;
		
		FPlatformProcess::Sleep(0.1f);
		
		if (Thread)
		{
			Thread->WaitForCompletion();
		}
	}
}

void FBoidWorker::EnqueueWork(ABoid* Boid)
{
	{
		FScopeLock Lock(&QueueCriticalSection);
		WorkQueue.Add(Boid);
	}
}

/**
 * Called each time the worker ticks, this is where we process the queue and perform the translation logic
 * 
 * @return 
 */
uint32 FBoidWorker::Run()
{
	while (bIsRunning)
	{
		UE_LOG(LogTemp, Warning, TEXT("BoidWorker run()"));
		// lock the queue so we dont get race conditions
		TArray<ABoid*> LocalWorkQueue;
		{
			FScopeLock Lock(&QueueCriticalSection);
			LocalWorkQueue = WorkQueue;
			WorkQueue.Empty();
		}
		
		// work within the local queue instead
		for (ABoid* Boid : LocalWorkQueue)
		{
			if (Boid)
			{
				ApplyBoidMovement(Boid, FApp::GetDeltaTime());
				UpdateMeshRotation(Boid, FApp::GetDeltaTime());
			}
		}
		
		FPlatformProcess::Sleep(0.1f);
	}
	
	return 0;
}

/**
 * Calculate the Boid's new velocity, this method also calls the avoidance methods to build the final result
 * 
 * @param Boid 
 * @param DeltaTime 
 */
void FBoidWorker::ApplyBoidMovement(ABoid* Boid, float DeltaTime)
{
	if (!Boid) return;
	
	FVector Acceleration = FVector::ZeroVector;
	
	// find flockmates in general area to fly with
	TArray<AActor*> Flockmates;
	
	Boid->PerceptionSensor->GetOverlappingActors(Flockmates, TSubclassOf<ABoid>());
	Acceleration += Separate(Boid, Flockmates);
	Acceleration += Align(Boid, Flockmates);
	Acceleration += GroupUp(Boid, Flockmates);
	
	// check if heading for collision
	if (IsObstacleAhead(Boid))
	{
		// apply obstacle avoidance force
		Acceleration += AvoidObstacle(Boid);
	}
	
	// update velocity
	Boid->BoidVelocity += (Acceleration * DeltaTime);
	Boid->BoidVelocity = Boid->BoidVelocity.GetClampedToSize(MinSpeed, MaxSpeed);
}

/**
 * Calculate and set the Boid's CurrentRotation variable, this value is used within the Boid itself
 * as setting ActorRotation isn't thread safe
 * 
 * @param Boid 
 * @param DeltaTime 
 */
void FBoidWorker::UpdateMeshRotation(ABoid* Boid, float DeltaTime)
{
	if (!Boid) return;
	
	Boid->CurrentRotation = FMath::RInterpTo(Boid->CurrentRotation, Boid->GetActorRotation(), DeltaTime, 7.0f);
}

/**
 * Identify nearby flockmates and adjust the target vector accordingly
 * @param Boid 
 * @param Flock 
 * @return 
 */
FVector FBoidWorker::Separate(ABoid* Boid, TArray<AActor*> Flock) const
{
	FVector Steering = FVector::ZeroVector;
	int32 FlockCount = 0;
	FVector SeparationDirection = FVector::ZeroVector;
	
	// get separation steering force for each of the boid's flockmates
	for (AActor* OverlapActor : Flock)
	{
		ABoid* Flockmate = Cast<ABoid>(OverlapActor);
		if (Flockmate != nullptr && Flockmate != Boid)
		{
			if (FVector::DotProduct(Boid->GetActorForwardVector(), (Flockmate->GetActorLocation() - Boid->GetActorLocation()).GetSafeNormal()) <= SeparationFOV)
			{
				//flockmate is outside SeparationFOV, disregard it and continue the loop
				continue;
			}
			
			//get normalized direction away from nearby boid
			SeparationDirection = Boid->GetActorLocation() - Flockmate->GetActorLocation();
			SeparationDirection = SeparationDirection.GetSafeNormal();
			
			//get scaling factor based off other boid's proximity. 0 = very far away (no separation force) & 1 = very close (full separation force)
			const float ProximityFactor = 1.0f - (SeparationDirection.Size() / Boid->PerceptionSensor->GetScaledSphereRadius());
			
			if (ProximityFactor < 0.0f)
			{
				// flockmate is outside of PerceptionFOV, disregard and continue loop
				continue;
			}

			//add steering force of flockmate and increase flock count
			Steering += (ProximityFactor * SeparationDirection);
			FlockCount++;
		}
	}

	if (FlockCount > 0)
	{
		// get flock average separation steering force, apply separation steering strength factor and return force
		Steering /= FlockCount;
		Steering.GetSafeNormal() -= Boid->BoidVelocity.GetSafeNormal();
		Steering *= SeparationStrength;
		return Steering;
	}
	
	return FVector::ZeroVector;
}

/**
 * Align with nearby flockmates to give the impression of birds flying together
 * 
 * @param Boid 
 * @param Flock 
 * @return 
 */
FVector FBoidWorker::Align(ABoid* Boid, TArray<AActor*> Flock) const
{
	FVector Steering = FVector::ZeroVector;
	int32 FlockCount = 0.0f;
	
	// get alignment steering force for each of the boid's flockmates
	for (AActor* OverlapActor : Flock)
	{
		ABoid* Flockmate = Cast<ABoid>(OverlapActor);
		if (Flockmate != nullptr && Flockmate != Boid)
		{
			// check if flockmate is outside alignment perception fov
			if (FVector::DotProduct(Boid->GetActorForwardVector(), (Flockmate->GetActorLocation() - Boid->GetActorLocation()).GetSafeNormal()) <= AlignmentFOV)
			{
				// flockmate is outside AlignmentFOV, disregard it and continue
				continue;
			}
			
			Steering += Flockmate->BoidVelocity.GetSafeNormal();
			FlockCount++;
		}
	}
	
	if (FlockCount > 0)
	{
		// get alignment force to average flock direction
		Steering /= FlockCount;
		Steering.GetSafeNormal() -= Boid->BoidVelocity.GetSafeNormal();
		Steering *= AlignmentStrength;
		return Steering;
	}
	
	return FVector::ZeroVector;
}

FVector FBoidWorker::GroupUp(ABoid* Boid, TArray<AActor*> Flock)
{
	int32 FlockCount = 0.0f;
	FVector AveragePosition = FVector::ZeroVector;
	
	//get sum of flockmate positions
	for (AActor* OverlapActor : Flock)
	{
		ABoid* Flockmate = Cast<ABoid>(OverlapActor);
		if (Flockmate != nullptr && Flockmate != Boid)
		{
			//check if flockmate is outside cohesion perception angle
			//TODO: find way to offload this check to collision based solution, sphere collision that have slices for perception FOV elastic grow/shrinking (), (<, (|, (>, etc.?
			if (FVector::DotProduct(Boid->GetActorForwardVector(), (Flockmate->GetActorLocation() - Boid->GetActorLocation()).GetSafeNormal()) <= CohesionFOV)
			{
				continue;	//flockmate is outside viewing angle, disregard this flockmate and continue the loop
			}
			// get cohesive force to group with flockmate
			AveragePosition += Flockmate->GetActorLocation();
			FlockCount++;
		}
	}
	
	if (FlockCount > 0)
	{
		// average cohesion force of flock
		AveragePosition /= FlockCount;
		FVector Steering = AveragePosition - Boid->GetActorLocation();
		Steering.GetSafeNormal() -= Boid->BoidVelocity.GetSafeNormal();
		Steering *= CohesionStrength;
		return Steering;
	}
	
	return FVector::ZeroVector;
}

bool FBoidWorker::IsObstacleAhead(ABoid* Boid)
{
	if (AvoidanceSensors.IsEmpty()) return false;
	
	FQuat SensorRotation = FQuat::FindBetweenVectors(AvoidanceSensors[0], Boid->GetActorForwardVector());
	FVector NewSensorDirection = SensorRotation.RotateVector(AvoidanceSensors[0]);
	//::DrawDebugSphere(InWorld, SamplePos, 6, 6, FColor::Cyan, true, 30.0f);

	FCollisionQueryParams TraceParameters;
	TraceParameters.AddIgnoredActor(Boid);
	FHitResult Hit;
	// run line trace for collision check on 1st sensor
	Boid->GetWorld()->LineTraceSingleByChannel(Hit, Boid->GetActorLocation(), Boid->GetActorLocation() + NewSensorDirection * SensorRadius, ECC_Pawn, TraceParameters);
	
	// check if boid is inside object (i.e. no need to avoid/impossible to)
	if (Hit.bBlockingHit)
	{
		// if (Boid && Boid->GetWorld())
		// {
		// 	// Defer tick enabling to game thread
		// 	AsyncTask(ENamedThreads::GameThread, [Boid, Hit]()
		// 	{
		// 		DrawDebugSphere(Boid->GetWorld(), Hit.Location, 200.0f, 32, FColor::Green, false, 0.0f);
		// 	});
		// }
		
		TArray<AActor*> OverlapActors;
		Boid->BoidCollision->GetOverlappingActors(OverlapActors);
		for (AActor* OverlapActor : OverlapActors)
		{
			if (Hit.GetActor() == OverlapActor)
			{
				return false;
			}
		}
	}
	
	return Hit.bBlockingHit;
}

/**
 * Cast forward to identify obstacles
 * 
 * @param Boid 
 * @return 
 */
FVector FBoidWorker::AvoidObstacle(ABoid* Boid)
{
	if (!Boid) return FVector::ZeroVector;
	
	FQuat SensorRotation = FQuat::FindBetweenVectors(AvoidanceSensors[0], Boid->GetActorForwardVector());
	FVector NewSensorDirection = FVector::ZeroVector;
	FCollisionQueryParams TraceParameters;
	TraceParameters.AddIgnoredActor(Boid);
	FHitResult Hit;
	
	for (FVector AvoidanceSensor : AvoidanceSensors)
	{
		//rotate avoidance sensor to align with boid orientation and trace for collision
		NewSensorDirection = SensorRotation.RotateVector(AvoidanceSensor);
		Boid->GetWorld()->LineTraceSingleByChannel(Hit, Boid->GetActorLocation(), Boid->GetActorLocation() + NewSensorDirection * SensorRadius, ECC_Pawn, TraceParameters);
		
		if (!Hit.bBlockingHit)
		{
			FVector Steering = NewSensorDirection.GetSafeNormal() - Boid->BoidVelocity.GetSafeNormal();
			Steering *= AvoidanceStrength;
			
			return Steering;
		}
	}
	
	return FVector::ZeroVector;
}
