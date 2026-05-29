// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.

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
 