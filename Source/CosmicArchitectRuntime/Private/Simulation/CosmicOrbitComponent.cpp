// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "Simulation/CosmicOrbitComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"



// Initializes orbital component with default configuration.
//
// Configures:
// - Runtime tick
// - Editor tick
// - Initial simulation state
UCosmicOrbitComponent::UCosmicOrbitComponent()
{
	// Allows continuous updating both at runtime
	// and during editor preview.
	bTickInEditor = true;
     
	PrimaryComponentTick.bCanEverTick = true;
}

#if WITH_EDITOR



// Updates orbital state when properties change
// from the editor.
//
// Responsibilities:
// - Parent body validation
// - Attachment update
// - Orbital repositioning
// - Visual orbit refresh
void UCosmicOrbitComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Identifies modified property from editor.
	FName PropertyName = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	// Determines if change requires recalculating
	// orbital position or visualization.
	bool bNeedsUpdate = (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, ParentBody) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, SemiMajorAxisKm) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, Eccentricity) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, InclinationX) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, InclinationY) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, InclinationZ) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, OrbitColor) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, OrbitSegments) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, OrbitThickness) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, bShowOrbitInEditor) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, InitialPosition) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, OrbitalPeriod));

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOrbitComponent, ParentBody))
	{
		if (AActor* Owner = GetOwner())
		{
			if (ParentBody)
			{
				// Avoid circular references where
				// actor tries to orbit itself.
				if (ParentBody == Owner)
				{
					ParentBody = nullptr;
					return;
				}

				// Maintain global coordinates when updating
				// attachment hierarchy.
				Owner->AttachToActor(
					ParentBody,
					FAttachmentTransformRules::KeepWorldTransform);
			}
			else
			{
				// Remove hierarchical relationship while maintaining
				// stable global transform.
				Owner->DetachFromActor(
					FDetachmentTransformRules::KeepWorldTransform);
			}
		}
	}

	// Immediately update orbit when
	// modified from editor outside simulation.
	UWorld* World = GetWorld();

	if (bNeedsUpdate && World && World->WorldType == EWorldType::Editor)
	{
		UpdateInitialOrbitPosition();
	}
}

#endif



// Initializes temporal orbital state upon starting
// runtime simulation.
void UCosmicOrbitComponent::BeginPlay()
{
	Super::BeginPlay();

	// Converts normalized initial position
	// into absolute orbital time.
	CurrentOrbitTime = OrbitalPeriod * InitialPosition;
}



// Updates orbital simulation each frame.
//
// The system:
// - Integrates orbital time
// - Resolves eccentric anomaly
// - Calculates elliptical position
// - Applies orbital inclination
// - Updates axial rotation
void UCosmicOrbitComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_EDITOR

	UpdateOrbitVisualization();

	UWorld* World = GetWorld();

	// Avoid orbital simulation when editor
	// is not executing active simulation.
	if (!World || (World->WorldType == EWorldType::Editor && !bEditorSimulating))
	{
		return;
	}

#endif

	// Allows accelerating or decelerating orbital simulation
	// from editor tools.
	const float ScaledDelta = DeltaTime * EditorSpeedMultiplier;

	AActor* Owner = GetOwner();

	// Avoid invalid or degenerate calculations.
	if (!ParentBody || OrbitalPeriod <= 0.0f || ScaledDelta <= KINDA_SMALL_NUMBER || !Owner)
	{
		return;
	}

	FRotator DeltaRotation = FRotator(0.0f, SpinSpeed * ScaledDelta, 0.0f);

	Owner->AddActorLocalRotation(DeltaRotation);

	// Advance accumulated orbital time.
	CurrentOrbitTime += ScaledDelta;

	// Keep time within a valid orbital cycle.
	CurrentOrbitTime = FMath::Fmod(CurrentOrbitTime, OrbitalPeriod);

	// Mean angular motion of orbit.
	float MeanMotion = (2.0f * PI) / OrbitalPeriod;

	// Mean anomaly based on current orbital time.
	float MeanAnomaly = MeanMotion * CurrentOrbitTime;

	// Initial approximation for Newton-Raphson.
	float E = MeanAnomaly;

	// Iterative resolution of Kepler equation:
	//
	//     E - e*sin(E) = M
	//
	// where:
	// - E = eccentric anomaly
	// - e = eccentricity
	// - M = mean anomaly
	for (int32 i = 0; i < 5; ++i)
	{
		float DeltaE =
			(E - Eccentricity * FMath::Sin(E) - MeanAnomaly)
			/ (1.0f - Eccentricity * FMath::Cos(E));

		E -= DeltaE;
	}

	// Conversion from kilometers to centimeters
	// for compatibility with Unreal Engine units.
	float SemiMajorAxisCm = SemiMajorAxisKm * 100000;

	// Orbital coordinates on local elliptical plane.
	float X = SemiMajorAxisCm * (FMath::Cos(E) - Eccentricity);

	float Y =
		SemiMajorAxisCm *
		FMath::Sqrt(1.0f - Eccentricity * Eccentricity) *
		FMath::Sin(E);

	FVector OrbitalPos(X, Y, 0.0f);

	// Three-dimensional orbit rotation
	// relative to local reference frame.
	FRotator OrbitTilt(InclinationX, InclinationY, InclinationZ);

	FVector RotatedPos = OrbitTilt.RotateVector(OrbitalPos);

	Owner->SetActorRelativeLocation(RotatedPos);

}



// Initializes basic visual parameters
// used by orbital visualization.
void UCosmicOrbitComponent::InitOrbit(FColor color)
{
	OrbitThickness = 5000.0f;
	OrbitColor = color;

	UpdateInitialOrbitPosition();
}

// Calculates and applies initial orbital position.
//
// Used primarily:
// - In editor
// - During initialization
// - After orbital parameter changes
void UCosmicOrbitComponent::UpdateInitialOrbitPosition()
{
	// Converts normalized initial position
	// into absolute orbital time.
	CurrentOrbitTime = OrbitalPeriod * InitialPosition;

	if (!ParentBody)
	{
		return;
	}

	// Keep time within a valid orbital cycle.
	CurrentOrbitTime = FMath::Fmod(CurrentOrbitTime, OrbitalPeriod);

	// Mean orbital angular motion.
	float MeanMotion = (2.0f * PI) / OrbitalPeriod;

	// Mean anomaly corresponding to current time.
	float MeanAnomaly = MeanMotion * CurrentOrbitTime;

	float E = MeanAnomaly;

	// Iterative resolution of Kepler equation
	// for elliptical orbits only.
	if (Eccentricity > 0.0f)
	{
		for (int32 i = 0; i < 5; ++i)
		{
			float SinE = FMath::Sin(E);
			float CosE = FMath::Cos(E);

			float DeltaE =
				(E - Eccentricity * SinE - MeanAnomaly)
				/ (1.0f - Eccentricity * CosE);

			E -= DeltaE;
		}
	}

	// Conversion from kilometers to centimeters.
	float SemiMajorAxisCm = SemiMajorAxisKm * 100000.0f;

	// Orbital position on local elliptical plane.
	float X = SemiMajorAxisCm * (FMath::Cos(E) - Eccentricity);

	float Y =
		SemiMajorAxisCm *
		FMath::Sqrt(1.0f - Eccentricity * Eccentricity) *
		FMath::Sin(E);

	FVector OrbitalPos(X, Y, 0.0f);

	// Three-dimensional orientation of orbit.
	FRotator OrbitTilt(InclinationX, InclinationY, InclinationZ);

	FVector RotatedPos = OrbitTilt.RotateVector(OrbitalPos);

	// Final relative position with respect to parent body.
	FVector FinalLocation = RotatedPos;

	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorRelativeLocation(FinalLocation);
	}
}



// Generates debug visual representation of orbit.
//
// Trajectory is approximated via linear segments
// using polar equation of an ellipse.
void UCosmicOrbitComponent::UpdateOrbitVisualization()
{
	if (!bShowOrbitInEditor || !ParentBody || !GetOwner())
	{
		return;
	}

	UWorld* World = GetWorld();

	if (!World || World->WorldType != EWorldType::Editor)
	{
		return;
	}

	// Conversion from kilometers to centimeters.
	float SemiMajorAxisCm = SemiMajorAxisKm * 100000.0f;

	// Geometric resolution used to approximate
	// orbital trajectory.
	int32 NumPoints = OrbitSegments;

	FVector BodyLocation = ParentBody->GetActorLocation();

	TArray<FVector> Points;

	for (int32 i = 0; i <= NumPoints; ++i)
	{
		float Angle = (2.0f * PI * i) / NumPoints;

		// Polar equation of an elliptical orbit:
		//
		//     r = a(1 - e²) / (1 + e*cos(theta))
		//
		// where:
		// - a = semi-major axis
		// - e = eccentricity
		// - theta = orbital angle
		float Radius =
			SemiMajorAxisCm *
			(1.0f - Eccentricity * Eccentricity) /
			(1.0f + Eccentricity * FMath::Cos(Angle));

		// Local position on orbital plane.
		FVector LocalPos(
			Radius * FMath::Cos(Angle),
			Radius * FMath::Sin(Angle),
			0.0f
		);

		// Three-dimensional orbital inclination application.
		FRotator OrbitTilt(InclinationX, InclinationY, InclinationZ);

		FVector RotatedPos = OrbitTilt.RotateVector(LocalPos);

		Points.Add(BodyLocation + RotatedPos);
	}

	// Visual orbit construction via
	// consecutive linear segments.
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		DrawDebugLine(
			World,
			Points[i],
			Points[i + 1],
			OrbitColor,
			false,
			-1.0f,
			0,
			OrbitThickness);
	}
}