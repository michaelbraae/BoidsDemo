#include "TrafficEntity.h"

#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "../TrafficManager.h"

UTrafficEntity::UTrafficEntity()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.SetTickFunctionEnable(false);
}

/**
 * Begin play is used here to initialise and create references to additional dependencies
 */
void UTrafficEntity::BeginPlay()
{
	Super::BeginPlay();
	
	OwningPawn = Cast<APawn>(GetOwner());
	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	
	// check for invalid references to the player and our owning pawn before we fetch additional dependencies
	if (!OwningPawn || !PlayerPawn) return;
	
	FloatingPawnMovement = OwningPawn->GetComponentByClass<UFloatingPawnMovement>();
	SteeringBehaviour = OwningPawn->GetComponentByClass<USteeringBehaviour>();
	LODSetter = OwningPawn->GetComponentByClass<ULODSetter>();
	
	// make sure any other dependencies are valid before we continue
	if (!FloatingPawnMovement || !SteeringBehaviour || !LODSetter) return;
	
	LODSetter->OnLODChanged.AddDynamic(this, &UTrafficEntity::OnLODChanged);
	FloatingPawnMovement->MaxSpeed = FMath::RandRange(1000, 5000);
	
	// generally this will always be false, as the traffic manager which spawns the entity will perform the registration
	// if you need to spawn a BP_NPC which has this component, ensure that the bool of the same name is true on BP_NPC
	if (bShouldRegisterSelf)
	{
		AActor* TrafficManagerActor = UGameplayStatics::GetActorOfClass(GetWorld(), ATrafficManager::StaticClass()); 
		TrafficManager = Cast<ATrafficManager>(TrafficManagerActor);
		TrafficManager->RegisterTrafficEntity(this);
		PrimaryComponentTick.SetTickFunctionEnable(true);
	}
}

/**
 * Handle the LODSetter's FOnLODLevelChanged event
 * This event is only broadcast when the lod level changes
 * We can perform heavier operations here as the LODSetter ticks only once per second
 *
 * We disable and enable the more detailed behaviours here based on the desired lod level
 * 
 * @param NewLODLevel 
 */
void UTrafficEntity::OnLODChanged(int NewLODLevel)
{
	if (bForceLowLOD)
	{
		if (FloatingPawnMovement->IsComponentTickEnabled()) FloatingPawnMovement->SetComponentTickEnabled(false);
		if (SteeringBehaviour->AreCollisionsEnabled) SteeringBehaviour->SetCollisionsEnabled(false);
		return;
	}
	
	switch (NewLODLevel) {
		case ULODSetter::HighLOD:
			if (!FloatingPawnMovement->IsComponentTickEnabled()) FloatingPawnMovement->SetComponentTickEnabled(true);
			if (!SteeringBehaviour->AreCollisionsEnabled) SteeringBehaviour->SetCollisionsEnabled(true);
			return;
		case ULODSetter::MediumLOD:
			if (!FloatingPawnMovement->IsComponentTickEnabled()) FloatingPawnMovement->SetComponentTickEnabled(true);
			if (SteeringBehaviour->AreCollisionsEnabled) SteeringBehaviour->SetCollisionsEnabled(false); 
			return;
		case ULODSetter::LowLOD:
			if (FloatingPawnMovement->IsComponentTickEnabled()) FloatingPawnMovement->SetComponentTickEnabled(false);
			if (SteeringBehaviour->AreCollisionsEnabled) SteeringBehaviour->SetCollisionsEnabled(false);
			LowLodVector = OwningPawn->GetActorLocation();
		default: ;
	}
}

void UTrafficEntity::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	FVector PawnLocation = OwningPawn->GetActorLocation();
	
	CheckDistanceAndUpdateTarget(PawnLocation);

	if (bForceLowLOD)
	{
		OwningPawn->SetActorLocation(LowLodVector);
		return;
	}
	
	// we handle translation here rather than the worker
	// as calling these methods via an FRunnable isn't thread safe and may causes race conditions or even crashes
	if (LODSetter->LODLevel == ULODSetter::LowLOD)
	// if (true)
	{
		OwningPawn->SetActorLocation(LowLodVector);
		return;
	}
	
	// This is where the thread calculated vector and rotation are applied to the movement component
	FloatingPawnMovement->AddInputVector(TargetInputVector);
	
	TargetRotation = UKismetMathLibrary::FindLookAtRotation(PawnLocation, OwningPawn->GetVelocity() + PawnLocation);
	GetOwner()->SetActorRotation(UKismetMathLibrary::RLerp(OwningPawn->GetActorRotation(), TargetRotation, 0.05f, true));
}

/**
 * Check if we need to move to the next segment
 * if we reach the end, and the beginning is close, we can loop instead of doing a return journey
 *
 * @param PawnLocation 
 */
void UTrafficEntity::CheckDistanceAndUpdateTarget(FVector PawnLocation)
{
	// 1600 here is a distance which feels okay during testing, but we may want to make this more dynamic
	// if the check returns true, we have reached our destination, and can update the CurrentIndex
	if (FVector::Dist(PawnLocation, NextRoadLocation) < 1600)
	{
		// we are at the end, shall we loop or go back?
		// if we reach the end and the distance to the 0 or .Num() element is not too far, we can just loop
		if (CurrentIndex + 1 == RoadLocations.Num() && FVector::Dist(NextRoadLocation, RoadLocations[0]) < 10000)
		{
			CurrentIndex = 0;
			bReturnJourney = false;
		}
		else
		{
			CurrentIndex += bReturnJourney ? -1 : 1;
		}
		
		if (CurrentIndex < 0)
		{
			bReturnJourney = false;
			CurrentIndex = 1;
		}
		else if (CurrentIndex >= RoadLocations.Num())
		{
			bReturnJourney = true;
			CurrentIndex = RoadLocations.Num() - 2;
		}
		
		NextRoadLocation = RoadLocations[CurrentIndex];
	}
}
