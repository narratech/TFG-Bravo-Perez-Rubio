// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicEarthLikeNoiseSettings.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicEarthLikeNoiseSettings : public UCosmicNoiseClass
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	int32 Seed;

	UPROPERTY(EditAnywhere, Category = "Noise Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HeightNormalizationScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	FCosmicNoiseDataLayer ContinentalLayer; 

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	FCosmicNoiseDataLayer MountainLayer;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	FCosmicNoiseDataLayer HillLayer;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	FCosmicNoiseDataLayer DetailLayer;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	FCosmicNoiseDataLayer RiverLayer;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	FCosmicNoiseBiomeParameters BiomeParameters;

	virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
	
};
