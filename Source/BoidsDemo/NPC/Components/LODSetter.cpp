#include "LODSetter.h"


ULODSetter::ULODSetter()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.SetTickFunctionEnable(false);
}

void ULODSetter::BeginPlay()
{
	Super::BeginPlay();
	
	OwningActor = Cast<AActor>(GetOwner());
	PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	
	// LODSetter requires a reference to the player pawn
	if (!OwningActor || !PlayerPawn) return;
	
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

void ULODSetter::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bListenForRenderChanges && !OwningActor->WasRecentlyRendered() && LODLevel != LowLOD)
	{
		LODLevel = LowLOD;
		OnLODChanged.Broadcast(LODLevel);
		return;
	}
	
	float DistanceToPlayer = FVector::Dist(PlayerPawn->GetActorLocation(), OwningActor->GetActorLocation());
	
	UpdateLODLevel(DistanceToPlayer);
}

/**
 * Given a distance to the player, we update our LOD level
 * When a change occurs, we activate or deactivate the necessary components to only have what we need
 * 
 * @param DistanceToPlayer 
 */
void ULODSetter::UpdateLODLevel(const float DistanceToPlayer)
{
	bool bShouldUpdate = false;
	if (DistanceToPlayer < HighLodThreshold)
	{
		if (LODLevel != HighLOD)
		{
			LODLevel = HighLOD;
			bShouldUpdate = true;
		}
	}
	else if (DistanceToPlayer < MediumLodThreshold)
	{
		if (LODLevel != MediumLOD)
		{
			LODLevel = MediumLOD;
			bShouldUpdate = true;
		}
	}
	else if (LODLevel != LowLOD)
	{
		LODLevel = LowLOD;
		bShouldUpdate = true;
	}
	
	// only broadcast the update if our LOD has actually changed
	if (bShouldUpdate) OnLODChanged.Broadcast(LODLevel);
}
