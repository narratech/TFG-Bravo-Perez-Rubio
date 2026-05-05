// Fill out your copyright notice in the Description page of Project Settings.

#include "Benchmark/BenchmarkSimBody.h"
#include "Simulation/CosmicOrbitComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ABenchmarkSimBody::ABenchmarkSimBody()
{
	PrimaryActorTick.bCanEverTick = false; // El OrbitComponent ya hace tick

	// Crear malla estática simple (esfera por defecto de UE)
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	// Cargar la esfera por defecto del motor
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere")
	);

	if (SphereMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(SphereMesh.Object);
	}

	// Escala pequeña para que sea una esfera simple
	MeshComponent->SetWorldScale3D(FVector(10.f));

	// Sin colisión para reducir overhead
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Sin sombras para reducir overhead de renderizado
	MeshComponent->SetCastShadow(false);
	MeshComponent->SetReceivesDecals(false);

	// IMPORTANTE: Hacerlo movable
	MeshComponent->SetMobility(EComponentMobility::Movable);

	// Crear componente orbital
	OrbitComponent = CreateDefaultSubobject<UCosmicOrbitComponent>(TEXT("OrbitComponent"));
}

void ABenchmarkSimBody::InitRandomOrbit(AActor* InParentBody, float InSemiMajorAxisKm)
{
	if (!OrbitComponent) return;

	OrbitComponent->ParentBody = InParentBody;
	OrbitComponent->SemiMajorAxisKm = InSemiMajorAxisKm;

	// Valores aleatorios para variedad
	OrbitComponent->Eccentricity = FMath::FRandRange(0.0f, 0.5f);
	OrbitComponent->OrbitalPeriod = FMath::FRandRange(5.0f, 30.0f);
	OrbitComponent->InitialPosition = FMath::FRandRange(0.0f, 1.0f);
	OrbitComponent->InclinationX = FMath::FRandRange(0.0f, 30.0f);
	OrbitComponent->InclinationY = FMath::FRandRange(0.0f, 30.0f);
	OrbitComponent->InclinationZ = FMath::FRandRange(0.0f, 30.0f);
	OrbitComponent->SpinSpeed = FMath::FRandRange(-45.0f, 45.0f);
}

void ABenchmarkSimBody::InitAsCentralBody()
{
	if (OrbitComponent)
	{
		OrbitComponent->ParentBody = nullptr;
		OrbitComponent->SemiMajorAxisKm = 0.0f;
	}
}