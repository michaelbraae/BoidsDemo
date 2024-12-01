#pragma once

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"

#include "SteeringBehaviour.generated.h"

UCLASS(ClassGroup=(NPC), meta=(BlueprintSpawnableComponent))
class USteeringBehaviour : public UActorComponent
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	
	UPROPERTY()
	FVector InterestDirection;
	
	UPROPERTY()
	bool bHasInterestDirection = false;
	
	void SetCollisionsEnabled(bool bEnabled);
	
	UPROPERTY()
	bool AreCollisionsEnabled = true;
private:
	UPROPERTY()
	AActor* Owner;
	
	UPROPERTY()
	USphereComponent* SphereComponent;

	UPROPERTY()
	UCapsuleComponent* CapsuleComponent;
	
	UFUNCTION()
	void BeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
