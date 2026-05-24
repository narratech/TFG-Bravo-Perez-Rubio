// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * 
 */
class COSMICARCHITECTNOISE_API FCosmicEarthLikeNoiseStrategy : public ICosmicNoiseStrategy
{
public:
	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	int32 Seed;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
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

	void Initialize(int32 InSeed,
		float InHeightNormalizationScale,
		FCosmicNoiseBiomeParameters InBiomeParameters,
		FCosmicNoiseDataLayer InContinental,
		FCosmicNoiseDataLayer InMountain,
		FCosmicNoiseDataLayer InHill,
		FCosmicNoiseDataLayer InDetail,
		FCosmicNoiseDataLayer InRiver);

	void EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor) const override;

protected:
	FastNoiseLite ContinentalNoise;
	FastNoiseLite MountainNoise;
	FastNoiseLite HillNoise;
	FastNoiseLite DetailNoise;
	FastNoiseLite RiverNoise;

	FastNoiseLite HumidityNoise;
	FastNoiseLite TempNoise;
};
