// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CosmicBenchmarkManager.h"          // FBenchmarkPlanetConfig
#include "CosmicBenchMarkConfig.generated.h"

class UCosmicNoiseClass;
class UCosmicFoliageCollection;
class UMaterialInstance;

/**
 *  Actor de configuración del Benchmark.
 *  Colócalo en el nivel y rellena sus propiedades desde el editor.
 *  En BeginPlay enviará toda la configuración al BenchmarkManager automáticamente. 
 */
UCLASS(ClassGroup = (CosmicArchitect), meta = (BlueprintSpawnableComponent),
    HideCategories = (Rendering, Lighting, Navigation, Replication, Physics, Collision,
        Activation, AssetUserData, HLOD, Cooking, Tags, ComponentReplication))
    class COSMICARCHITECTBENCHMARK_API ACosmicBenchmarkConfig : public AActor
{
    GENERATED_BODY()

public:
    ACosmicBenchmarkConfig();

    // 
    //  MATERIALES Y ASSETS
    // 
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    UMaterialInstance* BaseMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    UMaterialInstance* MoonMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    UMaterialInstance* OceanMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    UMaterialInstance* StarMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    UMaterialInstance* GasGiantMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    UMaterialInstance* RingMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    UCosmicNoiseClass* NoiseClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
    UCosmicFoliageCollection* FoliageCollection;

    // 
    //  GEOMETRÍA DEL PLANETA
    // 
    /** Radio del planeta en Km para todos los tests */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Geometry",
        meta = (ClampMin = "0.1", ClampMax = "50000.0",
            ToolTip = "Radio del planeta en Km. Afecta a todos los tests de planeta."))
    float PlanetRadiusKm = 10.0f;

    // 
    //  SPAWN
    // 
    /** Origen del mundo desde el que se calculan todas las posiciones de spawn */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Spawn",
        meta = (ToolTip = "Centro del mundo para los tests. Por defecto (0,0,0)."))
    FVector SpawnCenter = FVector::ZeroVector;

    /** Separación entre planetas en el test estándar (cm, eje X) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Spawn",
        meta = (ClampMin = "10000.0",
            ToolTip = "Separación entre planetas en SpawnPlanets. 500000 = 5 km UE."))
    float SpawnSpacingCm = 500000.0f;

    /** Radio del anillo de spawn en SpawnPlanetsNear (cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Spawn",
        meta = (ClampMin = "10000.0",
            ToolTip = "Radio del círculo de spawn alrededor de la cámara en SpawnPlanetsNear."))
    float NearSpawnRadiusCm = 1005000.0f;

    /** Separación entre planetas en SpawnPlanetsFar (cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Spawn",
        meta = (ClampMin = "10000.0"))
    float FarSpawnSpacingCm = 5000000.0f;

    // 
    //  OCÉANO
    // 
    /** ¿Los planetas generados en los tests tendrán océano? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Ocean")
    bool bHasOcean = true;

    /** Nivel del mar (0 = nivel base del planeta) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Ocean",
        meta = (EditCondition = "bHasOcean"))
    float OceanSeaLevel = 0.0f;

    /** Resolución del clipmap del océano */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Ocean",
        meta = (EditCondition = "bHasOcean", ClampMin = "16", ClampMax = "512"))
    int32 OceanClipmapResolution = 128;

    // 
    //  FOLIAJE
    // 
    /** ¿Los planetas en SpawnPlanets usarán foliaje por defecto? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Foliage")
    bool bUseFoliageByDefault = false;

    // 
    //  CAPTURA
    // 
    /** Duración de captura por paso en los tests secuenciales */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Benchmark|Capture",
        meta = (ClampMin = "1.0", ClampMax = "120.0",
            ToolTip = "Segundos de captura por paso. Más tiempo = datos más precisos."))
    float CaptureDurationSeconds = 8.0f;

    /** Pausa de estabilización entre pasos (deja que el motor se asiente) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Benchmark|Capture",
        meta = (ClampMin = "0.5", ClampMax = "30.0"))
    float StabilizationDelaySeconds = 1.5f;

protected:
    virtual void BeginPlay() override;

private:
    /** Construye un FBenchmarkPlanetConfig a partir de las propiedades del actor */
    FBenchmarkPlanetConfig BuildPlanetConfig() const;
};
