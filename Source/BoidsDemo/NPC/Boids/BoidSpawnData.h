#pragma once

#include "Boid.h"

struct FBoidSpawnData
{
	TSubclassOf<ABoid> BoidType;
	FVector SpawnLocation;
	FRotator SpawnRotation;
	FActorSpawnParameters SpawnParams;

	FBoidSpawnData(
		TSubclassOf<ABoid> InBoidType,
		FVector InSpawnLocation,
		FRotator InSpawnRotation,
		FActorSpawnParameters InSpawnParams
	);
};
