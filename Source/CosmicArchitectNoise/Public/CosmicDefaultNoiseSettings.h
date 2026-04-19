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


	virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const override;
};
