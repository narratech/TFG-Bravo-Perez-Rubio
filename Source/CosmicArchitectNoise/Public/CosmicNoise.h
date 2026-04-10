// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseSettings.h"

/**
 * 
 */
class COSMICARCHITECTNOISE_API CosmicNoise
{
public:
    static TArray<float> CalculateHeights(const TArray<FVector>& Points, const FVector& PlanetCenter, const FTransform& ComponentTransform, UCosmicNoiseSettings* Settings);
    static TArray<float> CalculateHeightsDirect(const TArray<FVector>& Points, UCosmicNoiseSettings* Settings);
};
