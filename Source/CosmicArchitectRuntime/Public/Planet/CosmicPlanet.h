// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseSettings.h"
#include "GameFramework/Actor.h"
#include "CosmicPlanet.generated.h"

class UCosmicClipmapComponent;
class UCosmicFoliageSpawner;
class UCosmicCollisionComponent;

UCLASS(HideCategories = (
	Replication, Input, Actor, LOD, Activation, Cooking, Networking,
	Physics, Navigation, Tags, DataLayers, LevelInstance))
class COSMICARCHITECTRUNTIME_API ACosmicPlanet : public AActor
{
	GENERATED_BODY()
	
public:	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet")
	float RadiusKm = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCosmicClipmapComponent* ClipmapComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (ShowOnlyInnerProperties))
	UCosmicCollisionComponent* CollisionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Noise")
	UCosmicNoiseSettings* NoiseSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCosmicFoliageSpawner* FoliageSpawnerComponent;

	UPROPERTY(EditAnywhere, Category = "Materials")
	FColor PlanetMainColor1 = FColor::Green;

	UPROPERTY(EditAnywhere, Category = "Materials")
	FColor PlanetMainColor2 = FColor::Red;

	UPROPERTY(EditAnywhere, Category = "Materials")
	FColor PlanetAltitudeColor = FColor::Yellow;

	UPROPERTY(EditAnywhere, Category = "Materials", meta = (ClampMin = "0.1", ClampMax = "20"))
	float MaterialNoiseScale = 1.f;

	
	// Sets default values for this actor's properties
	ACosmicPlanet();

	virtual void OnConstruction(const FTransform& Transform) override;

	void InitPlanet(float InRadiusKm, UCosmicNoiseSettings* NewNoiseSettings, FColor color1, FColor color2, FColor colorHeight, float scale, UMaterialInterface* BaseMaterial, UTexture2D* DefaultTexture);

	void CleanupNoiseSettings();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void InitClipmap();
};
