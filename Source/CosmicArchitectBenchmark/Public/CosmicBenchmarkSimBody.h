// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CosmicBenchmarkSimBody.generated.h"

class UCosmicOrbitComponent;
class UCosmicGravityComponent;
class UStaticMeshComponent;

UCLASS()
class COSMICARCHITECTBENCHMARK_API ACosmicBenchmarkSimBody : public AActor
{
	GENERATED_BODY()

public:
	ACosmicBenchmarkSimBody();

	// Componente de malla básico (esfera simple) 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	// Componente orbital
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCosmicOrbitComponent* OrbitComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCosmicGravityComponent* GravityComponent;

	// Inicializar con parámetros orbitales aleatorios
	void InitRandomOrbit(AActor* InParentBody, float InSemiMajorAxisKm);

	void InitGravityComponent(bool Nbody);

	// Inicializar como cuerpo central (sin órbita)
	void InitAsCentralBody();

};
