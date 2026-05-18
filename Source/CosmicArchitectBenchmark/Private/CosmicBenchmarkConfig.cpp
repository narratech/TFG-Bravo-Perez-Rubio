// Fill out your copyright notice in the Description page of Project Settings.

#include "CosmicBenchMarkConfig.h"
#include "CosmicFoliageCollection.h"
#include "Materials/MaterialInstance.h"

ACosmicBenchmarkConfig::ACosmicBenchmarkConfig()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ACosmicBenchmarkConfig::BeginPlay()
{
    Super::BeginPlay();

    UCosmicBenchmarkManager* BM = UCosmicBenchmarkManager::Get(GetWorld());
    if (!BM)
    {
        UE_LOG(LogTemp, Error, TEXT("ABenchMarkConfig: No se encontró el BenchmarkManager."));
        return;
    }

    // 1 — Assets
    BM->InitializeAssets(
        BaseMaterial, MoonMaterial, OceanMaterial,
        StarMaterial, GasGiantMaterial, RingMaterial,
        NoiseClass, FoliageCollection
    );

    // 2 — Configuración de planeta
    BM->SetPlanetConfig(BuildPlanetConfig());

    UE_LOG(LogTemp, Log, TEXT("ABenchMarkConfig: Configuración enviada al BenchmarkManager."));
    UE_LOG(LogTemp, Log, TEXT("  Radius=%.1f km | Ocean=%s | Foliage=%s | Capture=%.1fs"),
        PlanetRadiusKm,
        bHasOcean ? TEXT("ON") : TEXT("OFF"),
        bUseFoliageByDefault ? TEXT("ON") : TEXT("OFF"),
        CaptureDurationSeconds);
}

FBenchmarkPlanetConfig ACosmicBenchmarkConfig::BuildPlanetConfig() const
{
    FBenchmarkPlanetConfig Config;

    Config.RadiusKm = PlanetRadiusKm;

    Config.SpawnCenter = SpawnCenter;
    Config.SpawnSpacingCm = SpawnSpacingCm;
    Config.NearSpawnRadiusCm = NearSpawnRadiusCm;
    Config.FarSpawnSpacingCm = FarSpawnSpacingCm;

    Config.bHasOcean = bHasOcean;
    Config.OceanSeaLevel = OceanSeaLevel;
    Config.OceanClipmapResolution = OceanClipmapResolution;

    Config.bUseFoliageByDefault = bUseFoliageByDefault;

    Config.CaptureDurationSeconds = CaptureDurationSeconds;
    Config.StabilizationDelaySeconds = StabilizationDelaySeconds;

    return Config;
}
