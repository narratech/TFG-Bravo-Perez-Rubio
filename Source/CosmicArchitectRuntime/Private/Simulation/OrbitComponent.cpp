// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/OrbitComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"

// Sets default values for this component's properties
UOrbitComponent::UOrbitComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bTickInEditor = true;

	PrimaryComponentTick.bCanEverTick = true;
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
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, InclinationX) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, InclinationY) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, InclinationZ) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, OrbitColor) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, OrbitSegments) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, OrbitThickness) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, bShowOrbitInEditor) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(UOrbitComponent, InitialPosition) ||
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
	CurrentOrbitTime = OrbitalPeriod * InitialPosition;
}


// Called every frame
void UOrbitComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_EDITOR
	UpdateOrbitVisualization();

	UWorld* World = GetWorld();
	if (!World || World->WorldType == EWorldType::Editor) return;
#endif

	if (AActor* Owner = GetOwner())
	{
		UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Owner->GetRootComponent());

		FRotator DeltaRotation = FRotator(0.0f, SpinSpeed * DeltaTime, 0.0f);
		Owner->AddActorLocalRotation(DeltaRotation); //Aplicar rotación sobre si mismo
	}

	if (!ParentBody || OrbitalPeriod <= 0.0f) return;

	CurrentOrbitTime += DeltaTime;

	CurrentOrbitTime = FMath::Fmod(CurrentOrbitTime, OrbitalPeriod);

	float MeanMotion = (2.0f * PI) / OrbitalPeriod; //Calcular el porcentaje de la órbita completado
	float MeanAnomaly = MeanMotion * CurrentOrbitTime;

	float E = MeanAnomaly;

	for (int32 i = 0; i < 5; ++i) { //Iteraciones de la ecuacion de Kepler
		float SinE = FMath::Sin(E);
		float CosE = FMath::Cos(E);

		float DeltaE = (E - Eccentricity * SinE - MeanAnomaly) / (1.0f - Eccentricity * CosE);
		E -= DeltaE;
	}

	float SemiMajorAxisCm = SemiMajorAxisKm * 100000;

	//Calcular posicion final
	float X = SemiMajorAxisCm * (FMath::Cos(E) - Eccentricity);
	float Y = SemiMajorAxisCm * FMath::Sqrt(1.0f - Eccentricity * Eccentricity) * FMath::Sin(E);

	FVector OrbitalPos(X, Y, 0.0f);

	//Aplicar inclinacion (Rotar plano de la orbita)
	FRotator OrbitTilt(InclinationX, InclinationY, InclinationZ);
	FVector RotatedPos = OrbitTilt.RotateVector(OrbitalPos);

	FVector FinalLocation = ParentBody->GetActorLocation() + RotatedPos;

	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(FinalLocation);//Mover actor a la posicion
	}
}

void UOrbitComponent::InitOrbit(FColor color)
{
	OrbitThickness = 5000.0f;
	OrbitColor = color;
	UpdateInitialOrbitPosition();
}

void UOrbitComponent::UpdateInitialOrbitPosition()
{
	// Resetear el tiempo orbital
	CurrentOrbitTime = OrbitalPeriod * InitialPosition;

	// Calcular posición inicial (t=0)
	if (!ParentBody) return;

	CurrentOrbitTime = FMath::Fmod(CurrentOrbitTime, OrbitalPeriod);

	float MeanMotion = (2.0f * PI) / OrbitalPeriod; //Calcular el porcentaje de la órbita completado
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

	// Calcular posición en el plano orbital
	float SemiMajorAxisCm = SemiMajorAxisKm * 100000.0f;

	float X = SemiMajorAxisCm * (FMath::Cos(E) - Eccentricity);
	float Y = SemiMajorAxisCm * FMath::Sqrt(1.0f - Eccentricity * Eccentricity) * FMath::Sin(E);

	FVector OrbitalPos(X, Y, 0.0f);

	// Aplicar inclinación
	FRotator OrbitTilt(InclinationX, InclinationY, InclinationZ);
	FVector RotatedPos = OrbitTilt.RotateVector(OrbitalPos);

	// Posición final relativa al cuerpo padre
	FVector FinalLocation = ParentBody->GetActorLocation() + RotatedPos;

	// Mover el actor
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(FinalLocation);
	}
}

void UOrbitComponent::UpdateOrbitVisualization()
{
	if (!bShowOrbitInEditor || !ParentBody || !GetOwner()) return;

	UWorld* World = GetWorld();
	if (!World || World->WorldType != EWorldType::Editor) return;

	float SemiMajorAxisCm = SemiMajorAxisKm * 100000.0f;
	int32 NumPoints = OrbitSegments; // Resolución del círculo
	FVector BodyLocation = ParentBody->GetActorLocation();

	TArray<FVector> Points;

	for (int32 i = 0; i <= NumPoints; ++i)
	{
		float Angle = (2.0f * PI * i) / NumPoints;

		// Calcular punto en la órbita (aproximación circular para simplificar)
		// Para una elipse real, necesitarías resolver Kepler para cada punto
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

	// Dibujar líneas entre puntos
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		DrawDebugLine(World, Points[i], Points[i + 1], OrbitColor, false, -1.0f, 0, OrbitThickness);
	}
}


