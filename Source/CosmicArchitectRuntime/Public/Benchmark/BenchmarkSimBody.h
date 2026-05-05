// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchmarkSimBody.generated.h"

class UCosmicOrbitComponent;
class UStaticMeshComponent;

UCLASS()
class COSMICARCHITECTRUNTIME_API ABenchmarkSimBody : public AActor
{
	GENERATED_BODY()
	
public:
	ABenchmarkSimBody();

	// Componente de malla básico (esfera simple)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComponent;

	// Componente orbital
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCosmicOrbitComponent* OrbitComponent;

	// Inicializar con parámetros orbitales aleatorios
	void InitRandomOrbit(AActor* InParentBody, float InSemiMajorAxisKm);

	// Inicializar como cuerpo central (sin órbita)
	void InitAsCentralBody();

};
