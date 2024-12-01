#include "SteeringBehaviour.h"

void USteeringBehaviour::BeginPlay()
{
	Super::BeginPlay();
	Owner = GetOwner();
	
	SphereComponent = Owner->FindComponentByClass<USphereComponent>();
	CapsuleComponent = Owner->FindComponentByClass<UCapsuleComponent>();
	
	if (!Owner || !SphereComponent || !CapsuleComponent) return;
	
	SphereComponent->IgnoreActorWhenMoving(GetOwner(), true);
	CapsuleComponent->IgnoreActorWhenMoving(GetOwner(), true);
	
	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &USteeringBehaviour::BeginOverlap);
	CapsuleComponent->OnComponentEndOverlap.AddDynamic(this, &USteeringBehaviour::OnOverlapEnd);
}

void USteeringBehaviour::SetCollisionsEnabled(bool bEnabled)
{
	AreCollisionsEnabled = bEnabled;
	if (bEnabled)
	{
		SphereComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CapsuleComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void USteeringBehaviour::BeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// we only care about AvoidanceSpheres for now
	if (!OtherComp->ComponentHasTag("AvoidanceSphere")) return;
	
	FVector DirectionToOverlap = Other->GetActorLocation() - Owner->GetActorLocation();
	
	InterestDirection = DirectionToOverlap * -1;
	bHasInterestDirection = true;
	
	FString Name = Other->GetName();
}

void USteeringBehaviour::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	bHasInterestDirection = false;
	InterestDirection = FVector::Zero();
}
