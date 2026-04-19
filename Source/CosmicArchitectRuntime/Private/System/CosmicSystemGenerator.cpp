#include "System/CosmicSystemGenerator.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/Package.h"
#include "Engine/World.h"              
#include "Engine/EngineTypes.h"        
#include "GameFramework/Actor.h"
#include "CosmicDefaultNoiseSettings.h"
#include "Planet/CosmicPlanet.h"
#include "Simulation/CosmicOrbitComponent.h"
#include "Simulation/CosmicGravityComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"

ACosmicSystemGenerator::ACosmicSystemGenerator()
{
    // E: Desactivamos el Tick porque no necesitamos actualizaciones por frame.
    // I: Disable Tick as we don't need per-frame updates.
    PrimaryActorTick.bCanEverTick = false;

    GenerationVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("GenerationVolume"));
    RootComponent = GenerationVolume;
    GenerationVolume->SetLineThickness(20.0f);

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

void ACosmicSystemGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform); // O usa AActor::OnConstruction(Transform) si tienes problemas con macros

    // E: Actualizamos el tamaño visual de la caja cuando cambiamos el valor en el editor.
    // I: Update the visual box size when changing the value in the editor.
    if (GenerationVolume)
    {
        // E: Convertimos Km a Unidades de Unreal (cm). 1 Km = 100,000 cm.
        // I: Convert Km to Unreal Units (cm). 1 Km = 100,000 cm.
        GenerationVolume->SetBoxExtent(VolumeSizeKm * 100000.0f);
    }
}

void ACosmicSystemGenerator::GenerateStar()
{
    
}

UCosmicNoiseClass* ACosmicSystemGenerator::CreateRandomNoiseSettings(FRandomStream& Stream, const float PlanetRadius)
{
    UCosmicDefaultNoiseSettings* NewSettings = NewObject<UCosmicDefaultNoiseSettings>(GetTransientPackage(),NAME_None, RF_Transient);

    // SEED
    NewSettings->Seed = Stream.RandRange(0, 999999);

    // ESCALA BASE
    const float RadiusScale = PlanetRadius / 1000.0f;

    const float FeatureScaleKm = FMath::Clamp(PlanetRadius * 0.02f, 5.0f, 500.0f);

    // NOISE LAYER
    FCosmicNoiseLayer& Layer = NewSettings->LayerParameters;

    // Tipo de ruido aleatorio
    Layer.NoiseType = static_cast<ECosmicNoiseType>(Stream.RandRange(0, 3));

    // Tipo fractal
    Layer.FractalType = static_cast<ECosmicFractalType>(Stream.RandRange(0, 3));

    // Frecuencia: inversamente proporcional al tamaño
    Layer.Frequency = Stream.FRandRange(0.5f, 2.5f) / FeatureScaleKm;

    // Octavas
    Layer.Octaves = Stream.RandRange(3, 6);

    // Lacunarity 
    Layer.Lacunarity = Stream.FRandRange(1.8f, 2.5f);

    // Persistencia 
    Layer.Persistence = Stream.FRandRange(0.4f, 0.7f);

    // Amplitud: proporcional al radio del planeta
    Layer.Amplitude = PlanetRadius * Stream.FRandRange(0.02f, 0.15f);

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

    if (!GetWorld())
        return;

    FRandomStream Stream(Seed);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const float SystemRadiusKm = VolumeSizeKm.X * 0.5f;

    //Estrella

    ACosmicPlanet* Star = GetWorld()->SpawnActor<ACosmicPlanet>(
        ACosmicPlanet::StaticClass(),
        GetActorLocation(),
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (!Star)
        return;

    float StarRadiusKm = SystemRadiusKm * 0.15f;

    Star->InitPlanet(StarRadiusKm, nullptr,
        GetRandomColor(Stream, 50, 255),
        GetRandomColor(Stream, 50, 255),
        GetRandomColor(Stream, 50, 255),
        Stream.FRandRange(0.5f, 2.f),
        StarMaterial,
        nullptr);

    Star->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

    UCosmicGravityComponent* StarGravity = NewObject<UCosmicGravityComponent>(Star);
    StarGravity->RegisterComponent();
    StarGravity->SetIsPlanet(true);
    StarGravity->RadiusKm = StarRadiusKm;
    StarGravity->SurfaceGravity = 274.0f;
    StarGravity->GravityMode = ECosmicGravityMode::None;
    Star->AddInstanceComponent(StarGravity);
    
    GeneratedBodies.Add(Star);

    //Crear luz para la estrella
    ADirectionalLight* NuevaLuz = GetWorld()->SpawnActor<ADirectionalLight>(
        ADirectionalLight::StaticClass(),
        Star->GetActorLocation(),
        Star->GetActorRotation(),
        SpawnParams
    );
    NuevaLuz->AttachToActor(Star, FAttachmentTransformRules::KeepWorldTransform);
    ULightComponent* LightComp = NuevaLuz->GetLightComponent();
    UDirectionalLightComponent* DirectionalLightComp = Cast<UDirectionalLightComponent>(LightComp);
    DirectionalLightComp->AtmosphereSunLightIndex = 0; // Importante para el cielo
    DirectionalLightComp->Intensity = 50.0f;

    GeneratedBodies.Add(NuevaLuz);

    FColor color;

    //Crear planetas

    for (int32 i = 0; i < NumberOfBodies; i++)
    {
        ACosmicPlanet* Planet = GetWorld()->SpawnActor<ACosmicPlanet>(
            ACosmicPlanet::StaticClass(),
            GetActorLocation(), // posición irrelevante
            FRotator::ZeroRotator,
            SpawnParams
        );

        if (!Planet)
            continue;

        Planet->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

        // Distancia orbital
        float OrbitDistanceKm = Stream.FRandRange(StarRadiusKm * 3.0f, SystemRadiusKm);

        // Radio proporcional a distancia
        float PlanetRadiusKm = OrbitDistanceKm * Stream.FRandRange(0.01f, 0.05f);

        UTexture2D* TexturaElegida = nullptr;

        if (PosiblesTexturas.Num() > 0) {
            int32 RandomIndex = FMath::RandRange(0, PosiblesTexturas.Num() - 1);

            TexturaElegida = PosiblesTexturas[RandomIndex];
        }
        

        Planet->InitPlanet(PlanetRadiusKm, CreateRandomNoiseSettings(Stream, PlanetRadiusKm),
            GetRandomColor(Stream, 50, 255),
            GetRandomColor(Stream, 50, 255),
            GetRandomColor(Stream, 50, 255),
            Stream.FRandRange(0.5f, 2.f),
            BaseMaterial,
            TexturaElegida
        );

        

        /* Gravity */

        UCosmicGravityComponent* Gravity = NewObject<UCosmicGravityComponent>(Planet);
        Gravity->RegisterComponent();
        Gravity->SetIsPlanet(true);
        Gravity->RadiusKm = PlanetRadiusKm;
        Gravity->SurfaceGravity = Stream.FRandRange(3.0f, 25.0f);
        Gravity->GravityMode = ECosmicGravityMode::None;

        Planet->AddInstanceComponent(Gravity);

        /* Orbit */

        UCosmicOrbitComponent* Orbit = NewObject<UCosmicOrbitComponent>(Planet);
        Orbit->RegisterComponent();

        Orbit->ParentBody = Star;
        Orbit->SemiMajorAxisKm = OrbitDistanceKm;
        Orbit->Eccentricity = Stream.FRandRange(0.0f, 0.15f);
        Orbit->InclinationX = Stream.FRandRange(0.0f, 10.0f);
        Orbit->InitialPosition = Stream.FRandRange(0.0f, 1.0f);

        UE_LOG(LogTemp, Warning,
            TEXT("OrbitDistance: %f"),
            OrbitDistanceKm);

        // Aproximación simplificada Kepler
        Orbit->OrbitalPeriod = FMath::Pow(OrbitDistanceKm, 3);

        float Hue = Stream.FRandRange(0.f, 360.f);
        float Saturation = 0.8f;
        float Value = 1.0f;

        color = FColor(
            Stream.RandRange(50, 255),  // R
            Stream.RandRange(50, 255),  // G
            Stream.RandRange(50, 255),  // B
            255
        );

        Orbit->InitOrbit(color);

        Planet->AddInstanceComponent(Orbit);

        GeneratedBodies.Add(Planet);

        //Generar lunas

        int32 MoonCount = Stream.RandRange(0, 4);

        for (int32 m = 0; m < MoonCount; m++)
        {
            ACosmicPlanet* Moon = GetWorld()->SpawnActor<ACosmicPlanet>(
                ACosmicPlanet::StaticClass(),
                Planet->GetActorLocation(),
                FRotator::ZeroRotator,
                SpawnParams
            );

            if (!Moon)
                continue;

            Moon->AttachToActor(Planet, FAttachmentTransformRules::KeepWorldTransform);

            float MoonOrbitKm = PlanetRadiusKm * Stream.FRandRange(10.0f, 15.0f);

            float MoonRadiusKm = PlanetRadiusKm * Stream.FRandRange(0.1f, 0.3f);

            Moon->InitPlanet(MoonRadiusKm, CreateRandomNoiseSettings(Stream, MoonRadiusKm),
                GetRandomColor(Stream, 50, 255),
                GetRandomColor(Stream, 50, 255),
                GetRandomColor(Stream, 50, 255),
                Stream.FRandRange(0.5f, 2.f),
                BaseMaterial,
                nullptr
            );

            UCosmicGravityComponent* MoonGravity = NewObject<UCosmicGravityComponent>(Moon);

            MoonGravity->RegisterComponent();
            MoonGravity->SetIsPlanet(true);
            MoonGravity->RadiusKm = MoonRadiusKm;
            MoonGravity->SurfaceGravity =
                Stream.FRandRange(1.0f, 5.0f);

            Moon->AddInstanceComponent(MoonGravity);

            UCosmicOrbitComponent* MoonOrbit = NewObject<UCosmicOrbitComponent>(Moon);

            MoonOrbit->RegisterComponent();
            MoonOrbit->ParentBody = Planet;
            MoonOrbit->SemiMajorAxisKm = MoonOrbitKm;
            MoonOrbit->Eccentricity = Stream.FRandRange(0.0f, 0.1f);
            MoonOrbit->InitialPosition = Stream.FRandRange(0.0f, 1.0f);

            MoonOrbit->OrbitalPeriod = FMath::Pow(MoonOrbitKm, 8);

            color = FColor(
                Stream.RandRange(50, 255),  // R
                Stream.RandRange(50, 255),  // G
                Stream.RandRange(50, 255),  // B
                255
            );

            MoonOrbit->InitOrbit(color);

            Moon->AddInstanceComponent(MoonOrbit);

            GeneratedBodies.Add(Moon);
        }
    }

    UE_LOG(LogTemp, Display,
        TEXT("System Generation Complete. Bodies: %d"),
        GeneratedBodies.Num());
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