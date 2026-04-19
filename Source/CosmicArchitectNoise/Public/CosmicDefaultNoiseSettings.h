// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "CosmicDefaultNoiseSettings.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicDefaultNoiseSettings : public UCosmicNoiseClass
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	int32 Seed;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	FCosmicNoiseLayer LayerParameters;

	UPROPERTY(EditAnywhere, Category = "Noise Settings")
	FCosmicNoiseBiomeParameters BiomeParameters;

	virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};
