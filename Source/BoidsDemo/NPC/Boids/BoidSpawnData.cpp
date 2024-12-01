#include "BoidSpawnData.h"

FBoidSpawnData::FBoidSpawnData(
	TSubclassOf<ABoid> InBoidType,
	FVector InSpawnLocation,
	FRotator InSpawnRotation,
	FActorSpawnParameters InSpawnParams
) : BoidType(InBoidType),
	SpawnLocation(InSpawnLocation),
	SpawnRotation(InSpawnRotation),
	SpawnParams(InSpawnParams)
{}
