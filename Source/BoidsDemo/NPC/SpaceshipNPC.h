#pragma once

#include "SpaceshipNPC.generated.h"

// TODO: Boids should extend this class too
UCLASS(BlueprintType, meta=(ShortTooltip="A SpaceshipNPC is the base class for all traffic and boids"), MinimalAPI)
class ASpaceshipNPC : public APawn
{
	GENERATED_BODY()
public:
};
