// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CosmicNoiseSettings.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTNOISE_API UCosmicNoiseSettings : public UDataAsset
{
	GENERATED_BODY()
	
public:

    /* ---------------- MODE SWITCH ---------------- */

    UPROPERTY(EditAnywhere, Category = "Mode")
    bool bUseAdvancedSettings = false;

    /* ================= SIMPLE MODE ================= */

    UPROPERTY(EditAnywhere, Category = "Simple", meta = (EditCondition = "!bUseAdvancedSettings"))
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

    //Botón “Bake Advanced from Simple” -> generaria advanced con los datos del simple y cambia el modo

        /* ================= ADVANCED MODE ================= */

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    int32 AdvancedSeed = 1337;

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    bool bUseDomainWarp = false;

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    float DomainWarpStrength = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "Advanced", meta = (EditCondition = "bUseAdvancedSettings"))
    float DomainWarpFrequency = 0.001f;

};
