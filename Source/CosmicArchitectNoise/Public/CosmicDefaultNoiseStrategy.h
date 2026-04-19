// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ICosmicNoiseStrategy.h"

/**
 * 
 */
class COSMICARCHITECTNOISE_API FCosmicDefaultNoiseStrategy : public ICosmicNoiseStrategy
{
public:


	
	// Heredado vía ICosmicNoiseStrategy
	void Initialize() override;

	void EvaluatePoint(const FVector& NoiseDir, float& OutHeight, FLinearColor& OutColor) const override;

};
