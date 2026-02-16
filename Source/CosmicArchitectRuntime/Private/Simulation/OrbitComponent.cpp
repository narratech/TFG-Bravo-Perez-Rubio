// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/OrbitComponent.h"
#include "Math/UnrealMathUtility.h"

// Sets default values for this component's properties
UOrbitComponent::UOrbitComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UOrbitComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UOrbitComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ParentBody || OrbitalPeriod <= 0.0f) return;

	CurrentOrbitTime += DeltaTime;

	CurrentOrbitTime = FMath::Fmod(CurrentOrbitTime, OrbitalPeriod);

	float MeanMotion = (2.0f * PI) / OrbitalPeriod;
	float MeanAnomaly = MeanMotion * CurrentOrbitTime;

	float E = MeanAnomaly;

	for (int32 i = 0; i < 5; ++i) {
		float SinE = FMath::Sin(E);
		float CosE = FMath::Cos(E);

		float DeltaE = (E - Eccentricity * SinE - MeanAnomaly) / (1.0f - Eccentricity * CosE);
		E -= DeltaE;
	}

	float SemiMajorAxisCm = SemiMajorAxisKm * 100000;

	float X = SemiMajorAxisCm * (FMath::Cos(E) - Eccentricity);
	float Y = SemiMajorAxisCm * FMath::Sqrt(1.0f - Eccentricity * Eccentricity) * FMath::Sin(E);

	FVector OrbitalPos(X, Y, 0.0f);

	FRotator OrbitTilt(Inclination, 0.0f, 0.0f);
	FVector RotatedPos = OrbitTilt.RotateVector(OrbitalPos);

	FVector FinalLocation = ParentBody->GetActorLocation() + RotatedPos;

	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(FinalLocation);
	}
}

