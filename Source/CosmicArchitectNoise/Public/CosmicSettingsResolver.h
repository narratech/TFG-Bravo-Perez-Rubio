// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseTypes.h"

class UCosmicNoiseSettings;

/**
 * 
 */
class COSMICARCHITECTNOISE_API CosmicSettingsResolver
{
public:
    static void Resolve(
        const UCosmicNoiseSettings* Settings,
        TArray<FCosmicNoiseTypes>& OutLayers,
        bool& bOutUseWarp,
        float& OutWarpStrength,
        float& OutWarpFrequency);
};
