#pragma once

struct FBoidWorkerInitialisationData
{
	TArray<FVector> AvoidanceSensors;
	float SensorRadius;
	
	float MinSpeed;
	float MaxSpeed;
	
	float SeparationFOV;
	float AlignmentFOV;
	float CohesionFOV;
	
	float SeparationStrength;
	float AlignmentStrength;
	float CohesionStrength;
	float AvoidanceStrength;
	
	FBoidWorkerInitialisationData(
		TArray<FVector> InAvoidanceSensors,
		float InSensorRadius,
		float InMinSpeed,
		float InMaxSpeed,
		float InSeparationFOV,
		float InAlignmentFOV,
		float InCohesionFOV,
		float InSeparationStrength,
		float InAlignmentStrength,
		float InCohesionStrength,
		float InAvoidanceStrength
	);
};
