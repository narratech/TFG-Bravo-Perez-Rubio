// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "System/CosmicSystemNoiseManager.h"
#include "CosmicDefaultNoiseSettings.h"
#include "CosmicMultiNoiseSettings.h"
#include "CosmicErosionMultifractalNoiseSettings.h"
#include "CosmicCraterNoiseSettings.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "Math/RandomStream.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#endif

namespace
{
    void ConfigureMultiNoiseForArchetype(
        UCosmicMultiNoiseSettings* Settings,
        FRandomStream& Stream,
        float /*PlanetRadiusKm*/,
        ECosmicPlanetArchetype Archetype,
        bool bHasOcean)
    {
        if (!Settings)
        {
            return;
        }

        Settings->Seed = Stream.RandRange(1, 999999);
        Settings->HeightNormalizationScale = 1.0f;

        // 1. Continentalness Layer (Unit-sphere frequencies calibrated to CosmicEarthNoise in EarthPlanet)
        Settings->ContinentalLayer.Frequency = Stream.FRandRange(0.80f, 1.35f);
        Settings->ContinentalLayer.Octaves = Stream.RandRange(7, 8);
        Settings->ContinentalLayer.Lacunarity = 2.0f;
        Settings->ContinentalLayer.Persistence = 0.5f;
        Settings->ContinentalLayer.Amplitude = Stream.FRandRange(22000.0f, 28000.0f);

        // 2. Erosion Layer
        Settings->ErosionLayer.Frequency = Stream.FRandRange(45.0f, 100.0f);
        Settings->ErosionLayer.Octaves = 6;
        Settings->ErosionLayer.Lacunarity = Stream.FRandRange(1.20f, 1.50f);
        Settings->ErosionLayer.Persistence = 0.5f;
        Settings->ErosionLayer.Amplitude = 2000.0f;

        // 3. Peaks & Valleys Layer (Alpine ridges and deep gorges)
        Settings->PeaksValleysLayer.Frequency = Stream.FRandRange(9.0f, 16.0f);
        Settings->PeaksValleysLayer.Octaves = Stream.RandRange(8, 10);
        Settings->PeaksValleysLayer.Lacunarity = 2.0f;
        Settings->PeaksValleysLayer.Persistence = 0.55f;
        Settings->PeaksValleysLayer.Amplitude = Stream.FRandRange(26000.0f, 35000.0f);

        // 4. Micro Detail Layer
        Settings->DetailLayer.Frequency = Stream.FRandRange(500.0f, 1000.0f);
        Settings->DetailLayer.Octaves = Stream.RandRange(6, 8);
        Settings->DetailLayer.Lacunarity = 2.0f;
        Settings->DetailLayer.Persistence = 0.45f;
        Settings->DetailLayer.Amplitude = Stream.FRandRange(800.0f, 1200.0f);

        // 5. Specialized MultiNoise Parameters
        Settings->MultiNoiseParams.SeaLevelThreshold = bHasOcean
            ? Stream.FRandRange(0.41f, 0.46f)
            : Stream.FRandRange(0.28f, 0.38f);
        Settings->MultiNoiseParams.OceanDepthScale = Stream.FRandRange(2.1f, 2.7f);
        Settings->MultiNoiseParams.ContinentalShelfWidth = Stream.FRandRange(0.08f, 0.11f);
        Settings->MultiNoiseParams.InlandPlateauBoost = Stream.FRandRange(1.1f, 1.45f);
        Settings->MultiNoiseParams.HighErosionFlattening = Stream.FRandRange(0.32f, 0.46f);
        Settings->MultiNoiseParams.ErosionContrast = Stream.FRandRange(2.8f, 3.6f);
        Settings->MultiNoiseParams.MountainPeakSharpness = (Archetype == ECosmicPlanetArchetype::FrozenIce)
            ? Stream.FRandRange(1.65f, 2.25f)
            : Stream.FRandRange(1.30f, 1.75f);
        Settings->MultiNoiseParams.ValleyDepthMultiplier = Stream.FRandRange(1.8f, 2.3f);

        if (Archetype == ECosmicPlanetArchetype::AridDesert)
        {
            Settings->MultiNoiseParams.TerracingStrength = Stream.FRandRange(0.40f, 0.72f);
            Settings->MultiNoiseParams.TerraceSteps = Stream.FRandRange(3.0f, 6.0f);
        }
        else
        {
            Settings->MultiNoiseParams.TerracingStrength = Stream.FRandRange(0.10f, 0.24f);
            Settings->MultiNoiseParams.TerraceSteps = Stream.FRandRange(1.0f, 3.0f);
        }

        // 6. Domain Warping (Inigo Quilez organic continental flow)
        Settings->DomainWarpParams.bUseDomainWarp = true;
        Settings->DomainWarpParams.DomainWarpStrength = Stream.FRandRange(0.20f, 0.30f);
        Settings->DomainWarpParams.DomainWarpFrequency = Stream.FRandRange(1.0f, 1.45f);
        Settings->DomainWarpParams.DomainWarpOctaves = 3;

        // 7. Planetary Climatology & Biomes
        Settings->BiomeParameters.TemperatureFrequency = Stream.FRandRange(3.2f, 5.0f);
        Settings->BiomeParameters.HumidityFrequency = Stream.FRandRange(50.0f, 100.0f);
        Settings->BiomeParameters.HumidityOctaves = 4;
        Settings->BiomeParameters.HumidityContrast = Stream.FRandRange(1.35f, 1.65f);

        if (Archetype == ECosmicPlanetArchetype::AridDesert)
        {
            Settings->BiomeParameters.LatitudeEffect = Stream.FRandRange(0.25f, 0.45f);
            Settings->BiomeParameters.AltitudeTemperaturePenalty = Stream.FRandRange(0.45f, 0.65f);
            Settings->BiomeParameters.HumidityOffset = Stream.FRandRange(-0.30f, -0.12f);
        }
        else if (Archetype == ECosmicPlanetArchetype::FrozenIce)
        {
            Settings->BiomeParameters.LatitudeEffect = Stream.FRandRange(0.65f, 0.95f);
            Settings->BiomeParameters.AltitudeTemperaturePenalty = Stream.FRandRange(0.85f, 1.0f);
            Settings->BiomeParameters.HumidityOffset = Stream.FRandRange(-0.05f, 0.15f);
        }
        else
        {
            // Earth-like balanced climate matching CosmicEarthNoise
            Settings->BiomeParameters.LatitudeEffect = Stream.FRandRange(0.38f, 0.52f);
            Settings->BiomeParameters.AltitudeTemperaturePenalty = Stream.FRandRange(0.80f, 0.94f);
            Settings->BiomeParameters.HumidityOffset = Stream.FRandRange(-0.05f, 0.08f);
        }

        // 8. Orographic Precipitation & Rain Shadow
        Settings->OrographicParams.bEnableOrographicEffect = true;
        Settings->OrographicParams.bUsePlanetaryZonalWinds = true;
        Settings->OrographicParams.PrevailingWindDirection = FVector(1.0f, 0.0f, 0.0f);
        Settings->OrographicParams.OrographicLiftStrength = Stream.FRandRange(0.40f, 0.95f);
        Settings->OrographicParams.RainShadowStrength = Stream.FRandRange(2.5f, 4.2f);
        Settings->OrographicParams.WindwardSampleOffset = Stream.FRandRange(0.08f, 0.14f);
        Settings->OrographicParams.RainShadowDistance = Stream.FRandRange(0.10f, 0.15f);
    }

    void ConfigureErosionMultifractalForArchetype(
        UCosmicErosionMultifractalNoiseSettings* Settings,
        FRandomStream& Stream,
        float /*PlanetRadiusKm*/,
        ECosmicPlanetArchetype Archetype,
        bool /*bHasOcean*/)
    {
        if (!Settings)
        {
            return;
        }

        Settings->Seed = Stream.RandRange(1, 999999);
        Settings->HeightNormalizationScale = 1.0f;

        // 1. Continental Layer (Unit-sphere frequencies)
        Settings->ContinentalLayer.Frequency = Stream.FRandRange(0.85f, 1.40f);
        Settings->ContinentalLayer.Octaves = 6;
        Settings->ContinentalLayer.Lacunarity = 2.0f;
        Settings->ContinentalLayer.Persistence = 0.5f;
        Settings->ContinentalLayer.Amplitude = Stream.FRandRange(34000.0f, 44000.0f);

        // 2. Mountain Layer (Hybrid Ridged Multifractal)
        Settings->MountainLayer.Frequency = Stream.FRandRange(8.0f, 15.0f);
        Settings->MountainLayer.Octaves = 7;
        Settings->MountainLayer.Lacunarity = Stream.FRandRange(2.05f, 2.20f);
        Settings->MountainLayer.Persistence = 0.55f;
        Settings->MountainLayer.Amplitude = Stream.FRandRange(26000.0f, 36000.0f);

        // 3. Hill Layer
        Settings->HillLayer.Frequency = Stream.FRandRange(18.0f, 32.0f);
        Settings->HillLayer.Octaves = 5;
        Settings->HillLayer.Lacunarity = 2.0f;
        Settings->HillLayer.Persistence = 0.48f;
        Settings->HillLayer.Amplitude = Stream.FRandRange(6000.0f, 9500.0f);

        // 4. Micro Detail Layer
        Settings->DetailLayer.Frequency = Stream.FRandRange(400.0f, 850.0f);
        Settings->DetailLayer.Octaves = 5;
        Settings->DetailLayer.Lacunarity = 2.0f;
        Settings->DetailLayer.Persistence = 0.45f;
        Settings->DetailLayer.Amplitude = Stream.FRandRange(850.0f, 1400.0f);

        // 5. Fluvial Cellular River Drainage Layer
        Settings->RiverLayer.Frequency = Stream.FRandRange(4.5f, 8.5f);
        Settings->RiverLayer.Octaves = 1;
        Settings->RiverLayer.Lacunarity = 2.0f;
        Settings->RiverLayer.Persistence = 0.5f;
        Settings->RiverLayer.Amplitude = 2500.0f;

        // 6. Sedimentary Strata & Terraces
        Settings->StrataParameters.bEnableTerraces = true;
        if (Archetype == ECosmicPlanetArchetype::AridDesert)
        {
            // Prominent canyon mesas, buttes, and stepped desert plateaus
            Settings->StrataParameters.TerraceInterval = Stream.FRandRange(1600.0f, 2800.0f);
            Settings->StrataParameters.TerraceSharpness = Stream.FRandRange(0.78f, 0.90f);
            Settings->StrataParameters.TerraceWeight = Stream.FRandRange(0.62f, 0.85f);
            Settings->StrataParameters.TerraceWarpIntensity = Stream.FRandRange(500.0f, 1100.0f);
            Settings->StrataParameters.TerraceWarpFrequency = Stream.FRandRange(2.0f, 4.5f);
        }
        else
        {
            // Natural geological benching on mountain flanks
            Settings->StrataParameters.TerraceInterval = Stream.FRandRange(2200.0f, 3600.0f);
            Settings->StrataParameters.TerraceSharpness = Stream.FRandRange(0.62f, 0.78f);
            Settings->StrataParameters.TerraceWeight = Stream.FRandRange(0.30f, 0.52f);
            Settings->StrataParameters.TerraceWarpIntensity = Stream.FRandRange(400.0f, 850.0f);
            Settings->StrataParameters.TerraceWarpFrequency = Stream.FRandRange(2.5f, 5.0f);
        }

        // 7. Fluvial Drainage & Hydraulic/Thermal Erosion
        Settings->ErosionParameters.bEnableErosion = true;
        Settings->ErosionParameters.FluvialCarveDepth = Stream.FRandRange(1800.0f, 3400.0f);
        Settings->ErosionParameters.FluvialValleyWidth = Stream.FRandRange(0.18f, 0.28f);
        Settings->ErosionParameters.FluvialBankSmoothness = Stream.FRandRange(0.85f, 1.25f);
        Settings->ErosionParameters.DrainageJitter = Stream.FRandRange(0.90f, 0.98f);
        Settings->ErosionParameters.TalusDecayStrength = Stream.FRandRange(0.30f, 0.50f);
        Settings->ErosionParameters.DrainageMoistureConvergence = Stream.FRandRange(0.40f, 0.60f);

        // 8. Heterogeneous Multifractal (Musgrave)
        Settings->MultifractalParameters.PeakSharpness = (Archetype == ECosmicPlanetArchetype::FrozenIce || Archetype == ECosmicPlanetArchetype::VolcanicScorched)
            ? Stream.FRandRange(1.75f, 2.25f)
            : Stream.FRandRange(1.45f, 1.85f);
        Settings->MultifractalParameters.LowlandSmoothingFactor = Stream.FRandRange(0.12f, 0.22f);
        Settings->MultifractalParameters.FractalWeightedStrength = Stream.FRandRange(0.78f, 0.90f);
        Settings->MultifractalParameters.TectonicWarpFrequency = Stream.FRandRange(1.2f, 2.4f);
        Settings->MultifractalParameters.TectonicWarpAmplitude = Stream.FRandRange(0.20f, 0.35f);

        // 9. Climate & Biomes
        Settings->BiomeParameters.TemperatureFrequency = Stream.FRandRange(3.2f, 5.2f);
        Settings->BiomeParameters.HumidityFrequency = Stream.FRandRange(35.0f, 75.0f);
        Settings->BiomeParameters.HumidityOctaves = 4;
        Settings->BiomeParameters.HumidityContrast = Stream.FRandRange(1.35f, 1.60f);

        if (Archetype == ECosmicPlanetArchetype::AridDesert)
        {
            Settings->BiomeParameters.LatitudeEffect = Stream.FRandRange(0.30f, 0.48f);
            Settings->BiomeParameters.AltitudeTemperaturePenalty = Stream.FRandRange(0.45f, 0.65f);
            Settings->BiomeParameters.HumidityOffset = Stream.FRandRange(-0.35f, -0.15f);
        }
        else if (Archetype == ECosmicPlanetArchetype::FrozenIce)
        {
            Settings->BiomeParameters.LatitudeEffect = Stream.FRandRange(0.70f, 0.95f);
            Settings->BiomeParameters.AltitudeTemperaturePenalty = Stream.FRandRange(0.80f, 0.98f);
            Settings->BiomeParameters.HumidityOffset = Stream.FRandRange(-0.05f, 0.15f);
        }
        else
        {
            Settings->BiomeParameters.LatitudeEffect = Stream.FRandRange(0.42f, 0.58f);
            Settings->BiomeParameters.AltitudeTemperaturePenalty = Stream.FRandRange(0.72f, 0.88f);
            Settings->BiomeParameters.HumidityOffset = Stream.FRandRange(-0.08f, 0.08f);
        }
    }

    void ConfigureCraterNoiseForArchetype(
        UCosmicCraterNoiseSettings* Settings,
        FRandomStream& Stream,
        float PlanetRadiusKm,
        ECosmicPlanetArchetype Archetype)
    {
        if (!Settings)
        {
            return;
        }

        Settings->Seed = Stream.RandRange(1, 999999);
        Settings->HeightNormalizationScale = 1.0f;

        const bool bIsMoon = (Archetype == ECosmicPlanetArchetype::Moon);
        const float ScaleFactor = FMath::Clamp(PlanetRadiusKm / 6.0f, 0.45f, 2.0f);

        // 1. Primordial Planetary Highlands & Crust Layer (calibrated to CraterNoise, Moon_Noise, MarsLikeNoise, Calisto_Noise)
        FCosmicNoiseLayer& BaseLayer = Settings->LayerParameters;
        BaseLayer.NoiseType = (Archetype == ECosmicPlanetArchetype::VolcanicScorched || Stream.FRand() < 0.35f)
            ? ECosmicNoiseType::Ridged
            : ECosmicNoiseType::Simplex;
        BaseLayer.FractalType = (Archetype == ECosmicPlanetArchetype::VolcanicScorched)
            ? ECosmicFractalType::Ridged
            : ECosmicFractalType::FBM;
        BaseLayer.Frequency = Stream.FRandRange(2.2f, 5.5f);
        BaseLayer.Octaves = Stream.RandRange(5, 7);
        BaseLayer.Lacunarity = Stream.FRandRange(2.0f, 2.5f);
        BaseLayer.Persistence = 0.5f;
        BaseLayer.Amplitude = bIsMoon
            ? Stream.FRandRange(2500.0f, 6000.0f) * ScaleFactor
            : Stream.FRandRange(5000.0f, 11000.0f) * ScaleFactor;

        // 2. Multi-Scale Poisson Voronoi Impact Craters
        FCosmicNoiseCraterParameters& Crater = Settings->CraterParameters;
        Crater.CraterFrequency = bIsMoon
            ? Stream.FRandRange(3.5f, 8.0f)
            : Stream.FRandRange(3.0f, 6.5f);
        Crater.CraterDepth = bIsMoon
            ? Stream.FRandRange(3500.0f, 8000.0f) * ScaleFactor
            : Stream.FRandRange(6000.0f, 12000.0f) * ScaleFactor;
        Crater.CraterOctaves = Stream.RandRange(4, 6);
        Crater.CraterDensity = Stream.FRandRange(0.48f, 0.62f);
        Crater.CraterRadiusMultiplier = Stream.FRandRange(0.18f, 0.30f);
        Crater.CraterRimHeight = Stream.FRandRange(0.22f, 0.42f);
        Crater.CraterRimSharpness = Stream.FRandRange(6.5f, 14.0f);
        Crater.CraterFloorHeight = Stream.FRandRange(0.38f, 0.80f);
        Crater.CentralPeakHeight = Stream.FRandRange(0.28f, 0.42f);
        Crater.CentralPeakRadius = Stream.FRandRange(0.18f, 0.25f);
        Crater.EjectaStrength = Stream.FRandRange(0.38f, 0.55f);
        Crater.CraterDistortion = Stream.FRandRange(0.40f, 0.75f);
        Crater.CraterLacunarity = Stream.FRandRange(2.0f, 2.6f);
        Crater.CraterPersistence = Stream.FRandRange(0.48f, 0.64f);
        Crater.CraterNoiseBreakup = Stream.FRandRange(0.45f, 0.75f);
        Crater.MariaThreshold = Stream.FRandRange(0.32f, 0.44f);

        // 3. Ridged Mountain Chains (Montes) & Wrinkle Ridges (Dorsa) - relative to BaseLayer.Amplitude
        Settings->RidgeLayer.Frequency = Stream.FRandRange(6.0f, 11.0f);
        Settings->RidgeLayer.Octaves = 6;
        Settings->RidgeLayer.Lacunarity = Stream.FRandRange(2.05f, 2.20f);
        Settings->RidgeLayer.Persistence = 0.52f;
        Settings->RidgeLayer.Amplitude = (Archetype == ECosmicPlanetArchetype::VolcanicScorched)
            ? Stream.FRandRange(0.75f, 1.05f)
            : Stream.FRandRange(0.45f, 0.75f);

        // 4. Heterogeneous Micro-Detail Layer - relative to BaseLayer.Amplitude
        Settings->DetailLayer.Frequency = Stream.FRandRange(60.0f, 140.0f);
        Settings->DetailLayer.Octaves = 4;
        Settings->DetailLayer.Lacunarity = 2.0f;
        Settings->DetailLayer.Persistence = 0.45f;
        Settings->DetailLayer.Amplitude = Stream.FRandRange(0.08f, 0.16f);

        // 5. Geological Terrain Shaping & Crater Weathering
        Settings->TerrainParameters.RidgePeakSharpness = (Archetype == ECosmicPlanetArchetype::VolcanicScorched)
            ? Stream.FRandRange(1.75f, 2.25f)
            : Stream.FRandRange(1.45f, 1.85f);
        Settings->TerrainParameters.RidgeWeightedStrength = Stream.FRandRange(0.72f, 0.88f);
        Settings->TerrainParameters.MariaBasinDepthScale = bIsMoon
            ? Stream.FRandRange(0.50f, 0.80f)
            : Stream.FRandRange(0.40f, 0.65f);
        Settings->TerrainParameters.MariaRoughnessFactor = Stream.FRandRange(0.16f, 0.30f);
        Settings->TerrainParameters.MariaCraterFlooding = bIsMoon
            ? Stream.FRandRange(0.40f, 0.65f)
            : Stream.FRandRange(0.25f, 0.50f);
        Settings->TerrainParameters.CraterFloorFlattening = Stream.FRandRange(0.55f, 0.78f);
        Settings->TerrainParameters.CraterAgeVariation = Stream.FRandRange(0.35f, 0.60f);
        Settings->TerrainParameters.InnerWallTerracing = Stream.FRandRange(0.18f, 0.38f);
        Settings->TerrainParameters.EjectaRayBreakup = Stream.FRandRange(0.45f, 0.70f);
        Settings->TerrainParameters.CraterBiomeInfluence = Stream.FRandRange(0.10f, 0.20f);

        // 6. Crustal Domain Warping
        Settings->DomainWarpParams.bUseDomainWarp = true;
        Settings->DomainWarpParams.DomainWarpStrength = Stream.FRandRange(0.18f, 0.32f);
        Settings->DomainWarpParams.DomainWarpFrequency = Stream.FRandRange(1.0f, 1.8f);
        Settings->DomainWarpParams.DomainWarpOctaves = 3;

        // 7. Planetary Climatology & Biomes (drives vertex color G = Temp, B = Humidity across the globe)
        Settings->BiomeParameters.TemperatureFrequency = Stream.FRandRange(2.8f, 4.8f);
        Settings->BiomeParameters.LatitudeEffect = Stream.FRandRange(0.42f, 0.68f);
        Settings->BiomeParameters.AltitudeTemperaturePenalty = Stream.FRandRange(0.45f, 0.68f);
        Settings->BiomeParameters.HumidityFrequency = Stream.FRandRange(8.0f, 20.0f);
        Settings->BiomeParameters.HumidityOctaves = 4;
        Settings->BiomeParameters.HumidityContrast = Stream.FRandRange(1.30f, 1.65f);
        Settings->BiomeParameters.HumidityOffset = (Archetype == ECosmicPlanetArchetype::VolcanicScorched)
            ? Stream.FRandRange(-0.20f, -0.05f)
            : Stream.FRandRange(-0.10f, 0.10f);
    }
}

void FCosmicSystemNoiseManager::Reset()
{
    GeneratedNoiseAssetCounter = 0;
#if WITH_EDITOR
    GeneratedNoiseSettingsAssets.Empty();
#endif
}

#if WITH_EDITOR
void FCosmicSystemNoiseManager::OnPostDuplicate(FString& InOutFolderId)
{
    InOutFolderId.Empty();
    GeneratedNoiseSettingsAssets.Empty();
}

void FCosmicSystemNoiseManager::SanitizeObjectName(FString& Name)
{
    for (TCHAR& Character : Name)
    {
        if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
        {
            Character = TEXT('_');
        }
    }
}

void FCosmicSystemNoiseManager::EnsureGeneratedNoiseSettingsFolderId(
    FString& InOutFolderId,
    const FString& ActorName,
    UPackage* PackageToDirty)
{
    if (!InOutFolderId.IsEmpty())
    {
        return;
    }

    FString ActorPart = ActorName;
    const FString GuidPart = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    SanitizeObjectName(ActorPart);
    InOutFolderId = FString::Printf(TEXT("%s_%s"), *ActorPart, *GuidPart);
    SanitizeObjectName(InOutFolderId);

    if (PackageToDirty)
    {
        PackageToDirty->MarkPackageDirty();
    }
}

FString FCosmicSystemNoiseManager::GetGeneratedNoiseSettingsFolder(
    const FString& FolderId,
    const FString& BaseAssetFolder)
{
    FString Folder = BaseAssetFolder;
    Folder.TrimStartAndEndInline();

    if (Folder.IsEmpty())
    {
        Folder = TEXT("/Game/CosmicArchitect/GeneratedNoiseSettings");
    }
    else if (!Folder.StartsWith(TEXT("/")))
    {
        Folder = FString::Printf(TEXT("/Game/%s"), *Folder);
    }

    while (Folder.EndsWith(TEXT("/")))
    {
        Folder.LeftChopInline(1);
    }

    if (!FPackageName::IsValidLongPackageName(Folder, true))
    {
        UE_LOG(LogTemp, Warning, TEXT("CosmicSystemNoiseManager: invalid noise asset folder '%s'. Using transient noise settings."), *Folder);
        return FString();
    }

    if (FolderId.IsEmpty())
    {
        return FString();
    }

    return FString::Printf(TEXT("%s/%s"), *Folder, *FolderId);
}

FString FCosmicSystemNoiseManager::MakeNoiseAssetName(int32 AssetIndex)
{
    return FString::Printf(TEXT("Noise_%03d"), AssetIndex);
}

void FCosmicSystemNoiseManager::LoadGeneratedNoiseSettingsAssets(
    const FString& FolderId,
    const FString& BaseAssetFolder)
{
    GeneratedNoiseSettingsAssets.Empty();

    const FString Folder = GetGeneratedNoiseSettingsFolder(FolderId, BaseAssetFolder);
    if (Folder.IsEmpty())
    {
        return;
    }

    FARFilter Filter;
    Filter.PackagePaths.Add(*Folder);
    Filter.ClassPaths.Add(UCosmicNoiseClass::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = false;

    TArray<FAssetData> AssetDataList;
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        const FString AssetName = AssetData.AssetName.ToString();
        if (!AssetName.StartsWith(TEXT("Noise_")))
        {
            continue;
        }

        int32 AssetIndex = INDEX_NONE;
        if (!LexTryParseString(AssetIndex, *AssetName.RightChop(6)) || AssetIndex < 0)
        {
            continue;
        }

        UCosmicNoiseClass* NoiseSettings = Cast<UCosmicNoiseClass>(AssetData.GetAsset());
        if (!NoiseSettings)
        {
            continue;
        }

        if (GeneratedNoiseSettingsAssets.Num() <= AssetIndex)
        {
            GeneratedNoiseSettingsAssets.SetNum(AssetIndex + 1);
        }

        GeneratedNoiseSettingsAssets[AssetIndex] = NoiseSettings;
    }
}

void FCosmicSystemNoiseManager::SaveGeneratedNoiseSettingsAsset(UCosmicNoiseClass* NoiseSettings)
{
    if (!NoiseSettings || !NoiseSettings->IsAsset())
    {
        return;
    }

    NoiseSettings->MarkPackageDirty();
    TArray<UPackage*> PackagesToSave;
    PackagesToSave.Add(NoiseSettings->GetOutermost());
    UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);
}

UCosmicNoiseClass* FCosmicSystemNoiseManager::CreateOrReusePersistentNoiseSettingsAsset(
    const FString& FolderId,
    const FString& BaseAssetFolder,
    TSubclassOf<UCosmicNoiseClass> SettingsClass)
{
    if (!SettingsClass)
    {
        SettingsClass = UCosmicMultiNoiseSettings::StaticClass();
    }

    const int32 AssetIndex = GeneratedNoiseAssetCounter++;

    if (GeneratedNoiseSettingsAssets.IsValidIndex(AssetIndex) &&
        GeneratedNoiseSettingsAssets[AssetIndex] &&
        GeneratedNoiseSettingsAssets[AssetIndex]->GetClass() == SettingsClass)
    {
        return GeneratedNoiseSettingsAssets[AssetIndex].Get();
    }

    const FString Folder = GetGeneratedNoiseSettingsFolder(FolderId, BaseAssetFolder);
    if (Folder.IsEmpty())
    {
        return nullptr;
    }

    const FString AssetName = MakeNoiseAssetName(AssetIndex);
    const FString PackageName = FString::Printf(TEXT("%s/%s"), *Folder, *AssetName);
    const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);

    if (UCosmicNoiseClass* ExistingLoaded = Cast<UCosmicNoiseClass>(StaticLoadObject(UCosmicNoiseClass::StaticClass(), nullptr, *ObjectPath)))
    {
        if (ExistingLoaded->GetClass() == SettingsClass)
        {
            if (GeneratedNoiseSettingsAssets.Num() <= AssetIndex)
            {
                GeneratedNoiseSettingsAssets.SetNum(AssetIndex + 1);
            }
            GeneratedNoiseSettingsAssets[AssetIndex] = ExistingLoaded;
            return ExistingLoaded;
        }
    }

    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        return nullptr;
    }

    Package->FullyLoad();

    if (UObject* ExistingObject = StaticFindObjectFast(UObject::StaticClass(), Package, *AssetName))
    {
        if (ExistingObject->GetClass() == SettingsClass)
        {
            if (UCosmicNoiseClass* MatchingSettings = Cast<UCosmicNoiseClass>(ExistingObject))
            {
                if (GeneratedNoiseSettingsAssets.Num() <= AssetIndex)
                {
                    GeneratedNoiseSettingsAssets.SetNum(AssetIndex + 1);
                }
                GeneratedNoiseSettingsAssets[AssetIndex] = MatchingSettings;
                return MatchingSettings;
            }
        }

        // Move mismatched previous noise class out of this package so the new strategy class can take its place cleanly
        const FName UniqueTrashName = MakeUniqueObjectName(GetTransientPackage(), ExistingObject->GetClass(), TEXT("ReplacedCosmicNoise"));
        ExistingObject->ClearFlags(RF_Public | RF_Standalone);
        ExistingObject->Rename(*UniqueTrashName.ToString(), GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);
    }

    UCosmicNoiseClass* NewAsset = NewObject<UCosmicNoiseClass>(
        Package,
        SettingsClass,
        *AssetName,
        RF_Public | RF_Standalone | RF_Transactional);

    if (NewAsset)
    {
        FAssetRegistryModule::AssetCreated(NewAsset);
        if (GeneratedNoiseSettingsAssets.Num() <= AssetIndex)
        {
            GeneratedNoiseSettingsAssets.SetNum(AssetIndex + 1);
        }
        GeneratedNoiseSettingsAssets[AssetIndex] = NewAsset;
    }

    return NewAsset;
}
#endif

UCosmicNoiseClass* FCosmicSystemNoiseManager::CreateRandomNoiseSettings(
    FRandomStream& Stream,
    float PlanetRadius,
    UWorld* World,
    const FString& FolderId,
    const FString& BaseAssetFolder,
    bool bSavePersistentAssets)
{
    return CreateNoiseSettingsForArchetype(
        Stream,
        PlanetRadius,
        ECosmicPlanetArchetype::EarthLike,
        true,
        World,
        FolderId,
        BaseAssetFolder,
        bSavePersistentAssets
    );
}

UCosmicNoiseClass* FCosmicSystemNoiseManager::CreateNoiseSettingsForArchetype(
    FRandomStream& Stream,
    float PlanetRadius,
    ECosmicPlanetArchetype Archetype,
    bool bHasOcean,
    UWorld* World,
    const FString& FolderId,
    const FString& BaseAssetFolder,
    bool bSavePersistentAssets)
{
    // Choose the optimal noise strategy class for this planetary archetype:
    // - EarthLike: 65% MultiNoise (CosmicEarthNoise style), 35% ErosionMultifractal
    // - AridDesert: 70% ErosionMultifractal (stepped canyon mesas & river wadis), 30% MultiNoise
    // - FrozenIce: 55% MultiNoise (glacial alpine peaks & U-valleys), 45% ErosionMultifractal
    // - CrateredBarren & Moon: 100% CraterNoise (Poisson Voronoi multi-scale craters, central rebound peaks, maria basins)
    // - VolcanicScorched: 60% CraterNoise (impact basins + ridged crust), 40% ErosionMultifractal (tectonic ridges & lava terraces)
    TSubclassOf<UCosmicNoiseClass> ChosenClass = UCosmicMultiNoiseSettings::StaticClass();
    const float StrategyRoll = Stream.FRandRange(0.0f, 1.0f);

    switch (Archetype)
    {
    case ECosmicPlanetArchetype::EarthLike:
        ChosenClass = (StrategyRoll < 0.65f)
            ? TSubclassOf<UCosmicNoiseClass>(UCosmicMultiNoiseSettings::StaticClass())
            : TSubclassOf<UCosmicNoiseClass>(UCosmicErosionMultifractalNoiseSettings::StaticClass());
        break;

    case ECosmicPlanetArchetype::AridDesert:
        ChosenClass = (StrategyRoll < 0.70f)
            ? TSubclassOf<UCosmicNoiseClass>(UCosmicErosionMultifractalNoiseSettings::StaticClass())
            : TSubclassOf<UCosmicNoiseClass>(UCosmicMultiNoiseSettings::StaticClass());
        break;

    case ECosmicPlanetArchetype::FrozenIce:
        ChosenClass = (StrategyRoll < 0.55f)
            ? TSubclassOf<UCosmicNoiseClass>(UCosmicMultiNoiseSettings::StaticClass())
            : TSubclassOf<UCosmicNoiseClass>(UCosmicErosionMultifractalNoiseSettings::StaticClass());
        break;

    case ECosmicPlanetArchetype::VolcanicScorched:
        ChosenClass = (StrategyRoll < 0.60f)
            ? TSubclassOf<UCosmicNoiseClass>(UCosmicCraterNoiseSettings::StaticClass())
            : TSubclassOf<UCosmicNoiseClass>(UCosmicErosionMultifractalNoiseSettings::StaticClass());
        break;

    case ECosmicPlanetArchetype::CrateredBarren:
    case ECosmicPlanetArchetype::Moon:
    default:
        ChosenClass = UCosmicCraterNoiseSettings::StaticClass();
        break;
    }

    UCosmicNoiseClass* NoiseSettings = nullptr;

#if WITH_EDITOR
    const bool bShouldPersist = bSavePersistentAssets
        && World
        && World->WorldType == EWorldType::Editor;

    if (bShouldPersist)
    {
        NoiseSettings = CreateOrReusePersistentNoiseSettingsAsset(FolderId, BaseAssetFolder, ChosenClass);
    }
#endif

    if (!NoiseSettings || NoiseSettings->GetClass() != ChosenClass)
    {
        NoiseSettings = NewObject<UCosmicNoiseClass>(GetTransientPackage(), ChosenClass);
    }

    if (UCosmicMultiNoiseSettings* MultiSettings = Cast<UCosmicMultiNoiseSettings>(NoiseSettings))
    {
        ConfigureMultiNoiseForArchetype(MultiSettings, Stream, PlanetRadius, Archetype, bHasOcean);
    }
    else if (UCosmicErosionMultifractalNoiseSettings* ErosionSettings = Cast<UCosmicErosionMultifractalNoiseSettings>(NoiseSettings))
    {
        ConfigureErosionMultifractalForArchetype(ErosionSettings, Stream, PlanetRadius, Archetype, bHasOcean);
    }
    else if (UCosmicCraterNoiseSettings* CraterSettings = Cast<UCosmicCraterNoiseSettings>(NoiseSettings))
    {
        ConfigureCraterNoiseForArchetype(CraterSettings, Stream, PlanetRadius, Archetype);
    }

#if WITH_EDITOR
    SaveGeneratedNoiseSettingsAsset(NoiseSettings);
#endif

    return NoiseSettings;
}
