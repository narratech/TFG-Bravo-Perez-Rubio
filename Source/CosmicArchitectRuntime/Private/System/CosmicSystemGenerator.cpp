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
    // E: Desactivamos el Tick porque no necesitamos actualizaciones por frame.
    // I: Disable Tick as we don't need per-frame updates.

    PrimaryActorTick.bCanEverTick = true;
#if !WITH_EDITOR
    PrimaryActorTick.bStartWithTickEnabled = false;
#endif

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // E: Inicialización de variables por defecto.
    // I: Default variable initialization.
    VolumeSizeKm = FVector(2000.0f, 2000.0f, 5.0f); // 20 Km
    NumberOfBodies = 5;
    Seed = 12345;

    // E: Rango de diámetro por defecto: entre 100 metros (0.1 km) y 500 metros (0.5 km).
    // I: Default diameter range: between 100 meters (0.1 km) and 500 meters (0.5 km).
    BodyDiameterRangeKm = FVector2D(0.1f, 0.5f);

    MinDistanceBetweenBodies = 1.0f; // 1 Km
    MaxDistanceToNearest = 0.0f;     // 0 = Sin agrupación forzada / No forced clustering       
    MaxGenerationAttempts = 100;

}

#if WITH_EDITOR
void ACosmicSystemGenerator::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World || World->WorldType != EWorldType::Editor) return;

    const FVector Center = GetActorLocation();
    
    DrawDebugBox(
        World,
        Center,
        (VolumeSizeKm * 100000) * 0.5f,
        BoxColor,
        false,
        DeltaTime * 2,
        0,
        LineWidth
    );
}
#endif

ACosmicSystemGenerator::FPlanetClassification ACosmicSystemGenerator::ClassifyPlanet(
    float OrbitDistanceKm, float PlanetRadiusKm,
    float SystemRadiusKm, FRandomStream& Stream) const
{
    FPlanetClassification Result;

    // Ratio radio/distancia: planetas grandes cerca = gigantes gaseosos
    const float SizeRatio = PlanetRadiusKm / OrbitDistanceKm;

    // Zona habitable: entre 25% y 65% del radio del sistema
    const float OrbitalFraction = OrbitDistanceKm / SystemRadiusKm;
    const bool bInHabitableZone = OrbitalFraction >= 0.25f && OrbitalFraction <= 0.65f;

    // Zona de transición (cinturón de asteroides): 55%-70%
    const bool bInBeltZone = OrbitalFraction >= 0.55f && OrbitalFraction <= 0.70f;

    if (SizeRatio > 0.04f)
    {
        // GAS GIANT 
        Result.Type = EPlanetType::GasGiant;
        Result.bHasOcean = false;
        Result.bHasRings = Stream.FRandRange(0.f, 1.f) > 0.35f; // 65% de tener anillos
        Result.bHasMoons = true;
        Result.MaxMoons = Stream.RandRange(1, 6);
    }
    else if (bInBeltZone && Stream.FRandRange(0.f, 1.f) > 0.6f)
    {
        // CINTURÓN DE ASTEROIDES 
        Result.Type = EPlanetType::AsteroidBelt;
        Result.bHasOcean = false;
        Result.bHasRings = false;
        Result.bHasMoons = false;
        Result.MaxMoons = 0;
    }
    else
    {
        // PLANETA ROCOSO - TELÚRICO 
        Result.Type = EPlanetType::Telluric;
        Result.bHasRings = false;
        Result.bHasMoons = true;
        Result.MaxMoons = Stream.RandRange(0, 3);

        // Océano solo en zona habitable, con 70% de probabilidad
        if (bInHabitableZone && Stream.FRandRange(0.f, 1.f) > 0.3f)
        {
            Result.bHasOcean = true;
            Result.OceanSeaLevel = Stream.FRandRange(-0.005f, 0.01f) * PlanetRadiusKm;
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
    UCosmicDefaultNoiseSettings* NewSettings = NewObject<UCosmicDefaultNoiseSettings>(GetTransientPackage(),NAME_None, RF_Transient);

    // SEED
    NewSettings->Seed = Stream.RandRange(0, 999999);

    // ESCALA BASE
    const float RadiusScale = PlanetRadius / 1000.0f;

    const float FeatureScaleKm = FMath::Clamp(PlanetRadius, 5.0f, 500.0f);

    // NOISE LAYER
    FCosmicNoiseLayer& Layer = NewSettings->LayerParameters;

    // Tipo de ruido aleatorio
    Layer.NoiseType = static_cast<ECosmicNoiseType>(Stream.RandRange(0, 3));

    // Tipo fractal
    Layer.FractalType = static_cast<ECosmicFractalType>(Stream.RandRange(1, 3));

    // Frecuencia: inversamente proporcional al tamaño
    Layer.Frequency = Stream.FRandRange(0.08f, 0.2f) * PlanetRadius;

    // Octavas
    Layer.Octaves = Stream.RandRange(6, 8);

    // Lacunarity 
    Layer.Lacunarity = Stream.FRandRange(1.8f, 2.5f);

    // Persistencia 
    Layer.Persistence = Stream.FRandRange(0.4f, 0.7f);

    // Amplitud: proporcional al radio del planeta
    Layer.Amplitude = PlanetRadius * 100000 * Stream.FRandRange(0.03f, 0.06f);

    // BIOMA / CLIMA
    FCosmicNoiseBiomeParameters& Biome = NewSettings->BiomeParameters;

    // Humedad
    Biome.HumidityFrequency = Stream.FRandRange(0.3f, 1.5f) / FeatureScaleKm;
    Biome.HumidityOctaves = Stream.RandRange(2, 5);
    Biome.HumidityOffset = Stream.FRandRange(-0.2f, 0.2f);
    Biome.HumidityContrast = Stream.FRandRange(0.8f, 1.5f);

    // Temperatura
    Biome.TemperatureFrequency = Stream.FRandRange(0.3f, 1.5f) / FeatureScaleKm;

    // Latitud
    Biome.LatitudeEffect = Stream.FRandRange(0.5f, 1.5f);

    // Penalizacion por altura
    Biome.AltitudeTemperaturePenalty = Stream.FRandRange(0.2f, 0.6f);

    return NewSettings;
}

FColor ACosmicSystemGenerator::GetRandomColor(FRandomStream& Stream, int min, int max)
{
    int minRange = FMath::Max(min, 0);
    int maxRange = FMath::Min(max, 255);

    return FColor(
        Stream.RandRange(minRange, maxRange),  // R
        Stream.RandRange(minRange, maxRange),  // G
        Stream.RandRange(minRange, maxRange),  // B
        255
    );
}

void ACosmicSystemGenerator::GenerateBodies()
{
    ClearBodies();
    if (!GetWorld()) return;

    FRandomStream Stream(Seed);
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const float SystemRadiusKm = VolumeSizeKm.X * 0.5f;

    // ESTRELLA 

    ACosmicPlanet* Star = GetWorld()->SpawnActor<ACosmicPlanet>(
        ACosmicPlanet::StaticClass(),
        GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

    if (!Star) return;

    const float StarRadiusKm = SystemRadiusKm * 0.15f;

    Star->InitPlanet(
        StarRadiusKm, nullptr,
        GetRandomColor(Stream, 180, 255),
        GetRandomColor(Stream, 100, 220),
        GetRandomColor(Stream, 50, 180),
        GetRandomColor(Stream, 50, 180),
        GetRandomColor(Stream, 50, 180),
        Stream.FRandRange(0.5f, 2.f),
        Stream.FRandRange(3.f, 5.f),
        Stream.FRandRange(50.f, 100.f),
        StarMaterial, nullptr,
        false,
        128
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

    // PLANETAS 

    for (int32 i = 0; i < NumberOfBodies; i++)
    {
        const float OrbitDistanceKm = Stream.FRandRange(StarRadiusKm * 3.f, SystemRadiusKm);
        const float PlanetRadiusKm = OrbitDistanceKm * Stream.FRandRange(0.01f, 0.06f);

        FPlanetClassification Class = ClassifyPlanet(
            OrbitDistanceKm, PlanetRadiusKm, SystemRadiusKm, Stream);

        // Cinturón de asteroides: solo un CosmicRingComponent sobre la estrella
        if (Class.Type == EPlanetType::AsteroidBelt)
        {
            UCosmicRingComponent* Belt = NewObject<UCosmicRingComponent>(Star);
            Belt->RegisterComponent();
            Belt->AttachToComponent(
                Star->GetRootComponent(),
                FAttachmentTransformRules::KeepRelativeTransform);

            Belt->InnerRadiusKM = OrbitDistanceKm * 0.9f;
            Belt->OuterRadiusKM = OrbitDistanceKm * 1.1f;
            Belt->BandFrequency = Stream.FRandRange(30.f, 80.f);
            Belt->RingColor = FLinearColor(
                Stream.FRandRange(0.3f, 0.7f),
                Stream.FRandRange(0.2f, 0.5f),
                Stream.FRandRange(0.1f, 0.3f), 1.f);
            Belt->MacroRingMaterial = RingMaterial;

            Star->AddInstanceComponent(Belt);
            continue; // No spawn planeta
        }

        // Spawn planeta (rocoso o gigante gaseoso)

        ACosmicPlanet* Planet = GetWorld()->SpawnActor<ACosmicPlanet>(
            ACosmicPlanet::StaticClass(),
            GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

        if (!Planet) continue;
        Planet->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

        UTexture2D* TexturaElegida = nullptr;
        if (PosiblesTexturas.Num() > 0)
            TexturaElegida = PosiblesTexturas[Stream.RandRange(0, PosiblesTexturas.Num() - 1)];

        // Material según tipo
        UMaterialInstance* PlanetMat =
            (Class.Type == EPlanetType::GasGiant) ? GasGiantMaterial : BaseMaterial;

        // Resolución según tamaño
        const int32 ClipRes = (Class.Type == EPlanetType::GasGiant) ? 64 : 128;
        const int32 OceanRes = Class.bHasOcean ? 128 : 64;

        bool IsGasGiant = (Class.Type == EPlanetType::GasGiant);

        Planet->InitPlanet(
            PlanetRadiusKm,
            IsGasGiant ? nullptr : CreateRandomNoiseSettings(Stream, PlanetRadiusKm),
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
            // Clipmap
            !IsGasGiant,
            ClipRes,
            IsGasGiant ? 1 : 6,
            32, 
            IsGasGiant ? 0.0 : 3.f,
            // Océano
            Class.bHasOcean,
            Class.OceanSeaLevel,
            OceanRes,
            Class.bHasOcean ? OceanMaterial : nullptr,
            // Follaje (null por ahora, añadir colección si se desea)
            nullptr
        );

        // Gravedad
        UCosmicGravityComponent* Gravity = NewObject<UCosmicGravityComponent>(Planet);
        Gravity->RegisterComponent();
        Gravity->IsPlanet = true;
        Gravity->RadiusKm = PlanetRadiusKm;
        Gravity->SurfaceGravity = Stream.FRandRange(3.f, 25.f);
        Gravity->GravityMode = ECosmicGravityMode::None;
        Planet->AddInstanceComponent(Gravity);

        // Órbita
        UCosmicOrbitComponent* Orbit = NewObject<UCosmicOrbitComponent>(Planet);
        Orbit->RegisterComponent();
        Orbit->ParentBody = Star;
        Orbit->SemiMajorAxisKm = OrbitDistanceKm;
        Orbit->Eccentricity = Stream.FRandRange(0.f, 0.15f);
        Orbit->InclinationX = Stream.FRandRange(0.f, 10.f);
        Orbit->InitialPosition = Stream.FRandRange(0.f, 1.f);
        Orbit->OrbitalPeriod = FMath::Pow(OrbitDistanceKm, 3.f);
        Orbit->InitOrbit(GetRandomColor(Stream, 50, 255));
        Planet->AddInstanceComponent(Orbit);

        // Anillos en gigantes gaseosos 
        if (Class.bHasRings && RingMaterial)
        {
            UCosmicRingComponent* Ring = NewObject<UCosmicRingComponent>(Planet);
            Ring->RegisterComponent();
            Ring->AttachToComponent(
                Planet->GetRootComponent(),
                FAttachmentTransformRules::KeepRelativeTransform);

            Ring->InnerRadiusKM = PlanetRadiusKm * 1.4f;
            Ring->OuterRadiusKM = PlanetRadiusKm * 2.8f;
            Ring->InnerRadiusUV = 0.2;
            Ring->OuterRadiusUV = 0.45;
            Ring->BandFrequency = Stream.FRandRange(20.f, 60.f);
            Ring->RingRotation = FRotator(Stream.FRandRange(-15.f, 15.f), 0.f, 0.f);
            Ring->RingColor = FLinearColor(
                Stream.FRandRange(0.4f, 0.9f),
                Stream.FRandRange(0.3f, 0.8f),
                Stream.FRandRange(0.1f, 0.5f), 1.f);
            Ring->MacroRingMaterial = RingMaterial;
            Planet->AddInstanceComponent(Ring);
        }

        GeneratedBodies.Add(Planet);

        // Lunas (solo planetas rocosos, no gigantes)
        if (!Class.bHasMoons) continue;

        const int32 MoonCount = Stream.RandRange(0, Class.MaxMoons);

        for (int32 m = 0; m < MoonCount; m++)
        {
            ACosmicPlanet* Moon = GetWorld()->SpawnActor<ACosmicPlanet>(
                ACosmicPlanet::StaticClass(),
                Planet->GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

            if (!Moon) continue;
            Moon->AttachToActor(Planet, FAttachmentTransformRules::KeepWorldTransform);

            const float MoonOrbitKm = PlanetRadiusKm * Stream.FRandRange(10.f, 15.f);
            const float MoonRadiusKm = PlanetRadiusKm * Stream.FRandRange(0.1f, 0.3f);

            // Las lunas nunca tienen océano
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
        }
    }
    /*UE_LOG(LogTemp, Display,
        TEXT("System Generation Complete. Bodies: %d"),
        GeneratedBodies.Num());*/
}

void ACosmicSystemGenerator::GenerateWithRandomSeed()
{
    // E: Generar semilla basada en tiempo del sistema y dirección de memoria.
    // I: Generate seed based on system time and memory address.
    int32 RandomSeed = 0;
    RandomSeed += static_cast<int32>(FDateTime::Now().GetTicks());
    RandomSeed += static_cast<int32>(FPlatformTime::Cycles());
    RandomSeed += reinterpret_cast<int64>(this);

    Seed = HashCombine(GetTypeHash(RandomSeed), GetTypeHash(FMath::Rand()));

    GenerateBodies();
}

void ACosmicSystemGenerator::ClearBodies()
{
    // E: Recorremos el array y destruimos los actores válidos.
    // I: Iterate through the array and destroy valid actors.
    for (AActor* Actor : GeneratedBodies)
    {
        if (ACosmicPlanet* Planet = Cast<ACosmicPlanet>(Actor))
        {
            Planet->CleanupNoiseSettings();  // Limpieza explícita
        }
        if (Actor)
        {
            Actor->Destroy();
        }
    }
    GeneratedBodies.Empty();
}