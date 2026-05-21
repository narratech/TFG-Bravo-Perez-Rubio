#include "System/CosmicSystemGenerator.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "CosmicDefaultNoiseSettings.h"
#include "Planet/CosmicPlanet.h"
#include "Planet/CosmicRingComponent.h"
#include "DrawDebugHelpers.h"
#include "Simulation/CosmicOrbitComponent.h"
#include "Simulation/CosmicGravityComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Materials/MaterialInstance.h"

ACosmicSystemGenerator::ACosmicSystemGenerator()
{
    PrimaryActorTick.bCanEverTick = true;
#if !WITH_EDITOR
    PrimaryActorTick.bStartWithTickEnabled = false;
#endif

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

#if WITH_EDITOR
void ACosmicSystemGenerator::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World || World->WorldType != EWorldType::Editor) return;

    DrawDebugBox(World, GetActorLocation(),
        (VolumeSizeKm * 100000) * 0.5f,
        BoxColor, false, DeltaTime * 2, 0, LineWidth);

    if (bIsSimulatingOrbits)
    {
        for (AActor* Actor : GeneratedBodies)
        {
            if (!Actor) continue;
            TArray<UCosmicOrbitComponent*> Orbits;
            Actor->GetComponents<UCosmicOrbitComponent>(Orbits);
            for (UCosmicOrbitComponent* Orbit : Orbits)
            {
                Orbit->EditorSpeedMultiplier = OrbitSpeedMultiplier;
            }
        }
    }
}
#endif

bool ACosmicSystemGenerator::IsOrbitDistanceValid(
    float ProposedOrbitKm,
    float ProposedRadiusKm,
    const TArray<float>& ExistingOrbits,
    const TArray<float>& ExistingRadii) const
{
    for (int32 i = 0; i < ExistingOrbits.Num(); ++i)
    {
        float RequiredSeparation = ProposedRadiusKm + ExistingRadii[i] + MinDistanceBetweenBodies;
        if (FMath::Abs(ProposedOrbitKm - ExistingOrbits[i]) < RequiredSeparation)
            return false;
    }
    return true;
}

bool ACosmicSystemGenerator::TryPlacePlanet(
    FRandomStream& Stream,
    float SystemRadiusKm,
    float StarRadiusKm,
    const TArray<float>& ExistingOrbitDistances,
    const TArray<float>& ExistingPlanetRadii,
    float& OutOrbitDistance,
    float& OutPlanetRadius,
    bool bIsGasGiant) const
{
    const float MinDist = StarRadiusKm * OrbitDistanceMinFactor;
    const float MaxDist = SystemRadiusKm;

    // Seleccionar factores de radio según tipo
    const float RadiusFactorMin = bIsGasGiant ? GasGiantRadiusFactorMin : PlanetRadiusFactorMin;
    const float RadiusFactorMax = bIsGasGiant ? GasGiantRadiusFactorMax : PlanetRadiusFactorMax;

    // Seleccionar rango de diámetro según tipo
    const FVector2D& DiameterRange = BodyDiameterRangeKm;

    for (int32 Attempt = 0; Attempt < MaxGenerationAttempts; ++Attempt)
    {
        float Orbit = Stream.FRandRange(MinDist, MaxDist);
        float Radius = Orbit * Stream.FRandRange(RadiusFactorMin, RadiusFactorMax);

        // Aplicar límites de diámetro (convertir a radio)
        Radius = FMath::Clamp(Radius, DiameterRange.X * 0.5f, DiameterRange.Y * 0.5f);

        if (!IsOrbitDistanceValid(Orbit, Radius, ExistingOrbitDistances, ExistingPlanetRadii))
            continue;

        // Verificar agrupamiento si es necesario
        if (MaxDistanceToNearest > 0.0f && ExistingOrbitDistances.Num() > 0)
        {
            bool bHasNeighbor = false;
            for (float ExistingOrbit : ExistingOrbitDistances)
            {
                if (FMath::Abs(Orbit - ExistingOrbit) <= MaxDistanceToNearest)
                {
                    bHasNeighbor = true;
                    break;
                }
            }
            if (!bHasNeighbor) continue;
        }

        OutOrbitDistance = Orbit;
        OutPlanetRadius = Radius;
        return true;
    }
    return false;
}

ACosmicSystemGenerator::FPlanetClassification ACosmicSystemGenerator::ClassifyPlanet(
    float OrbitDistanceKm, float PlanetRadiusKm,
    float SystemRadiusKm, FRandomStream& Stream,
    int32 RemainingBodies, int32 TotalBodies) const
{
    FPlanetClassification Result;

    const float OrbitalFraction = OrbitDistanceKm / SystemRadiusKm;
    const bool bInHabitableZone = (OrbitalFraction >= HabitableZoneInnerFraction &&
        OrbitalFraction <= HabitableZoneOuterFraction);
    const bool bInBeltZone = (OrbitalFraction >= BeltZoneInnerFraction &&
        OrbitalFraction <= BeltZoneOuterFraction);

    // Calcular fracción de cuerpos restantes
    const float RemainingFraction = (float)RemainingBodies / FMath::Max(1, TotalBodies);

    // NUEVA LÓGICA: Gigante gaseoso cuando quedan pocos cuerpos y hay probabilidad
    if (RemainingFraction <= GasGiantAppearanceThreshold &&
        Stream.FRandRange(0.f, 1.f) < GasGiantProbability)
    {
        Result.Type = EPlanetType::GasGiant;
        Result.bHasOcean = false;
        Result.bHasRings = Stream.FRandRange(0.f, 1.f) < GasGiantRingProbability;
        Result.bHasMoons = true;
        Result.MaxMoons = Stream.RandRange(GasGiantMoonMin, GasGiantMoonMax);
    }
    else if (bInBeltZone && Stream.FRandRange(0.f, 1.f) < BeltProbability)
    {
        Result.Type = EPlanetType::AsteroidBelt;
        Result.bHasOcean = false;
        Result.bHasRings = false;
        Result.bHasMoons = false;
        Result.MaxMoons = 0;
    }
    else
    {
        // Planeta telúrico por defecto
        Result.Type = EPlanetType::Telluric;
        Result.bHasRings = false;
        Result.bHasMoons = true;
        Result.MaxMoons = Stream.RandRange(TelluricMoonMin, TelluricMoonMax);

        if (bInHabitableZone && Stream.FRandRange(0.f, 1.f) < TelluricOceanProbability)
        {
            Result.bHasOcean = true;
            Result.OceanSeaLevel = Stream.FRandRange(-0.00002f, 0.00002f) * PlanetRadiusKm;
        }
        else
        {
            Result.bHasOcean = false;
        }
    }

    return Result;
}

UCosmicNoiseClass* ACosmicSystemGenerator::CreateRandomNoiseSettings(FRandomStream& Stream, float PlanetRadius)
{
    // (sin cambios respecto al original; se podrían exponer rangos en el futuro)
    UCosmicDefaultNoiseSettings* NewSettings = NewObject<UCosmicDefaultNoiseSettings>(GetTransientPackage(), NAME_None, RF_Transient);
    NewSettings->Seed = Stream.RandRange(0, 999999);

    const float FeatureScaleKm = FMath::Clamp(PlanetRadius, 5.0f, 500.0f);
    FCosmicNoiseLayer& Layer = NewSettings->LayerParameters;

    Layer.NoiseType = static_cast<ECosmicNoiseType>(Stream.RandRange(0, 3));
    Layer.FractalType = static_cast<ECosmicFractalType>(Stream.RandRange(1, 3));
    Layer.Frequency = Stream.FRandRange(2.f, 5.f);
    Layer.Octaves = Stream.RandRange(6, 8);
    Layer.Lacunarity = Stream.FRandRange(1.8f, 2.5f);
    Layer.Persistence = Stream.FRandRange(0.4f, 0.7f);
    Layer.Amplitude = PlanetRadius * 30000 * Stream.FRandRange(0.03f, 0.06f);

    FCosmicNoiseBiomeParameters& Biome = NewSettings->BiomeParameters;
    Biome.HumidityFrequency = Stream.FRandRange(0.3f, 1.5f) / FeatureScaleKm;
    Biome.HumidityOctaves = Stream.RandRange(2, 5);
    Biome.HumidityOffset = Stream.FRandRange(-0.2f, 0.2f);
    Biome.HumidityContrast = Stream.FRandRange(0.8f, 1.5f);
    Biome.TemperatureFrequency = Stream.FRandRange(0.3f, 1.5f) / FeatureScaleKm;
    Biome.LatitudeEffect = Stream.FRandRange(0.5f, 1.5f);
    Biome.AltitudeTemperaturePenalty = Stream.FRandRange(0.2f, 0.6f);

    return NewSettings;
}

FColor ACosmicSystemGenerator::GetRandomColor(FRandomStream& Stream, int min, int max)
{
    int minRange = FMath::Max(min, 0);
    int maxRange = FMath::Min(max, 255);
    return FColor(
        Stream.RandRange(minRange, maxRange),
        Stream.RandRange(minRange, maxRange),
        Stream.RandRange(minRange, maxRange),
        255
    );
}

void ACosmicSystemGenerator::SetNumBodies(int32 NumBodies)
{
    NumberOfBodies = FMath::Clamp(NumBodies, 1, 200);
}

void ACosmicSystemGenerator::GenerateBodies()
{
    ClearBodies();
    if (!GetWorld()) return;

    FRandomStream Stream(Seed);
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const float SystemRadiusKm = VolumeSizeKm.X * 0.5f;
    const float StarRadiusKm = SystemRadiusKm * StarRadiusFraction;

    // --- ESTRELLA ---
    ACosmicPlanet* Star = GetWorld()->SpawnActor<ACosmicPlanet>(
        ACosmicPlanet::StaticClass(),
        GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
    if (!Star) return;

    Star->InitPlanet(
        StarRadiusKm, nullptr,
        GetRandomColor(Stream, 180, 255),
        GetRandomColor(Stream, 100, 220),
        GetRandomColor(Stream, 50, 180),
        GetRandomColor(Stream, 50, 180),
        GetRandomColor(Stream, 50, 180),
        Stream.FRandRange(0.5f, 2.f),
        Stream.FRandRange(4.f, 8.f),
        Stream.FRandRange(4.f, 6.f),
        StarMaterial, nullptr,
        false, 128, 0, 0, 0, false
    );
    Star->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

    UCosmicGravityComponent* StarGravity = NewObject<UCosmicGravityComponent>(Star);
    StarGravity->RegisterComponent();
    StarGravity->IsPlanet = true;
    StarGravity->RadiusKm = StarRadiusKm;
    StarGravity->SurfaceGravity = 274.0f;
    StarGravity->GravityMode = ECosmicGravityMode::None;
    Star->AddInstanceComponent(StarGravity);
    GeneratedBodies.Add(Star);

    // Luz direccional
    ADirectionalLight* NuevaLuz = GetWorld()->SpawnActor<ADirectionalLight>(
        ADirectionalLight::StaticClass(),
        Star->GetActorLocation(), Star->GetActorRotation(), SpawnParams);
    NuevaLuz->AttachToActor(Star, FAttachmentTransformRules::KeepWorldTransform);
    if (UDirectionalLightComponent* DL = Cast<UDirectionalLightComponent>(NuevaLuz->GetLightComponent()))
    {
        DL->AtmosphereSunLightIndex = 0;
        DL->Intensity = 50.f;
    }
    GeneratedBodies.Add(NuevaLuz);

    // Contador de cuerpos: la estrella y la luz no cuentan para NumberOfBodies (sólo planetas y lunas)
    int32 BodiesSpawned = 0;
    // Listas para mantener distancias orbitales y radios de los planetas ya colocados
    TArray<float> PlanetOrbits;
    TArray<float> PlanetRadii;

    // Generar planetas hasta alcanzar NumberOfBodies o hasta que no se pueda colocar más
    while (BodiesSpawned < NumberOfBodies)
    {
        const int32 RemainingBodies = NumberOfBodies - BodiesSpawned;
        const float RemainingFraction = (float)RemainingBodies / NumberOfBodies;
        const bool bShouldBeGasGiant = (RemainingFraction <= GasGiantAppearanceThreshold) &&
            (Stream.FRandRange(0.f, 1.f) < GasGiantProbability);

        float NewOrbit, NewRadius;
        if (!TryPlacePlanet(Stream, SystemRadiusKm, StarRadiusKm,
            PlanetOrbits, PlanetRadii, NewOrbit, NewRadius, bShouldBeGasGiant))
            break;

        // Clasificar el planeta
        FPlanetClassification Class = ClassifyPlanet(
            NewOrbit, NewRadius, SystemRadiusKm, Stream, RemainingBodies, NumberOfBodies);

        if (Class.Type == EPlanetType::GasGiant)
        {
            NewRadius = Stream.FRandRange(GasGiantRadiusMin, GasGiantRadiusMax);
        }

        // Cinturón de asteroides - no cuenta como cuerpo planetario, pero tampoco es una luna.
        if (Class.Type == EPlanetType::AsteroidBelt)
        {
            UCosmicRingComponent* Belt = NewObject<UCosmicRingComponent>(Star);
            Belt->RegisterComponent();
            Belt->AttachToComponent(Star->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
            Belt->InnerRadiusKM = NewOrbit * 0.9f;
            Belt->OuterRadiusKM = NewOrbit * 1.1f;
            Belt->BandFrequency = Stream.FRandRange(50.f, 200.f);
            Belt->RingThicknessKM = Stream.FRandRange(0.4f, 0.8f);
            Belt->SectorAngleDegrees = 4;
            Belt->VisibleSectors = 3;
            Belt->MaxInstancesPerSecond = 500;
            Belt->MinScale = 0.1f;
            Belt->MaxScale = 0.5f;
            Belt->AsteroidActivationDistanceKM = (Belt->OuterRadiusKM - Belt->InnerRadiusKM) / 4;
            Belt->RingColor = FLinearColor(
                Stream.FRandRange(0.003f, 0.007f),
                Stream.FRandRange(0.002f, 0.005f),
                Stream.FRandRange(0.001f, 0.003f), 1.f);
            Belt->MacroRingMaterial = RingMaterial;
            Star->AddInstanceComponent(Belt);
            continue; // no ocupa cupo de cuerpos
        }

        // Crear el planeta
        ACosmicPlanet* Planet = GetWorld()->SpawnActor<ACosmicPlanet>(
            ACosmicPlanet::StaticClass(),
            GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
        if (!Planet) continue;
        Planet->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

        // Incrementar contador
        BodiesSpawned++;
        PlanetOrbits.Add(NewOrbit);
        PlanetRadii.Add(NewRadius);

        UTexture2D* TexturaElegida = nullptr;
        if (PosiblesTexturas.Num() > 0)
            TexturaElegida = PosiblesTexturas[Stream.RandRange(0, PosiblesTexturas.Num() - 1)];

        const bool bIsGasGiant = (Class.Type == EPlanetType::GasGiant);
        UMaterialInstance* PlanetMat = bIsGasGiant ? GasGiantMaterial : BaseMaterial;
        const int32 ClipRes = bIsGasGiant ? GasGiantClipResolution : TelluricClipResolution;
        const int32 OceanRes = Class.bHasOcean ? OceanResolutionWithOcean : OceanResolutionWithoutOcean;
       

        Planet->InitPlanet(
            NewRadius,
            bIsGasGiant ? nullptr : CreateRandomNoiseSettings(Stream, NewRadius),
            GetRandomColor(Stream, 50, 255),
            GetRandomColor(Stream, 50, 255),
            GetRandomColor(Stream, 50, 255),
            GetRandomColor(Stream, 50, 255),
            GetRandomColor(Stream, 50, 255),
            Stream.FRandRange(0.5f, 2.f),
            Stream.FRandRange(3.f, 5.f),
            Stream.FRandRange(50.f, 100.f),
            PlanetMat,
            TexturaElegida,
            !bIsGasGiant,
            ClipRes,
            bIsGasGiant ? 1 : 6,
            32,
            bIsGasGiant ? 0.0f : 3.f,
            Class.bHasOcean,
            Class.OceanSeaLevel,
            OceanRes,
            Class.bHasOcean ? OceanMaterial : nullptr,
            nullptr
        );

        UCosmicGravityComponent* Gravity = NewObject<UCosmicGravityComponent>(Planet);
        Gravity->RegisterComponent();
        Gravity->IsPlanet = true;
        Gravity->RadiusKm = NewRadius;
        Gravity->SurfaceGravity = Stream.FRandRange(3.f, 25.f);
        Gravity->GravityMode = ECosmicGravityMode::None;
        Planet->AddInstanceComponent(Gravity);

        UCosmicOrbitComponent* Orbit = NewObject<UCosmicOrbitComponent>(Planet);
        Orbit->RegisterComponent();
        Orbit->ParentBody = Star;
        Orbit->SemiMajorAxisKm = NewOrbit;
        Orbit->Eccentricity = Stream.FRandRange(0.f, 0.15f);
        Orbit->InclinationX = Stream.FRandRange(0.f, 10.f);
        Orbit->InitialPosition = Stream.FRandRange(0.f, 1.f);
        Orbit->OrbitalPeriod = FMath::Pow(NewOrbit, 2.f);
        Orbit->InitOrbit(GetRandomColor(Stream, 50, 255));
        Planet->AddInstanceComponent(Orbit);

        if (Class.bHasRings && RingMaterial)
        {
            UCosmicRingComponent* Ring = NewObject<UCosmicRingComponent>(Planet);
            Ring->RegisterComponent();
            Ring->AttachToComponent(Planet->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
            Ring->InnerRadiusKM = NewRadius * 1.4f;
            Ring->OuterRadiusKM = NewRadius * 2.8f;
            Ring->BandFrequency = Stream.FRandRange(50.f, 200.f);
            Ring->RingRotation = FRotator(Stream.FRandRange(-15.f, 15.f), 0.f, 0.f);
            Ring->SectorAngleDegrees = 15;
            Ring->VisibleSectors = 3;
            Ring->MaxInstancesPerSecond = 500;
            Ring->MinScale = 0.1f;
            Ring->MaxScale = 0.3f;
            Ring->AsteroidActivationDistanceKM = (Ring->OuterRadiusKM - Ring->InnerRadiusKM) / 2;
            Ring->RingColor = FLinearColor(
                Stream.FRandRange(0.4f, 0.9f),
                Stream.FRandRange(0.3f, 0.8f),
                Stream.FRandRange(0.1f, 0.5f), 1.f);
            Ring->MacroRingMaterial = RingMaterial;
            Planet->AddInstanceComponent(Ring);
        }

        GeneratedBodies.Add(Planet);

        // --- LUNAS ---
        if (!Class.bHasMoons || BodiesSpawned >= NumberOfBodies)
            continue;

        int32 MaxMoonsForThisPlanet = Class.MaxMoons;
        for (int32 m = 0; m < MaxMoonsForThisPlanet && BodiesSpawned < NumberOfBodies; ++m)
        {
            ACosmicPlanet* Moon = GetWorld()->SpawnActor<ACosmicPlanet>(
                ACosmicPlanet::StaticClass(),
                Planet->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);
            if (!Moon) continue;
            Moon->AttachToActor(Planet, FAttachmentTransformRules::KeepWorldTransform);

            const float MoonOrbitKm = NewRadius * Stream.FRandRange(MoonOrbitDistanceFactorMin, MoonOrbitDistanceFactorMax);
            float MoonRadiusKm = NewRadius * Stream.FRandRange(MoonRadiusFactorMin, MoonRadiusFactorMax);
            MoonRadiusKm = FMath::Clamp(MoonRadiusKm, MoonDiameterRangeKm.X * 0.5f, MoonDiameterRangeKm.Y * 0.5f);

            Moon->InitPlanet(
                MoonRadiusKm,
                CreateRandomNoiseSettings(Stream, MoonRadiusKm),
                GetRandomColor(Stream, 50, 200),
                GetRandomColor(Stream, 50, 200),
                GetRandomColor(Stream, 50, 200),
                GetRandomColor(Stream, 50, 200),
                GetRandomColor(Stream, 50, 200),
                Stream.FRandRange(0.5f, 2.f),
                Stream.FRandRange(3.f, 5.f),
                Stream.FRandRange(50.f, 100.f),
                MoonMaterial, nullptr,
                true, 128, 4, 150, 3.f,
                false, 0.0, 64, nullptr,
                nullptr
            );

            UCosmicGravityComponent* MoonGravity = NewObject<UCosmicGravityComponent>(Moon);
            MoonGravity->RegisterComponent();
            MoonGravity->SetIsPlanet(true);
            MoonGravity->RadiusKm = MoonRadiusKm;
            MoonGravity->SurfaceGravity = Stream.FRandRange(1.f, 5.f);
            Moon->AddInstanceComponent(MoonGravity);

            UCosmicOrbitComponent* MoonOrbit = NewObject<UCosmicOrbitComponent>(Moon);
            MoonOrbit->RegisterComponent();
            MoonOrbit->ParentBody = Planet;
            MoonOrbit->SemiMajorAxisKm = MoonOrbitKm;
            MoonOrbit->Eccentricity = Stream.FRandRange(0.f, 0.1f);
            MoonOrbit->InitialPosition = Stream.FRandRange(0.f, 1.f);
            MoonOrbit->OrbitalPeriod = FMath::Pow(MoonOrbitKm, 8.f);
            MoonOrbit->InitOrbit(GetRandomColor(Stream, 50, 255));
            Moon->AddInstanceComponent(MoonOrbit);

            GeneratedBodies.Add(Moon);
            BodiesSpawned++;
        }
    }

    if (bIsSimulatingOrbits)
    {
        StartOrbitSimulation();
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
    for (AActor* Actor : GeneratedBodies)
    {
        if (ACosmicPlanet* Planet = Cast<ACosmicPlanet>(Actor))
        {
            Planet->CleanupNoiseSettings();
        }
        if (Actor)
        {
            Actor->Destroy();
        }
    }
    GeneratedBodies.Empty();
}

void ACosmicSystemGenerator::StartOrbitSimulation()
{
    bIsSimulatingOrbits = true;
    for (AActor* Actor : GeneratedBodies)
    {
        if (!Actor) continue;
        TArray<UCosmicOrbitComponent*> Orbits;
        Actor->GetComponents<UCosmicOrbitComponent>(Orbits);
        for (UCosmicOrbitComponent* Orbit : Orbits)
        {
            Orbit->EditorSpeedMultiplier = OrbitSpeedMultiplier;
            Orbit->bEditorSimulating = true;
        }
    }
}

void ACosmicSystemGenerator::StopOrbitSimulation()
{
    bIsSimulatingOrbits = false;
    for (AActor* Actor : GeneratedBodies)
    {
        if (!Actor) continue;
        TArray<UCosmicOrbitComponent*> Orbits;
        Actor->GetComponents<UCosmicOrbitComponent>(Orbits);
        for (UCosmicOrbitComponent* Orbit : Orbits)
        {
            Orbit->bEditorSimulating = false;
        }
    }
}