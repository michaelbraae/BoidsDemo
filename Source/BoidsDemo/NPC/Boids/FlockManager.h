#pragma once

//includes
#include "CoreMinimal.h"
#include "BoidSpawnData.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "BoidsDemo/NPC/Async/Boids/BoidWorker.h"
#include "BoidsDemo/NPC/Components/LODSetter.h"
#include "FlockManager.generated.h"

class UBillboardComponent;
class ABoid;

UCLASS()
class BOIDSDEMO_API AFlockManager : public AActor
{
	GENERATED_BODY()

public:
	AFlockManager();
	virtual ~AFlockManager() override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	void CreateWorker();

	//billboard visuals component to help in editor
	UPROPERTY(VisibleAnywhere, Category = "Boid|Components")
	UBillboardComponent* FlockManagerBillboard;
	
	UPROPERTY()
	ULODSetter* LODSetter;
	
	UPROPERTY()
	TArray<ABoid*> BoidsInFlock;
	
	UFUNCTION()
	void OnLODChanged(int NewLODLevel);
	
public:
	void AddBoidToFlock(ABoid* Boid);
	
	void RemoveBoidFromFlock(ABoid* Boid);
	
	void DestroyBoids();
	
protected:
	UPROPERTY(EditAnywhere, Category = "Boid|Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 700.0f;
	UPROPERTY(EditAnywhere, Category = "Boid|Movement", meta = (ClampMin = "0.0"))
	float MinSpeed = 300.0f;
	
	// SPAWNING
	bool bSpawningBoids = false;
	
	TArray<FBoidSpawnData> BoidsToSpawn;
	TArray<FBoidSpawnData> BoidsToDestroy;
	
	UPROPERTY(EditAnywhere, Category = "Boid|Spawn", meta = (Description = "The distance where the boids will be culled or spawned"))
	float CullDistance;
	
	UPROPERTY(EditAnywhere, Category = "Boid|Spawn")
	TSubclassOf<ABoid> BoidType;
	
	UPROPERTY(EditAnywhere, Category = "Boid|Spawn", meta = (ClampMin = "0", ClampMax = "500"))
	int32 NumBoidsToSpawn;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boid|Components")
	UBoxComponent* BoundaryCollision;
	
	UFUNCTION()
	void OnCageOverlapEnd(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);
	
	void SpawnBoids(int32 NumBoids);
	
public:
	UFUNCTION(BlueprintCallable, Category = "Boid|Movement")
	float GetMaxSpeed() { return MaxSpeed; };
	UFUNCTION(BlueprintCallable, Category = "Boid|Movement")
	float GetMinSpeed() { return MinSpeed; };
	
	// STEERING
protected:
	// TODO: add clamping 
	UPROPERTY(EditAnywhere, Category = "Boid|Steering")
	float AlignmentStrength = 200.0f;
	UPROPERTY(EditAnywhere, Category = "Boid|Steering")
	float SeparationStrength = 30.0f;
	UPROPERTY(EditAnywhere, Category = "Boid|Steering")
	float CohesionStrength = 5.0f;
	UPROPERTY(EditAnywhere, Category = "Boid|Steering")
	float AvoidanceStrength = 10000.0f;
	
	//PERCEPTION
	//determines the field of view of each perception field to determine if a flockmate is sensed (1.0 boid can only sense things directly in front of it, -1 boid can sense in all directions)
	UPROPERTY(EditAnywhere, Category = "Boid|Perception", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float SeparationFOV = -1.0f;
	UPROPERTY(EditAnywhere, Category = "Boid|Perception", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float AlignmentFOV = 0.5f;
	UPROPERTY(EditAnywhere, Category = "Boid|Perception", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float CohesionFOV = -0.5f;
	
	UPROPERTY(EditAnywhere, Category = "Boid|Avoidance", meta = (ClampMin = "0", ClampMax = "1000"))
	int32 NumSensors = 100;
	
	TArray<FVector> AvoidanceSensors;
	
	const float GoldenRatio = (1.0f + FMath::Sqrt(5.0f)) / 2;

	UPROPERTY(EditAnywhere, Category = "Boid|Avoidance", meta = (ClampMin = "0"))
	float SensorRadius = 300.f;
	
	void BuildAvoidanceSensors();

private:
	TUniquePtr<FBoidWorker> BoidWorker;
	
public:
	TArray<FVector> GetAvoidanceSensors() { return AvoidanceSensors; }
};