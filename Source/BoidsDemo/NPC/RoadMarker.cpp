#include "RoadMarker.h"

ARoadMarker::ARoadMarker()
{
	// these shouldn't be spatially loaded, so the AI can get a reference to them all at begin play
	// bIsSpatiallyLoaded = false;
	
	// add a sphere component so we can see the actor in editor
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->InitSphereRadius(2000.0f);
	SphereComponent->SetLineThickness(100.0f);
	SphereComponent->SetHiddenInGame(true);
	SphereComponent->ShapeColor = FColor::Purple;
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	RootComponent = SphereComponent;
}
