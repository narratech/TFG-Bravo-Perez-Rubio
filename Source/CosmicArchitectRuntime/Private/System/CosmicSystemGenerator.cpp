// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "System/CosmicSystemGenerator.h"
#include "System/CosmicSystemLayoutManager.h"
#include "System/CosmicCelestialBodySpawner.h"
#include "System/CosmicSystemOrbitController.h"
#include "Planet/CosmicPlanet.h"
#include "Planet/CosmicRingComponent.h"
#include "Engine/World.h"
#include "Engine/PointLight.h"
#include "Materials/MaterialInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"

ACosmicSystemGenerator::ACosmicSystemGenerator()
{
    PrimaryActorTick.bCanEverTick = true;
#if !WITH_EDITOR
    PrimaryActorTick.bStartWithTickEnabled = false;
#endif

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // Auto-load default Moon material (MI_CosmicMoonV2)
    static ConstructorHelpers::FObjectFinderOptional<UMaterialInstance> MoonMatV2Finder(
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicMoonV2.MI_CosmicMoonV2"));
    if (MoonMatV2Finder.Succeeded())
    {
        MoonMaterial = MoonMatV2Finder.Get();
    }
    else
    {
        static ConstructorHelpers::FObjectFinderOptional<UMaterialInstance> MoonMatFinder(
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicMoon.MI_CosmicMoon"));
        if (MoonMatFinder.Succeeded())
        {
            MoonMaterial = MoonMatFinder.Get();
        }
    }

    // Auto-load default Earth / Base material (MI_CosmicEarthV2 or MI_CosmicEarth2)
    static ConstructorHelpers::FObjectFinderOptional<UMaterialInstance> EarthMatV2Finder(
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicEarthV2.MI_CosmicEarthV2"));
    if (EarthMatV2Finder.Succeeded())
    {
        BaseMaterial = EarthMatV2Finder.Get();
    }
    else
    {
        static ConstructorHelpers::FObjectFinderOptional<UMaterialInstance> EarthMat2Finder(
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicEarth2.MI_CosmicEarth2"));
        if (EarthMat2Finder.Succeeded())
        {
            BaseMaterial = EarthMat2Finder.Get();
        }
    }

    // Auto-load Star material
    static ConstructorHelpers::FObjectFinderOptional<UMaterialInstance> StarMatFinder(
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicSun.MI_CosmicSun"));
    if (StarMatFinder.Succeeded())
    {
        StarMaterial = StarMatFinder.Get();
    }

    // Auto-load Gas Giant material
    static ConstructorHelpers::FObjectFinderOptional<UMaterialInstance> GasGiantMatFinder(
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicGasGiant.MI_CosmicGasGiant"));
    if (GasGiantMatFinder.Succeeded())
    {
        GasGiantMaterial = GasGiantMatFinder.Get();
    }

    // Auto-load Ring material
    static ConstructorHelpers::FObjectFinderOptional<UMaterialInstance> RingMatFinder(
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicRing.MI_CosmicRing"));
    if (RingMatFinder.Succeeded())
    {
        RingMaterial = RingMatFinder.Get();
    }
}

void ACosmicSystemGenerator::EnsureDefaultMaterials()
{
    if (!MoonMaterial)
    {
        MoonMaterial = FCosmicCelestialBodySpawner::GetDefaultMoonMaterial();
    }

    if (!BaseMaterial)
    {
        BaseMaterial = FCosmicCelestialBodySpawner::GetDefaultEarthMaterial();
    }

    if (!StarMaterial)
    {
        StarMaterial = FCosmicCelestialBodySpawner::GetDefaultStarMaterial();
    }

    if (!GasGiantMaterial)
    {
        GasGiantMaterial = FCosmicCelestialBodySpawner::GetDefaultGasGiantMaterial();
    }

    if (!RingMaterial)
    {
        RingMaterial = FCosmicCelestialBodySpawner::GetDefaultRingMaterial();
    }

    // Note: OceanMaterial is deliberately not required or forced here,
    // as the ocean subsystem automatically resolves MI_CosmicOceanV3/MI_CosmicOceanV2.
}

void ACosmicSystemGenerator::PostLoad()
{
    Super::PostLoad();
    EnsureDefaultMaterials();
}

void ACosmicSystemGenerator::PostActorCreated()
{
    Super::PostActorCreated();
    EnsureDefaultMaterials();
}

void ACosmicSystemGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    EnsureDefaultMaterials();
}

#if WITH_EDITOR
void ACosmicSystemGenerator::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World || World->WorldType != EWorldType::Editor)
    {
        return;
    }

    DrawDebugBox(
        World,
        GetActorLocation(),
        (VolumeSizeKm * 100000.0f) * 0.5f,
        BoxColor,
        false,
        DeltaTime * 2.0f,
        0,
        LineWidth
    );

    if (bIsSimulatingOrbits)
    {
        FCosmicSystemOrbitController::TickSimulation(GeneratedBodies, OrbitSpeedMultiplier);
    }
}

void ACosmicSystemGenerator::PostDuplicate(EDuplicateMode::Type Mode)
{
    Super::PostDuplicate(Mode);

#if WITH_EDITORONLY_DATA
    NoiseManager.OnPostDuplicate(GeneratedNoiseSettingsFolderId);
#endif
}
#endif

FCosmicSystemLayoutConfig ACosmicSystemGenerator::BuildLayoutConfig() const
{
    FCosmicSystemLayoutConfig Config;
    Config.VolumeSizeKm = VolumeSizeKm;
    Config.BodyDiameterRangeKm = BodyDiameterRangeKm;
    Config.MoonDiameterRangeKm = MoonDiameterRangeKm;
    Config.PlanetSurfaceGravityRange = PlanetSurfaceGravityRange;
    Config.MoonSurfaceGravityRange = MoonSurfaceGravityRange;
    Config.StarRadiusFraction = StarRadiusFraction;
    Config.MinDistanceBetweenBodies = MinDistanceBetweenBodies;
    Config.MaxDistanceToNearest = MaxDistanceToNearest;
    Config.MaxGenerationAttempts = MaxGenerationAttempts;
    Config.OrbitDistanceMinFactor = OrbitDistanceMinFactor;
    Config.PlanetRadiusFactorMin = PlanetRadiusFactorMin;
    Config.PlanetRadiusFactorMax = PlanetRadiusFactorMax;
    Config.MoonOrbitDistanceFactorMin = MoonOrbitDistanceFactorMin;
    Config.MoonOrbitDistanceFactorMax = MoonOrbitDistanceFactorMax;
    Config.MoonRadiusFactorMin = MoonRadiusFactorMin;
    Config.MoonRadiusFactorMax = MoonRadiusFactorMax;
    return Config;
}

FCosmicSystemClassificationRules ACosmicSystemGenerator::BuildClassificationRules() const
{
    FCosmicSystemClassificationRules Rules;
    Rules.GasGiantAppearanceThreshold = GasGiantAppearanceThreshold;
    Rules.GasGiantProbability = GasGiantProbability;
    Rules.GasGiantRadiusFactorMin = GasGiantRadiusFactorMin;
    Rules.GasGiantRadiusFactorMax = GasGiantRadiusFactorMax;
    Rules.GasGiantRadiusMin = GasGiantRadiusMin;
    Rules.GasGiantRadiusMax = GasGiantRadiusMax;
    Rules.HabitableZoneInnerFraction = HabitableZoneInnerFraction;
    Rules.HabitableZoneOuterFraction = HabitableZoneOuterFraction;
    Rules.BeltZoneInnerFraction = BeltZoneInnerFraction;
    Rules.BeltZoneOuterFraction = BeltZoneOuterFraction;
    Rules.BeltProbability = BeltProbability;
    Rules.GasGiantRingProbability = GasGiantRingProbability;
    Rules.TelluricOceanProbability = TelluricOceanProbability;
    Rules.GasGiantMoonMin = GasGiantMoonMin;
    Rules.GasGiantMoonMax = GasGiantMoonMax;
    Rules.TelluricMoonMin = TelluricMoonMin;
    Rules.TelluricMoonMax = TelluricMoonMax;
    return Rules;
}

FCosmicSystemGraphicsConfig ACosmicSystemGenerator::BuildGraphicsConfig() const
{
    FCosmicSystemGraphicsConfig Config;
    Config.GasGiantClipResolution = GasGiantClipResolution;
    Config.TelluricClipResolution = TelluricClipResolution;
    Config.OceanResolutionWithOcean = OceanResolutionWithOcean;
    Config.OceanResolutionWithoutOcean = OceanResolutionWithoutOcean;
    return Config;
}

void ACosmicSystemGenerator::SetNumBodies(int32 NumBodies)
{
    NumberOfBodies = FMath::Clamp(NumBodies, 1, 200);
}

void ACosmicSystemGenerator::GenerateBodies()
{
    ClearBodies();
    EnsureDefaultMaterials();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    NoiseManager.Reset();

#if WITH_EDITOR
    const bool bShouldPersist = bSaveGeneratedNoiseSettingsAssets
        && World->WorldType == EWorldType::Editor
        && !IsTemplate();

    if (bShouldPersist)
    {
        FCosmicSystemNoiseManager::EnsureGeneratedNoiseSettingsFolderId(
            GeneratedNoiseSettingsFolderId, GetName(), GetPackage());
        NoiseManager.LoadGeneratedNoiseSettingsAssets(
            GeneratedNoiseSettingsFolderId, GeneratedNoiseSettingsAssetFolder);
    }
#endif

    FRandomStream Stream(Seed);

    const FCosmicSystemLayoutConfig LayoutConfig = BuildLayoutConfig();
    const FCosmicSystemClassificationRules ClassRules = BuildClassificationRules();
    const FCosmicSystemGraphicsConfig GraphicsConfig = BuildGraphicsConfig();

    const float SystemRadiusKm = VolumeSizeKm.X * 0.5f;
    const float StarRadiusKm = SystemRadiusKm * StarRadiusFraction;

    // --- STAR ---
    ACosmicPlanet* Star = FCosmicCelestialBodySpawner::SpawnStar(
        World, this, GetActorLocation(), StarRadiusKm, StarMaterial, Stream);
    if (!Star)
    {
        return;
    }
    GeneratedBodies.Add(Star);

    // --- STAR POINT LIGHT ---
    const float MaxBodyRadiusKm = FMath::Max3(
        (float)BodyDiameterRangeKm.Y * 0.5f,
        (float)MoonDiameterRangeKm.Y * 0.5f,
        GasGiantRadiusMax
    );

    if (APointLight* PointLight = FCosmicCelestialBodySpawner::SpawnStarLight(
        World, Star, SystemRadiusKm, MaxBodyRadiusKm, MoonOrbitDistanceFactorMax, MoonDiameterRangeKm.Y))
    {
        GeneratedBodies.Add(PointLight);
    }

    // --- PLANETS & MOONS ---
    int32 BodiesSpawned = 0;
    TArray<float> PlanetOrbits;
    TArray<float> PlanetRadii;

    while (BodiesSpawned < NumberOfBodies)
    {
        const int32 RemainingBodies = NumberOfBodies - BodiesSpawned;
        const float RemainingFraction = (float)RemainingBodies / FMath::Max(1, NumberOfBodies);
        const bool bShouldBeGasGiant = (RemainingFraction <= GasGiantAppearanceThreshold) &&
            (Stream.FRandRange(0.0f, 1.0f) < GasGiantProbability);

        float NewOrbit = 0.0f;
        float NewRadius = 0.0f;
        if (!FCosmicSystemLayoutManager::TryPlacePlanet(
            Stream, SystemRadiusKm, StarRadiusKm,
            PlanetOrbits, PlanetRadii, NewOrbit, NewRadius, bShouldBeGasGiant,
            LayoutConfig, ClassRules))
        {
            break;
        }

        FCosmicBodyClassification Classification = FCosmicSystemLayoutManager::ClassifyPlanet(
            NewOrbit, NewRadius, SystemRadiusKm, Stream, RemainingBodies, NumberOfBodies, ClassRules);

        if (Classification.Type == ECosmicSystemPlanetType::GasGiant)
        {
            NewRadius = Stream.FRandRange(GasGiantRadiusMin, GasGiantRadiusMax);
        }

        // Asteroid belt ring
        if (Classification.Type == ECosmicSystemPlanetType::AsteroidBelt)
        {
            FCosmicCelestialBodySpawner::SpawnAsteroidBelt(Star, NewOrbit, RingMaterial, Stream);
            continue;
        }

        // Planet Body
        BodiesSpawned++;
        PlanetOrbits.Add(NewOrbit);
        PlanetRadii.Add(NewRadius);

        const bool bIsGasGiant = (Classification.Type == ECosmicSystemPlanetType::GasGiant);
        UMaterialInstance* PlanetMat = bIsGasGiant ? GasGiantMaterial : BaseMaterial;

        FString FolderId;
        FString BaseFolder;
#if WITH_EDITORONLY_DATA
        FolderId = GeneratedNoiseSettingsFolderId;
        BaseFolder = GeneratedNoiseSettingsAssetFolder;
#endif

        UCosmicNoiseClass* PlanetNoise = bIsGasGiant ? nullptr : NoiseManager.CreateRandomNoiseSettings(
            Stream, NewRadius, World, FolderId, BaseFolder, bSaveGeneratedNoiseSettingsAssets);

        FCosmicPlanetAppearanceParams Appearance = FCosmicPlanetAppearanceParams::MakeTelluricAppearance(Stream);

        ACosmicPlanet* Planet = FCosmicCelestialBodySpawner::SpawnPlanet(
            World, this, Star, GetActorLocation(),
            NewOrbit, NewRadius, Classification, PlanetNoise,
            PlanetMat, OceanMaterial, RingMaterial,
            Appearance, GraphicsConfig, BodyDiameterRangeKm, PlanetSurfaceGravityRange, Stream
        );

        if (!Planet)
        {
            continue;
        }

        GeneratedBodies.Add(Planet);

        // --- MOONS ---
        if (!Classification.bHasMoons || BodiesSpawned >= NumberOfBodies)
        {
            continue;
        }

        const int32 MaxMoonsForThisPlanet = Classification.MaxMoons;
        for (int32 m = 0; m < MaxMoonsForThisPlanet && BodiesSpawned < NumberOfBodies; ++m)
        {
            const float MoonOrbitKm = NewRadius * Stream.FRandRange(MoonOrbitDistanceFactorMin, MoonOrbitDistanceFactorMax);
            float MoonRadiusKm = NewRadius * Stream.FRandRange(MoonRadiusFactorMin, MoonRadiusFactorMax);
            MoonRadiusKm = FMath::Clamp(MoonRadiusKm, MoonDiameterRangeKm.X * 0.5f, MoonDiameterRangeKm.Y * 0.5f);

            UCosmicNoiseClass* MoonNoise = NoiseManager.CreateRandomNoiseSettings(
                Stream, MoonRadiusKm, World, FolderId, BaseFolder, bSaveGeneratedNoiseSettingsAssets);

            FCosmicPlanetAppearanceParams MoonAppearance = FCosmicPlanetAppearanceParams::MakeMoonAppearance(Stream);

            ACosmicPlanet* Moon = FCosmicCelestialBodySpawner::SpawnMoon(
                World, Planet, MoonOrbitKm, MoonRadiusKm,
                MoonNoise, MoonMaterial, MoonAppearance,
                MoonDiameterRangeKm, MoonSurfaceGravityRange, Stream
            );

            if (Moon)
            {
                GeneratedBodies.Add(Moon);
                BodiesSpawned++;
            }
        }
    }

    if (bIsSimulatingOrbits)
    {
        StartOrbitSimulation();
    }
    else
    {
        UpdateBodiesOrbitalPeriod();
    }
}

void ACosmicSystemGenerator::GenerateWithRandomSeed()
{
    int32 RandomSeed = 0;
    RandomSeed += static_cast<int32>(FDateTime::Now().GetTicks());
    RandomSeed += static_cast<int32>(FPlatformTime::Cycles());
    RandomSeed += reinterpret_cast<int64>(this);
    Seed = HashCombine(GetTypeHash(RandomSeed), GetTypeHash(FMath::Rand()));
    GenerateBodies();
}

void ACosmicSystemGenerator::ClearBodies()
{
    FCosmicCelestialBodySpawner::ClearBodies(GeneratedBodies);
}

void ACosmicSystemGenerator::StartOrbitSimulation()
{
    bIsSimulatingOrbits = true;
    FCosmicSystemOrbitController::StartOrbitSimulation(GeneratedBodies, OrbitSpeedMultiplier);
}

void ACosmicSystemGenerator::StopOrbitSimulation()
{
    bIsSimulatingOrbits = false;
    FCosmicSystemOrbitController::StopOrbitSimulation(GeneratedBodies);
}

void ACosmicSystemGenerator::UpdateBodiesOrbitalPeriod()
{
    FCosmicSystemOrbitController::UpdateBodiesOrbitalPeriod(GeneratedBodies, OrbitSpeedMultiplier);
}