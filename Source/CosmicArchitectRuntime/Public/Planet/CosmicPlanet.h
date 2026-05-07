// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CosmicPlanet.generated.h"

class UCosmicClipmapComponent;
class UCosmicNoiseClass;
class UCosmicFoliageSpawner;
class UCosmicCollisionComponent;
class UCosmicOceanComponent;
class UCosmicFoliageCollection;
class UMaterialInstance;

UCLASS(HideCategories = (
	Replication, Input, Actor, LOD, Activation, Cooking, Networking,
	Physics, Navigation, Tags, DataLayers, LevelInstance))
class COSMICARCHITECTRUNTIME_API ACosmicPlanet : public AActor
{
	GENERATED_BODY()
	
public:	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet")
	double RadiusKm = 1.0;

	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	TObjectPtr<UCosmicClipmapComponent> ClipmapComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	UCosmicCollisionComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	UCosmicOceanComponent* OceanComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Noise")
	UCosmicNoiseClass* NoiseClass;

	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	UCosmicFoliageSpawner* FoliageSpawnerComponent;

	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetMainColor1 = FColor::Red;

	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetMainColor2 = FColor::Orange;

	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetColdColor = FColor::White;

	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetHotColor = FColor::Red;

	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetSlopeColor = FColor::Black;

	UPROPERTY(EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleSmall = 1.f;

	UPROPERTY(EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleMedium = 3.f;

	UPROPERTY(EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleLarge = 100.f;

	
	// Sets default values for this actor's properties
	ACosmicPlanet();

#if WITH_EDITOR
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	void InitPlanet(
		float InRadiusKm,
		UCosmicNoiseClass* NewNoiseClass,
		FColor Color1, FColor Color2, FColor ColorCold, FColor ColorHot,
		FColor ColorSlope, float ScaleL, float ScaleM, float ScaleS,
		UMaterialInstance* BaseMaterial,
		UTexture2D* DefaultTexture,
		// Clipmap
		bool UseClipmap = true,
		int32 InBaseResolution = 128,
		int32 InNumLevels = 4,
		int32 InMinTriangleSize = 100,
		float InHeightVisibility = 5.0f,
		// Ocean
		bool  bInHasOcean = true,
		double InSeaLevelKm = 0.0,
		int32 InOceanResolution = 128,
		UMaterialInstance* InOceanMaterial = nullptr,
		// Foliage
		UCosmicFoliageCollection* InFoliageCollection = nullptr
	);

	void SetFoliageParams(
		int32 InFoliageInstancesPerFrame = 50.f,
		float NearLayerRadiusKm = 0.05f,
		float MediumLayerRadiusKm = 0.2f,
		float FarLayerRadiusKm = 0.5f);

	void CleanupNoiseSettings();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostDuplicate(EDuplicateMode::Type Mode) override;
#endif

	virtual void Destroyed() override;

	virtual void BeginDestroy() override;

	virtual void PostInitializeComponents() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InitClipmap();

	void RebuildPlanet();

	void UpdateNoiseSettings();

	void UpdateFoliage();

	void UpdateOcean();

	void UpdateMaterialOnly();

	void ClearData();

	bool bInitializedInEditor = false;

#if WITH_EDITOR
	// Se llama automáticamente cuando cambias algo en el panel de Detalles
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
