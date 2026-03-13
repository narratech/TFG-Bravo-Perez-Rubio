#include "System/CosmicSystemGenerator.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "CosmicArchitectRuntime/Public/Planet/CosmicPlanet.h"
#include "CosmicArchitectRuntime/Public/Simulation/OrbitComponent.h"
#include "CosmicArchitectRuntime/Public/Simulation/GravityComponent.h"

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
    VolumeSizeKm = FVector(20.0f, 20.0f, 5.0f); // 20 Km
    NumberOfBodies = 10;
    Seed = 12345;

    // E: Rango de diámetro por defecto: entre 100 metros (0.1 km) y 500 metros (0.5 km).
    // I: Default diameter range: between 100 meters (0.1 km) and 500 meters (0.5 km).
    BodyDiameterRangeKm = FVector2D(0.1f, 0.5f);

    MinDistanceBetweenBodies = 1.0f; // 1 Km
    MaxDistanceToNearest = 0.0f;     // 0 = Sin agrupación forzada / No forced clustering       
    MaxGenerationAttempts = 100;

    // E: Buscamos la esfera básica del motor para tener algo asignado por defecto.
    // I: Find the engine's basic sphere to have something assigned by default.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultSphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (DefaultSphereAsset.Succeeded())
    {
        SphereMesh = DefaultSphereAsset.Object;
    }
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

UCosmicNoiseSettings* ACosmicSystemGenerator::CreateRandomNoiseSettings(FRandomStream& Stream, const float PlanetRadius)
{
    UCosmicNoiseSettings* NewSettings = NewObject<UCosmicNoiseSettings>(GetTransientPackage(),NAME_None, RF_Transient);

    // Planetas y lunas: variedad de tipos
    NewSettings->Seed = Stream.RandRange(0, 999999);

    // Altura máxima según tamaño (planetas más grandes pueden tener montañas más altas)
    NewSettings->MaxMountainHeight = Stream.FRandRange(1000.0f, 8000.0f) * PlanetRadius * 0.5f;

    // Distribución de tipos de planetas
    float PlanetType = Stream.FRandRange(0.0f, 1.0f);

    if (PlanetType < 0.2f) // 20% - Planetas desérticos/planos
    {
        NewSettings->Mountainous = Stream.FRandRange(0.1f, 0.3f);
        NewSettings->Roughness = Stream.FRandRange(0.2f, 0.4f);
        NewSettings->Detail = Stream.FRandRange(0.3f, 0.5f);
        NewSettings->Smoothness = Stream.FRandRange(0.6f, 0.9f);
    }
    else if (PlanetType < 0.5f) // 30% - Planetas montañosos
    {
        NewSettings->Mountainous = Stream.FRandRange(0.6f, 0.9f);
        NewSettings->Roughness = Stream.FRandRange(0.5f, 0.8f);
        NewSettings->Detail = Stream.FRandRange(0.6f, 0.9f);
        NewSettings->Smoothness = Stream.FRandRange(0.1f, 0.3f);
    }
    else if (PlanetType < 0.8f) // 30% - Planetas mixtos
    {
        NewSettings->Mountainous = Stream.FRandRange(0.3f, 0.6f);
        NewSettings->Roughness = Stream.FRandRange(0.4f, 0.6f);
        NewSettings->Detail = Stream.FRandRange(0.4f, 0.7f);
        NewSettings->Smoothness = Stream.FRandRange(0.3f, 0.6f);
    }
    else {
        NewSettings->Mountainous = Stream.FRandRange(0.0f, 1.f);
        NewSettings->Roughness = Stream.FRandRange(0.0f, 1.f);
        NewSettings->Detail = Stream.FRandRange(0.3f, 1.f);
        NewSettings->Smoothness = Stream.FRandRange(0.2f, 1.f);
    }

    NewSettings->UpdateAdvancedFromSimple();

    return NewSettings;
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

    Star->InitPlanet(StarRadiusKm, CreateRandomNoiseSettings(Stream, StarRadiusKm));
    Star->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

    UGravityComponent* StarGravity = NewObject<UGravityComponent>(Star);
    StarGravity->RegisterComponent();
    StarGravity->SetIsPlanet(true);
    StarGravity->RadiusKm = StarRadiusKm;
    StarGravity->SurfaceGravity = 274.0f;
    StarGravity->GravityMode = EGravityMode::None;
    Star->AddInstanceComponent(StarGravity);
    
    GeneratedBodies.Add(Star);

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

        Planet->InitPlanet(PlanetRadiusKm, CreateRandomNoiseSettings(Stream, PlanetRadiusKm));

        /* Gravity */

        UGravityComponent* Gravity = NewObject<UGravityComponent>(Planet);
        Gravity->RegisterComponent();
        Gravity->SetIsPlanet(true);
        Gravity->RadiusKm = PlanetRadiusKm;
        Gravity->SurfaceGravity = Stream.FRandRange(3.0f, 25.0f);
        Gravity->GravityMode = EGravityMode::None;

        Planet->AddInstanceComponent(Gravity);

        /* Orbit */

        UOrbitComponent* Orbit = NewObject<UOrbitComponent>(Planet);
        Orbit->RegisterComponent();

        Orbit->ParentBody = Star;
        Orbit->SemiMajorAxisKm = OrbitDistanceKm;
        Orbit->Eccentricity = Stream.FRandRange(0.0f, 0.15f);
        Orbit->InclinationX = Stream.FRandRange(0.0f, 10.0f);
        Orbit->InitialPosition = Stream.FRandRange(0.0f, 1.0f);

        // Aproximación simplificada Kepler
        Orbit->OrbitalPeriod = FMath::Sqrt(FMath::Pow(OrbitDistanceKm, 3) / 1000.0f);

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

            float MoonOrbitKm = PlanetRadiusKm * Stream.FRandRange(5.0f, 15.0f);

            float MoonRadiusKm = PlanetRadiusKm * Stream.FRandRange(0.1f, 0.3f);

            Moon->InitPlanet(MoonRadiusKm, CreateRandomNoiseSettings(Stream, MoonRadiusKm));

            UGravityComponent* MoonGravity = NewObject<UGravityComponent>(Moon);

            MoonGravity->RegisterComponent();
            MoonGravity->SetIsPlanet(true);
            MoonGravity->RadiusKm = MoonRadiusKm;
            MoonGravity->SurfaceGravity =
                Stream.FRandRange(1.0f, 5.0f);

            Moon->AddInstanceComponent(MoonGravity);

            UOrbitComponent* MoonOrbit = NewObject<UOrbitComponent>(Moon);

            MoonOrbit->RegisterComponent();
            MoonOrbit->ParentBody = Planet;
            MoonOrbit->SemiMajorAxisKm = MoonOrbitKm;
            MoonOrbit->Eccentricity = Stream.FRandRange(0.0f, 0.1f);
            MoonOrbit->InitialPosition = Stream.FRandRange(0.0f, 1.0f);

            MoonOrbit->OrbitalPeriod = FMath::Sqrt(FMath::Pow(MoonOrbitKm, 3) / 100.0f);

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