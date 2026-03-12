// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UCosmicNoiseSettings;

/**
 * 
 */
class COSMICARCHITECTNOISE_API CosmicNoise
{
public:
    static TArray<float> CalculateHeights(const TArray<FVector>& Points, const FVector& PlanetCenter, const FTransform& ComponentTransform, const UCosmicNoiseSettings* Settings);
};
