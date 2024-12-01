#pragma once

#include "Components/SphereComponent.h"

#include "RoadMarker.generated.h"

UCLASS(Blueprintable)
class ARoadMarker : public AActor
{
	GENERATED_BODY()
public:
	ARoadMarker();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road Marker", meta = (ExposeOnSpawn))
	bool bIsBranch = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Road Marker", meta = (ExposeOnSpawn))
	int32 RoadNumber = 0;
	
private:
	
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USphereComponent* SphereComponent;
};
