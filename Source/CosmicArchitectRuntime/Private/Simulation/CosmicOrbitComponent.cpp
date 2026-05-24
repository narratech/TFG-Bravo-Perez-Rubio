// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/CosmicOrbitComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"



// Inicializa el componente orbital con configuración por defecto.
//
// Configura:
// - Tick runtime
// - Tick en editor
// - Estado inicial de simulación
UCosmicOrbitComponent::UCosmicOrbitComponent()
{
	// Permite actualización continua tanto en runtime
	// como durante previsualización en editor.
	bTickInEditor = true;
     
	PrimaryComponentTick.bCanEverTick = true;
}

#if WITH_EDITOR



// Actualiza el estado orbital cuando cambian propiedades
// desde el editor.
//
// Responsabilidades:
// - Validación del cuerpo padre
// - Actualización de attachments
// - Reposicionamiento orbital
// - Refresco visual de órbitas
void UCosmicOrbitComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Identifica la propiedad modificada desde el editor.
	FName PropertyName = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	// Determina si el cambio requiere recalcular
	// la posición orbital o visualización.
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
				// Evita referencias circulares donde
				// el actor intenta orbitarse a sí mismo.
				if (ParentBody == Owner)
				{
					ParentBody = nullptr;
					return;
				}

				// Mantiene coordenadas globales al actualizar
				// la jerarquía de attachment.
				Owner->AttachToActor(
					ParentBody,
					FAttachmentTransformRules::KeepWorldTransform);
			}
			else
			{
				// Elimina relación jerárquica manteniendo
				// transform global estable.
				Owner->DetachFromActor(
					FDetachmentTransformRules::KeepWorldTransform);
			}
		}
	}

	// Actualiza inmediatamente la órbita cuando
	// se modifica desde editor fuera de simulación.
	UWorld* World = GetWorld();

	if (bNeedsUpdate && World && World->WorldType == EWorldType::Editor)
	{
		UpdateInitialOrbitPosition();
	}
}

#endif



// Inicializa el estado temporal orbital al comenzar
// la simulación runtime.
void UCosmicOrbitComponent::BeginPlay()
{
	Super::BeginPlay();

	// Convierte la posición inicial normalizada
	// en tiempo orbital absoluto.
	CurrentOrbitTime = OrbitalPeriod * InitialPosition;
}



// Actualiza la simulación orbital cada frame.
//
// El sistema:
// - Integra tiempo orbital
// - Resuelve anomalía excéntrica
// - Calcula posición elíptica
// - Aplica inclinación orbital
// - Actualiza rotación axial
void UCosmicOrbitComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_EDITOR

	UpdateOrbitVisualization();

	UWorld* World = GetWorld();

	// Evita simulación orbital cuando el editor
	// no está ejecutando simulación activa.
	if (!World || (World->WorldType == EWorldType::Editor && !bEditorSimulating))
	{
		return;
	}

#endif

	// Permite acelerar o ralentizar la simulación
	// orbital desde herramientas editoriales.
	const float ScaledDelta = DeltaTime * EditorSpeedMultiplier;

	AActor* Owner = GetOwner();

	// Evita cálculos inválidos o degenerados.
	if (!ParentBody || OrbitalPeriod <= 0.0f || ScaledDelta <= KINDA_SMALL_NUMBER || !Owner)
	{
		return;
	}

	FRotator DeltaRotation = FRotator(0.0f, SpinSpeed * ScaledDelta, 0.0f);

	Owner->AddActorLocalRotation(DeltaRotation);

	// Avanza el tiempo orbital acumulado.
	CurrentOrbitTime += ScaledDelta;

	// Mantiene el tiempo dentro de un ciclo orbital válido.
	CurrentOrbitTime = FMath::Fmod(CurrentOrbitTime, OrbitalPeriod);

	// Movimiento angular medio de la órbita.
	float MeanMotion = (2.0f * PI) / OrbitalPeriod;

	// Anomalía media basada en tiempo orbital actual.
	float MeanAnomaly = MeanMotion * CurrentOrbitTime;

	// Aproximación inicial para Newton-Raphson.
	float E = MeanAnomaly;

	// Resolución iterativa de la ecuación de Kepler:
	//
	//     E - e*sin(E) = M
	//
	// donde:
	// - E = anomalía excéntrica
	// - e = excentricidad
	// - M = anomalía media
	for (int32 i = 0; i < 5; ++i)
	{
		float DeltaE =
			(E - Eccentricity * FMath::Sin(E) - MeanAnomaly)
			/ (1.0f - Eccentricity * FMath::Cos(E));

		E -= DeltaE;
	}

	// Conversión desde kilómetros a centímetros
	// para compatibilidad con Unreal Engine units.
	float SemiMajorAxisCm = SemiMajorAxisKm * 100000;

	// Coordenadas orbitales sobre el plano elíptico local.
	float X = SemiMajorAxisCm * (FMath::Cos(E) - Eccentricity);

	float Y =
		SemiMajorAxisCm *
		FMath::Sqrt(1.0f - Eccentricity * Eccentricity) *
		FMath::Sin(E);

	FVector OrbitalPos(X, Y, 0.0f);

	// Rotación tridimensional de la órbita
	// respecto al sistema de referencia local.
	FRotator OrbitTilt(InclinationX, InclinationY, InclinationZ);

	FVector RotatedPos = OrbitTilt.RotateVector(OrbitalPos);

	Owner->SetActorRelativeLocation(RotatedPos);

}



// Inicializa parámetros visuales básicos
// utilizados por la visualización orbital.
void UCosmicOrbitComponent::InitOrbit(FColor color)
{
	OrbitThickness = 5000.0f;
	OrbitColor = color;

	UpdateInitialOrbitPosition();
}

// Calcula y aplica la posición orbital inicial.
//
// Utilizado principalmente:
// - En editor
// - Durante inicialización
// - Tras cambios de parámetros orbitales
void UCosmicOrbitComponent::UpdateInitialOrbitPosition()
{
	// Convierte la posición inicial normalizada
	// en tiempo orbital absoluto.
	CurrentOrbitTime = OrbitalPeriod * InitialPosition;

	if (!ParentBody)
	{
		return;
	}

	// Mantiene el tiempo dentro de un ciclo orbital válido.
	CurrentOrbitTime = FMath::Fmod(CurrentOrbitTime, OrbitalPeriod);

	// Movimiento angular medio orbital.
	float MeanMotion = (2.0f * PI) / OrbitalPeriod;

	// Anomalía media correspondiente al instante actual.
	float MeanAnomaly = MeanMotion * CurrentOrbitTime;

	float E = MeanAnomaly;

	// Resolución iterativa de la ecuación de Kepler
	// únicamente para órbitas elípticas.
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

	// Conversión desde kilómetros a centímetros.
	float SemiMajorAxisCm = SemiMajorAxisKm * 100000.0f;

	// Posición orbital sobre el plano local elíptico.
	float X = SemiMajorAxisCm * (FMath::Cos(E) - Eccentricity);

	float Y =
		SemiMajorAxisCm *
		FMath::Sqrt(1.0f - Eccentricity * Eccentricity) *
		FMath::Sin(E);

	FVector OrbitalPos(X, Y, 0.0f);

	// Orientación tridimensional de la órbita.
	FRotator OrbitTilt(InclinationX, InclinationY, InclinationZ);

	FVector RotatedPos = OrbitTilt.RotateVector(OrbitalPos);

	// Posición relativa final respecto al cuerpo padre.
	FVector FinalLocation = RotatedPos;

	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorRelativeLocation(FinalLocation);
	}
}



// Genera la representación visual debug de la órbita.
//
// La trayectoria se aproxima mediante segmentos lineales
// utilizando la ecuación polar de una elipse.
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

	// Conversión desde kilómetros a centímetros.
	float SemiMajorAxisCm = SemiMajorAxisKm * 100000.0f;

	// Resolución geométrica utilizada para aproximar
	// la trayectoria orbital.
	int32 NumPoints = OrbitSegments;

	FVector BodyLocation = ParentBody->GetActorLocation();

	TArray<FVector> Points;

	for (int32 i = 0; i <= NumPoints; ++i)
	{
		float Angle = (2.0f * PI * i) / NumPoints;

		// Ecuación polar de una órbita elíptica:
		//
		//     r = a(1 - e²) / (1 + e*cos(theta))
		//
		// donde:
		// - a = semieje mayor
		// - e = excentricidad
		// - theta = ángulo orbital
		float Radius =
			SemiMajorAxisCm *
			(1.0f - Eccentricity * Eccentricity) /
			(1.0f + Eccentricity * FMath::Cos(Angle));

		// Posición local sobre el plano orbital.
		FVector LocalPos(
			Radius * FMath::Cos(Angle),
			Radius * FMath::Sin(Angle),
			0.0f
		);

		// Aplicación de inclinación orbital tridimensional.
		FRotator OrbitTilt(InclinationX, InclinationY, InclinationZ);

		FVector RotatedPos = OrbitTilt.RotateVector(LocalPos);

		Points.Add(BodyLocation + RotatedPos);
	}

	// Construcción visual de la órbita mediante
	// segmentos lineales consecutivos.
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