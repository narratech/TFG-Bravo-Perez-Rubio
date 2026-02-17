// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/OrbitComponent.h"
#include "Math/UnrealMathUtility.h"

// Sets default values for this component's properties
UOrbitComponent::UOrbitComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	//Permitir tick en el editor
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

}

#if WITH_EDITOR
void UOrbitComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Obtener el nombre de la propiedad que cambió
	FName PropertyName = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	// Lista de propiedades que deberían triggerear una actualización
	bool bNeedsUpdate = (PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, ParentBody) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, SemiMajorAxisKm) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, Eccentricity) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, Inclination) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, OrbitalPeriod));

	// Si estamos en el editor y no jugando, actualizar posición
	UWorld* World = GetWorld();
	if (bNeedsUpdate && World && World->WorldType == EWorldType::Editor)
	{
		UpdateInitialOrbitPosition();
	}
}
#endif


// Called when the game starts
void UOrbitComponent::BeginPlay()
{
	Super::BeginPlay();

	UpdateInitialOrbitPosition();
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


void UOrbitComponent::UpdateInitialOrbitPosition()
{
	// Resetear el tiempo orbital
	CurrentOrbitTime = 0.0f;

	// Calcular posición inicial (t=0)
	if (!ParentBody) return;

	float MeanAnomaly = 0.0f; // Para t=0, la anomalía media es 0

	// Resolver ecuación de Kepler para t=0
	float E = MeanAnomaly;
	if (Eccentricity > 0.0f)
	{
		for (int32 i = 0; i < 5; ++i) {
			float SinE = FMath::Sin(E);
			float CosE = FMath::Cos(E);

			float DeltaE = (E - Eccentricity * SinE - MeanAnomaly) / (1.0f - Eccentricity * CosE);
			E -= DeltaE;
		}
	}

	// Calcular posición en el plano orbital
	float SemiMajorAxisCm = SemiMajorAxisKm * 100000.0f;

	float X = SemiMajorAxisCm * (FMath::Cos(E) - Eccentricity);
	float Y = SemiMajorAxisCm * FMath::Sqrt(1.0f - Eccentricity * Eccentricity) * FMath::Sin(E);

	FVector OrbitalPos(X, Y, 0.0f);

	// Aplicar inclinación
	FRotator OrbitTilt(Inclination, 0.0f, 0.0f);
	FVector RotatedPos = OrbitTilt.RotateVector(OrbitalPos);

	// Posición final relativa al cuerpo padre
	FVector FinalLocation = ParentBody->GetActorLocation() + RotatedPos;

	// Mover el actor
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(FinalLocation);
	}
}

