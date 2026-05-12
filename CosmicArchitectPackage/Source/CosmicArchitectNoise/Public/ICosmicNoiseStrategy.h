// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseTypes.h"

/**
 * 
 */
class COSMICARCHITECTNOISE_API ICosmicNoiseStrategy
{
public:
    virtual ~ICosmicNoiseStrategy() = default;

    virtual void EvaluatePoint(
        const FVector& NoiseDir,
        float& OutHeight,
        FLinearColor& OutColor
    ) const = 0;
};
