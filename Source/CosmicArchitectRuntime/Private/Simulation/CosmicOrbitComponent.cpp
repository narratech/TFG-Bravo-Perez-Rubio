// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/CosmicOrbitComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"

// E: Establece los valores predeterminados de las propiedades de este componente
// I: Sets default values for this component's properties
UCosmicOrbitComponent::UCosmicOrbitComponent()
{
	// E: Configura este componente para inicializarse cuando el juego empieza y hacer tick cada frame. Puedes apagar
	// E: estas funciones para mejorar el rendimiento si no las necesitas.
	// I: Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// I: off to improve performance if you don't need them.
	bTickInEditor = true;

	PrimaryComponentTick.bCanEverTick = true;
}

#if WITH_EDITOR
void UCosmicOrbitComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// E: Obtener el nombre de la propiedad que cambió
	// I: Get the name of the property that changed
	FName PropertyName = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	// E: Lista de propiedades que deberían triggerear una actualización
	// I: List of properties that should trigger an update
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
				// E: Evitar que el actor se asigne a sí mismo como padre
				// I: Prevent the actor from assigning itself as its own parent
				if (ParentBody == Owner)
				{
					ParentBody = nullptr;
					return;
				}

				Owner->AttachToActor(ParentBody,
					FAttachmentTransformRules::KeepWorldTransform);
			}
			else
			{
				Owner->DetachFromActor(
					FDetachmentTransformRules::KeepWorldTransform);
			}
		}
	}

	// E: Si estamos en el editor y no jugando, actualizar posición
	// I: If we are in the editor and not playing, update position
	UWorld* World = GetWorld();
	if (bNeedsUpdate && World && World->WorldType == EWorldType::Editor)
	{
		UpdateInitialOrbitPosition();
	}
}
#endif


// E: Se llama cuando el juego comienza
// I: Called when the game starts
void UCosmicOrbitComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentOrbitTime = OrbitalPeriod * InitialPosition;
}


// E: Se llama cada frame
// I: Called every frame
void UCosmicOrbitComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_EDITOR
	UpdateOrbitVisualization();

	UWorld* World = GetWorld();
	// Solo bloqueamos si estamos en editor Y no estamos simulando.
	if (!World || (World->WorldType == EWorldType::Editor && !bEditorSimulating)) return;
#endif

	// Escala el DeltaTime con el multiplicador inyectado por el generador.
	const float ScaledDelta = DeltaTime * EditorSpeedMultiplier;

	if (AActor* Owner = GetOwner())
	{
		FRotator DeltaRotation = FRotator(0.0f, SpinSpeed * ScaledDelta, 0.0f);
		Owner->AddActorLocalRotation(DeltaRotation);
	}

	if (!ParentBody || OrbitalPeriod <= 0.0f || ScaledDelta <= KINDA_SMALL_NUMBER) return;

	CurrentOrbitTime += ScaledDelta;
	CurrentOrbitTime = FMath::Fmod(CurrentOrbitTime, OrbitalPeriod);

	float MeanMotion = (2.0f * PI) / OrbitalPeriod;
	float MeanAnomaly = MeanMotion * CurrentOrbitTime;
	float E = MeanAnomaly;

	for (int32 i = 0; i < 5; ++i)
	{
		float DeltaE = (E - Eccentricity * FMath::Sin(E) - MeanAnomaly)
			/ (1.0f - Eccentricity * FMath::Cos(E));
		E -= DeltaE;
	}

	float SemiMajorAxisCm = SemiMajorAxisKm * 100000;
	float X = SemiMajorAxisCm * (FMath::Cos(E) - Eccentricity);
	float Y = SemiMajorAxisCm * FMath::Sqrt(1.0f - Eccentricity * Eccentricity) * FMath::Sin(E);

	FVector OrbitalPos(X, Y, 0.0f);
	FRotator OrbitTilt(InclinationX, InclinationY, InclinationZ);
	FVector RotatedPos = OrbitTilt.RotateVector(OrbitalPos);

	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorRelativeLocation(RotatedPos);
	}
}

void UCosmicOrbitComponent::InitOrbit(FColor color)
{
	OrbitThickness = 5000.0f;
	OrbitColor = color;
	UpdateInitialOrbitPosition();
}

void UCosmicOrbitComponent::UpdateInitialOrbitPosition()
{
	// E: Resetear el tiempo orbital
	// I: Reset orbital time
	CurrentOrbitTime = OrbitalPeriod * InitialPosition;

	// E: Calcular posición inicial (t=0)
	// I: Calculate initial position (t=0)
	if (!ParentBody) return;

	CurrentOrbitTime = FMath::Fmod(CurrentOrbitTime, OrbitalPeriod);

	// E: Calcular el porcentaje de la órbita completado
	// I: Calculate the percentage of the orbit completed
	float MeanMotion = (2.0f * PI) / OrbitalPeriod;
	float MeanAnomaly = MeanMotion * CurrentOrbitTime;

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

	// E: Calcular posición en el plano orbital
	// I: Calculate position on the orbital plane
	float SemiMajorAxisCm = SemiMajorAxisKm * 100000.0f;

	float X = SemiMajorAxisCm * (FMath::Cos(E) - Eccentricity);
	float Y = SemiMajorAxisCm * FMath::Sqrt(1.0f - Eccentricity * Eccentricity) * FMath::Sin(E);

	FVector OrbitalPos(X, Y, 0.0f);

	// E: Aplicar inclinación
	// I: Apply inclination
	FRotator OrbitTilt(InclinationX, InclinationY, InclinationZ);
	FVector RotatedPos = OrbitTilt.RotateVector(OrbitalPos);

	// E: Posición final relativa al cuerpo padre
	// I: Final position relative to the parent body
	FVector FinalLocation = RotatedPos;

	// E: Mover el actor
	// I: Move the actor
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorRelativeLocation(FinalLocation);
	}
}

void UCosmicOrbitComponent::UpdateOrbitVisualization()
{
	if (!bShowOrbitInEditor || !ParentBody || !GetOwner()) return;

	UWorld* World = GetWorld();
	if (!World || World->WorldType != EWorldType::Editor) return;

	float SemiMajorAxisCm = SemiMajorAxisKm * 100000.0f;

	// E: Resolución del círculo
	// I: Circle resolution
	int32 NumPoints = OrbitSegments;
	FVector BodyLocation = ParentBody->GetActorLocation();

	TArray<FVector> Points;

	for (int32 i = 0; i <= NumPoints; ++i)
	{
		float Angle = (2.0f * PI * i) / NumPoints;

		// E: Calcular punto en la órbita (aproximación circular para simplificar)
		// E: Para una elipse real, necesitarías resolver Kepler para cada punto
		// I: Calculate point on the orbit (circular approximation for simplicity)
		// I: For a real ellipse, you would need to solve Kepler for each point
		float Radius = SemiMajorAxisCm * (1.0f - Eccentricity * Eccentricity) /
			(1.0f + Eccentricity * FMath::Cos(Angle));

		FVector LocalPos(
			Radius * FMath::Cos(Angle),
			Radius * FMath::Sin(Angle),
			0.0f
		);

		FRotator OrbitTilt(InclinationX, InclinationY, InclinationZ);
		FVector RotatedPos = OrbitTilt.RotateVector(LocalPos);
		Points.Add(BodyLocation + RotatedPos);
	}

	// E: Dibujar líneas entre puntos
	// I: Draw lines between points
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		DrawDebugLine(World, Points[i], Points[i + 1], OrbitColor, false, -1.0f, 0, OrbitThickness);
	}
}