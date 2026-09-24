// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "System/CosmicCelestialBodySpawner.h"
#include "Planet/CosmicPlanet.h"
#include "Planet/CosmicRingComponent.h"
#include "Simulation/CosmicGravityComponent.h"
#include "Simulation/CosmicOrbitComponent.h"
#include "Terrain/CosmicOceanComponent.h"
#include "System/CosmicSystemLayoutManager.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "Materials/MaterialInstance.h"

UMaterialInstance* FCosmicCelestialBodySpawner::ResolveDefaultMaterial(const TArray<const TCHAR*>& CandidatePaths)
{
    for (const TCHAR* Path : CandidatePaths)
    {
        if (UMaterialInstance* Mat = LoadObject<UMaterialInstance>(nullptr, Path))
        {
            return Mat;
        }
    }
    return nullptr;
}

UMaterialInstance* FCosmicCelestialBodySpawner::GetDefaultEarthMaterial()
{
    const TArray<const TCHAR*> Candidates = {
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicEarthV2.MI_CosmicEarthV2"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicEarth2.MI_CosmicEarth2"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicEarthV2.MI_CosmicEarthV2"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicEarth2.MI_CosmicEarth2"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicEarth.MI_CosmicEarth"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicRealEarth.MI_CosmicRealEarth"),
        TEXT("/Game/Materials/MI_CosmicEarthV2.MI_CosmicEarthV2")
    };
    return ResolveDefaultMaterial(Candidates);
}

UMaterialInstance* FCosmicCelestialBodySpawner::GetDefaultMoonMaterial()
{
    const TArray<const TCHAR*> Candidates = {
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicMoonV2.MI_CosmicMoonV2"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicMoonV2.MI_CosmicMoonV2"),
        TEXT("/Game/Materials/MI_CosmicMoonV2.MI_CosmicMoonV2"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicMoon.MI_CosmicMoon")
    };
    return ResolveDefaultMaterial(Candidates);
}

UMaterialInstance* FCosmicCelestialBodySpawner::GetDefaultStarMaterial()
{
    const TArray<const TCHAR*> Candidates = {
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicSun.MI_CosmicSun"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicSun.MI_CosmicSun"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicStar.M_CosmicStar")
    };
    return ResolveDefaultMaterial(Candidates);
}

UMaterialInstance* FCosmicCelestialBodySpawner::GetDefaultGasGiantMaterial()
{
    const TArray<const TCHAR*> Candidates = {
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicGasGiant.MI_CosmicGasGiant"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicGasGiant.MI_CosmicGasGiant"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicJupiter.MI_CosmicJupiter")
    };
    return ResolveDefaultMaterial(Candidates);
}

UMaterialInstance* FCosmicCelestialBodySpawner::GetDefaultRingMaterial()
{
    const TArray<const TCHAR*> Candidates = {
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicRing.MI_CosmicRing"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicRing.MI_CosmicRing"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicRing.M_CosmicRing")
    };
    return ResolveDefaultMaterial(Candidates);
}

void FCosmicCelestialBodySpawner::InitializePlanetAppearance(
    ACosmicPlanet* Planet,
    float RadiusKm,
    UCosmicNoiseClass* NoiseSettings,
    const FCosmicPlanetAppearanceParams& Appearance,
    UMaterialInstance* Material,
    bool bUseClipmap,
    int32 ClipResolution,
    int32 NumLevels,
    int32 MinTriangleSize,
    float HeightVisibility,
    bool bHasOcean,
    double OceanSeaLevel,
    int32 OceanResolution,
    UMaterialInstance* OceanMaterial)
{
    if (!Planet)
    {
        return;
    }

    // Encapsulate call to InitPlanet using both current archetype parameters and legacy fallbacks
    Planet->InitPlanet(
        RadiusKm,
        NoiseSettings,
        Appearance.ArchetypeIndex,
        Appearance.bUseCustomArchetype,
        Appearance.TerrainColorLow,
        Appearance.TerrainColorMid,
        Appearance.TerrainColorHigh,
        Appearance.RockColor,
        Appearance.bEnableSnow,
        Material,
        bUseClipmap,
        ClipResolution,
        NumLevels,
        MinTriangleSize,
        HeightVisibility,
        bHasOcean,
        OceanSeaLevel,
        OceanResolution,
        OceanMaterial,
        nullptr
    );
}

ACosmicPlanet* FCosmicCelestialBodySpawner::SpawnStar(
    UWorld* World,
    AActor* OwnerActor,
    const FVector& Location,
    float StarRadiusKm,
    UMaterialInstance* StarMaterial,
    FRandomStream& Stream)
{
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ACosmicPlanet* Star = World->SpawnActor<ACosmicPlanet>(
        ACosmicPlanet::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
    if (!Star)
    {
        return nullptr;
    }

    const FCosmicPlanetAppearanceParams StarAppearance = FCosmicPlanetAppearanceParams::MakeStarAppearance(Stream);

    InitializePlanetAppearance(
        Star,
        StarRadiusKm,
        nullptr,
        StarAppearance,
        StarMaterial,
        false, 128, 0, 0, 0.0f,
        false, 0.0, 0, nullptr
    );

    if (OwnerActor)
    {
        Star->AttachToActor(OwnerActor, FAttachmentTransformRules::KeepWorldTransform);
    }

    UCosmicGravityComponent* StarGravity = NewObject<UCosmicGravityComponent>(Star);
    StarGravity->RegisterComponent();
    StarGravity->IsPlanet = true;
    StarGravity->RadiusKm = StarRadiusKm;
    StarGravity->SurfaceGravity = 274.0f;
    StarGravity->GravityMode = ECosmicGravityMode::None;
    Star->AddInstanceComponent(StarGravity);

    return Star;
}

APointLight* FCosmicCelestialBodySpawner::SpawnStarLight(
    UWorld* World,
    ACosmicPlanet* Star,
    float SystemRadiusKm,
    float MaxBodyRadiusKm,
    float MoonOrbitDistanceFactorMax,
    float MoonDiameterMaxKm)
{
    if (!World || !Star)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APointLight* PointLight = World->SpawnActor<APointLight>(
        APointLight::StaticClass(),
        Star->GetActorLocation(),
        Star->GetActorRotation(),
        SpawnParams
    );

    if (PointLight)
    {
        PointLight->AttachToActor(Star, FAttachmentTransformRules::KeepWorldTransform);
        if (UPointLightComponent* PL = Cast<UPointLightComponent>(PointLight->GetLightComponent()))
        {
            const float MaxMoonReachKm = MaxBodyRadiusKm * MoonOrbitDistanceFactorMax + (MoonDiameterMaxKm * 0.5f);
            const float MaxRingReachKm = MaxBodyRadiusKm * 2.8f;
            const float LightRangeKm = FMath::Max(SystemRadiusKm + FMath::Max(MaxMoonReachKm, MaxRingReachKm), SystemRadiusKm * 1.1f);

            PL->AttenuationRadius = LightRangeKm * 100000.0f;
            PL->Intensity = 5.0f;
            PL->bUseInverseSquaredFalloff = false;
            PL->LightFalloffExponent = 1.0f;
            PL->MarkRenderStateDirty();
        }
    }

    return PointLight;
}

ACosmicPlanet* FCosmicCelestialBodySpawner::SpawnPlanet(
    UWorld* World,
    AActor* OwnerActor,
    ACosmicPlanet* Star,
    const FVector& Location,
    float OrbitKm,
    float RadiusKm,
    const FCosmicBodyClassification& Classification,
    UCosmicNoiseClass* NoiseSettings,
    UMaterialInstance* PlanetMaterial,
    UMaterialInstance* OceanMaterial,
    UMaterialInstance* RingMaterial,
    const FCosmicPlanetAppearanceParams& Appearance,
    const FCosmicSystemGraphicsConfig& GraphicsConfig,
    const FVector2D& BodyDiameterRangeKm,
    const FVector2D& PlanetGravityRange,
    FRandomStream& Stream)
{
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ACosmicPlanet* Planet = World->SpawnActor<ACosmicPlanet>(
        ACosmicPlanet::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
    if (!Planet)
    {
        return nullptr;
    }

    if (OwnerActor)
    {
        Planet->AttachToActor(OwnerActor, FAttachmentTransformRules::KeepWorldTransform);
    }

    const bool bIsGasGiant = (Classification.Type == ECosmicSystemPlanetType::GasGiant);
    const int32 ClipRes = bIsGasGiant ? GraphicsConfig.GasGiantClipResolution : GraphicsConfig.TelluricClipResolution;
    const int32 OceanRes = Classification.bHasOcean ? GraphicsConfig.OceanResolutionWithOcean : GraphicsConfig.OceanResolutionWithoutOcean;

    InitializePlanetAppearance(
        Planet,
        RadiusKm,
        bIsGasGiant ? nullptr : NoiseSettings,
        Appearance,
        PlanetMaterial,
        !bIsGasGiant,
        ClipRes,
        bIsGasGiant ? 1 : 6,
        32,
        bIsGasGiant ? 0.0f : 3.0f,
        Classification.bHasOcean,
        Classification.OceanSeaLevel,
        OceanRes,
        Classification.bHasOcean ? OceanMaterial : nullptr
    );

    // Randomize ocean wave parameters if ocean component is present
    if (Classification.bHasOcean && Planet->OceanComponent)
    {
        const float RadiusFactor = FMath::Clamp(RadiusKm / 1.0f, 0.5f, 3.0f);
        Planet->OceanComponent->WaveHeight = Stream.FRandRange(100.0f, 250.0f) * RadiusFactor;
        Planet->OceanComponent->WaveLength = Stream.FRandRange(4000.0f, 8000.0f) * RadiusFactor;
        Planet->OceanComponent->WaveSteepness = Stream.FRandRange(0.3f, 0.7f);
        Planet->OceanComponent->WaveSpeed = Stream.FRandRange(0.5f, 2.0f);
        Planet->OceanComponent->WaveChop = Stream.FRandRange(0.6f, 1.4f);
        Planet->OceanComponent->WaveSpread = Stream.FRandRange(0.8f, 1.2f);

        Planet->OceanComponent->WaterColor = FLinearColor(
            Stream.FRandRange(0.01f, 0.05f),
            Stream.FRandRange(0.10f, 0.30f),
            Stream.FRandRange(0.30f, 0.55f), 1.0f);
        Planet->OceanComponent->WaterScattering = FLinearColor(
            Stream.FRandRange(0.01f, 0.05f),
            Stream.FRandRange(0.02f, 0.08f),
            Stream.FRandRange(0.10f, 0.20f), 1.0f);
        Planet->OceanComponent->WaterAbsortion = FLinearColor(0.45f, 0.05f, 0.01f, 1.0f);
    }

    UCosmicGravityComponent* Gravity = NewObject<UCosmicGravityComponent>(Planet);
    Gravity->RegisterComponent();
    Gravity->IsPlanet = true;
    Gravity->RadiusKm = RadiusKm;
    Gravity->SurfaceGravity = FCosmicSystemLayoutManager::CalculateSurfaceGravity(
        RadiusKm, BodyDiameterRangeKm * 0.5f, PlanetGravityRange);
    Gravity->GravityMode = ECosmicGravityMode::None;
    Planet->AddInstanceComponent(Gravity);

    if (Star)
    {
        UCosmicOrbitComponent* Orbit = NewObject<UCosmicOrbitComponent>(Planet);
        Orbit->RegisterComponent();
        Orbit->ParentBody = Star;
        Orbit->SemiMajorAxisKm = OrbitKm;
        Orbit->Eccentricity = Stream.FRandRange(0.0f, 0.15f);
        Orbit->InclinationX = Stream.FRandRange(0.0f, 10.0f);
        Orbit->InitialPosition = Stream.FRandRange(0.0f, 1.0f);
        Orbit->OrbitalPeriod = FMath::Pow(OrbitKm, 2.0f);
        Orbit->InitOrbit(FCosmicPlanetAppearanceParams::GetRandomColor(Stream, 50, 255));
        Planet->AddInstanceComponent(Orbit);
    }

    if (Classification.bHasRings && RingMaterial)
    {
        UCosmicRingComponent* Ring = NewObject<UCosmicRingComponent>(Planet);
        Ring->RegisterComponent();
        Ring->AttachToComponent(Planet->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        Ring->InnerRadiusKM = RadiusKm * 1.4f;
        Ring->OuterRadiusKM = RadiusKm * 2.8f;
        Ring->BandFrequency = Stream.FRandRange(50.0f, 200.0f);
        Ring->RingRotation = FRotator(Stream.FRandRange(-15.0f, 15.0f), 0.0f, 0.0f);
        Ring->SectorAngleDegrees = 15;
        Ring->VisibleSectors = 3;
        Ring->MaxInstancesPerSecond = 500;
        Ring->MinScale = 0.1f;
        Ring->MaxScale = 0.3f;
        Ring->AsteroidActivationDistanceKM = (Ring->OuterRadiusKM - Ring->InnerRadiusKM) / 2.0f;
        Ring->RingColor = FLinearColor(
            Stream.FRandRange(0.4f, 0.9f),
            Stream.FRandRange(0.3f, 0.8f),
            Stream.FRandRange(0.1f, 0.5f), 1.0f);
        Ring->MacroRingMaterial = RingMaterial;
        Planet->AddInstanceComponent(Ring);
    }

    return Planet;
}

ACosmicPlanet* FCosmicCelestialBodySpawner::SpawnMoon(
    UWorld* World,
    ACosmicPlanet* ParentPlanet,
    float OrbitKm,
    float RadiusKm,
    UCosmicNoiseClass* NoiseSettings,
    UMaterialInstance* MoonMaterial,
    const FCosmicPlanetAppearanceParams& Appearance,
    const FVector2D& MoonDiameterRangeKm,
    const FVector2D& MoonGravityRange,
    FRandomStream& Stream)
{
    if (!World || !ParentPlanet)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ACosmicPlanet* Moon = World->SpawnActor<ACosmicPlanet>(
        ACosmicPlanet::StaticClass(),
        ParentPlanet->GetActorLocation(),
        FRotator::ZeroRotator,
        SpawnParams
    );
    if (!Moon)
    {
        return nullptr;
    }

    Moon->AttachToActor(ParentPlanet, FAttachmentTransformRules::KeepWorldTransform);

    InitializePlanetAppearance(
        Moon,
        RadiusKm,
        NoiseSettings,
        Appearance,
        MoonMaterial,
        true, 128, 4, 150, 3.0f,
        false, 0.0, 64, nullptr
    );

    UCosmicGravityComponent* MoonGravity = NewObject<UCosmicGravityComponent>(Moon);
    MoonGravity->RegisterComponent();
    MoonGravity->SetIsPlanet(true);
    MoonGravity->RadiusKm = RadiusKm;
    MoonGravity->SurfaceGravity = FCosmicSystemLayoutManager::CalculateSurfaceGravity(
        RadiusKm, MoonDiameterRangeKm * 0.5f, MoonGravityRange);
    Moon->AddInstanceComponent(MoonGravity);

    UCosmicOrbitComponent* MoonOrbit = NewObject<UCosmicOrbitComponent>(Moon);
    MoonOrbit->RegisterComponent();
    MoonOrbit->ParentBody = ParentPlanet;
    MoonOrbit->SemiMajorAxisKm = OrbitKm;
    MoonOrbit->Eccentricity = Stream.FRandRange(0.0f, 0.1f);
    MoonOrbit->InitialPosition = Stream.FRandRange(0.0f, 1.0f);
    MoonOrbit->OrbitalPeriod = FMath::Pow(OrbitKm, 8.0f);
    MoonOrbit->InitOrbit(FCosmicPlanetAppearanceParams::GetRandomColor(Stream, 50, 255));
    Moon->AddInstanceComponent(MoonOrbit);

    return Moon;
}

UCosmicRingComponent* FCosmicCelestialBodySpawner::SpawnAsteroidBelt(
    ACosmicPlanet* Star,
    float OrbitKm,
    UMaterialInstance* RingMaterial,
    FRandomStream& Stream)
{
    if (!Star)
    {
        return nullptr;
    }

    UCosmicRingComponent* Belt = NewObject<UCosmicRingComponent>(Star);
    Belt->RegisterComponent();
    Belt->AttachToComponent(Star->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    Belt->InnerRadiusKM = OrbitKm * 0.9f;
    Belt->OuterRadiusKM = OrbitKm * 1.1f;
    Belt->BandFrequency = Stream.FRandRange(50.0f, 200.0f);
    Belt->RingThicknessKM = Stream.FRandRange(0.4f, 0.8f);
    Belt->SectorAngleDegrees = 4;
    Belt->VisibleSectors = 3;
    Belt->MaxInstancesPerSecond = 500;
    Belt->MinScale = 0.1f;
    Belt->MaxScale = 0.5f;
    Belt->AsteroidActivationDistanceKM = (Belt->OuterRadiusKM - Belt->InnerRadiusKM) / 4.0f;
    Belt->RingColor = FLinearColor(
        Stream.FRandRange(0.003f, 0.007f),
        Stream.FRandRange(0.002f, 0.005f),
        Stream.FRandRange(0.001f, 0.003f), 1.0f);
    Belt->MacroRingMaterial = RingMaterial;
    Star->AddInstanceComponent(Belt);

    return Belt;
}

void FCosmicCelestialBodySpawner::ClearBodies(TArray<AActor*>& Bodies)
{
    for (AActor* Actor : Bodies)
    {
        if (Actor)
        {
            Actor->Destroy();
        }
    }
    Bodies.Empty();
}
