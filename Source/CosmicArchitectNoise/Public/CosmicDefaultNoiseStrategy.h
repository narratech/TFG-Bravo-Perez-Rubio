// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"
#include "ThirdParty/FastNoiseLite.h"

/**
 * 
 */
class COSMICARCHITECTNOISE_API FCosmicDefaultNoiseStrategy : public ICosmicNoiseStrategy
{
public:

	int32 Seed;
	FCosmicNoiseLayer LayerParameters;
	FCosmicNoiseBiomeParameters BiomeParameters;
	
	void Initialize(int32 Seed, FCosmicNoiseLayer LayerParameters, FCosmicNoiseBiomeParameters BiomeParameters);

	void EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor) const override;

protected:
	FastNoiseLite HumidityNoise;
	FastNoiseLite TempNoise;
	FastNoiseLite Noise;
};
