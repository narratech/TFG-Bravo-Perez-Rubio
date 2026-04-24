// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicCraterNoiseSettings.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicCraterNoiseSettings : public UCosmicNoiseClass
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	int32 Seed;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	FCosmicNoiseLayer LayerParameters;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	FCosmicNoiseBiomeParameters BiomeParameters;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	FCosmicNoiseCraterParameters CraterParameters;

	virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};
