#include "BoidWorkerInitialisationData.h"

FBoidWorkerInitialisationData::FBoidWorkerInitialisationData(TArray<FVector> InAvoidanceSensors, float InSensorRadius,
	float InMinSpeed, float InMaxSpeed, float InSeparationFOV, float InAlignmentFOV, float InCohesionFOV,
	float InSeparationStrength, float InAlignmentStrength, float InCohesionStrength, float InAvoidanceStrength)
	: AvoidanceSensors(InAvoidanceSensors),
	SensorRadius(InSensorRadius),
	MinSpeed(InMinSpeed),
	MaxSpeed(InMaxSpeed),
	SeparationFOV(InSeparationFOV),
	AlignmentFOV(InAlignmentFOV),
	CohesionFOV(InCohesionFOV),
	SeparationStrength(InSeparationStrength),
	AlignmentStrength(InAlignmentStrength),
	CohesionStrength(InCohesionFOV),
	AvoidanceStrength(InAvoidanceStrength)
{}
