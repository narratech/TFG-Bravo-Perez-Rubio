// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CosmicNoiseTypes.h"
#include "CosmicNoiseSettings.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class COSMICARCHITECTNOISE_API UCosmicNoiseSettings : public UDataAsset
{
	GENERATED_BODY()
	
public:

    UCosmicNoiseSettings();

    /* PUBLIC METHODS */
    /** Convierte los parámetros simples a capas avanzadas */
    UFUNCTION(BlueprintCallable, Category = "Noise")
    void UpdateAdvancedFromSimple();

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    /* MODE SWITCH */

    UPROPERTY(EditAnywhere, Category = "Mode")
    bool bUseAdvancedSettings = false;

    UPROPERTY(EditAnywhere, Category = "Simple")
    bool bIsCraterPlanet = false;

    /* SIMPLE MODE */

    UPROPERTY(EditAnywhere, Category = "Simple")
    int32 Seed = 1337;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0"))
    float MaxMountainHeight = 3000.0f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Mountainous = 0.6f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Roughness = 0.4f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Detail = 0.7f;

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings", ClampMin = "0", ClampMax = "1"))
    float Smoothness = 0.5f;

        /* ADVANCED MODE */

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    TArray<FCosmicNoiseTypes> NoiseLayers;

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    bool bUseDomainWarp = false;

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    float DomainWarpStrength = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    float DomainWarpFrequency = 0.001f;

private:
    /** Flag para prevenir recursion en el editor */
    bool bIsUpdatingAdvanced = false;
};
