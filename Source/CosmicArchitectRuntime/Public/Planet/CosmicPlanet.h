// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CosmicPlanet.generated.h"

class UCosmicClipmapComponent;

UCLASS()
class COSMICARCHITECTRUNTIME_API ACosmicPlanet : public AActor
{
	GENERATED_BODY()
	
public:	

	UPROPERTY(EditAnywhere, Category = "Planet")
	float Radius;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCosmicClipmapComponent* ClipmapComponent;

	// Sets default values for this actor's properties
	ACosmicPlanet();

	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
