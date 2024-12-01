
#pragma once

//includes
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoidsDemo/NPC/SpaceshipNPC.h"
#include "Boid.generated.h"

//forward declares
class UStaticMeshComponent;
class USphereComponent;
class AFlockManager;

UCLASS()
class BOIDSDEMO_API ABoid : public ASpaceshipNPC
{
	GENERATED_BODY()

public:
	ABoid();
	
	//called to update each frame
	virtual void Tick(float DeltaTime) override;
	
protected:
	//called on level start or when spawned
	virtual void BeginPlay() override;
	
public:
	//boid collision component
	UPROPERTY(VisibleAnywhere, Category = "Boid|Components")
	USphereComponent* BoidCollision;
	
	//flock manager that controls this boid's perception and steering behaviors
	AFlockManager* FlockManager;
	
	//collision sensor used to generate cohesion steering forces
	UPROPERTY(VisibleAnywhere, Category = "Boid|Components")
	USphereComponent* PerceptionSensor;
	
	inline AFlockManager* GetFlockManager() { return FlockManager; }

	//boid static mesh component for visuals
	// UPROPERTY(VisibleAnywhere, Category = "Boid|Components")
	// UStaticMeshComponent* BoidMesh;
	// TODO: create "FindFlockManager" helper function to look for a flock manager for cases where they don't get properly assigned (i.e. a manually placed boid in editor).
	
	//boid velocity
	FVector BoidVelocity;
	
	//boid mesh rotation used for smoothly interpolating mesh rotations
	FRotator CurrentRotation;
	
	UFUNCTION(BlueprintCallable)
	inline FVector GetBoidVelocity() { return BoidVelocity; };
};