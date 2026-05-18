// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicBenchmarkManager.h"
#include "CosmicBenchmarkRecorder.h"
#include "CosmicBenchmarkSimBody.h"
#include "Simulation/CosmicGravitySubsystem.h"
#include "System/CosmicSystemGenerator.h"
#include "CosmicFoliageCollection.h"
#include "Engine/World.h"
#include "Planet/CosmicPlanet.h"
#include "Kismet/GameplayStatics.h"
#include "CosmicNoiseClass.h"
#include "TimerManager.h"
#include "Materials/MaterialInstance.h"
#include "GameFramework/Pawn.h"
#include "Components/StaticMeshComponent.h"

UCosmicBenchmarkManager* UCosmicBenchmarkManager::Get(UWorld* World)
{
    if (!World) return nullptr;
    return World->GetSubsystem<UCosmicBenchmarkManager>();
}

void UCosmicBenchmarkManager::Tick(float DeltaTime)
{
    if (bIsCapturing)
    {
        // Registrar datos de este frame
        FCosmicBenchmarkRecorder::RecordFrame(DeltaTime);

        // Acumular tiempo transcurrido
        AccumulatedCaptureTime += DeltaTime;

        // Verificar si se alcanzó la duración de captura
        if (AccumulatedCaptureTime >= CaptureDuration)
        {
            OnBenchmarkCaptureComplete();
        }
    }
}

void UCosmicBenchmarkManager::InitializeAssets(UMaterialInstance* InBaseMaterial, UMaterialInstance* InMoonMaterial,
    UMaterialInstance* InOceanMaterial, UMaterialInstance* InStarMaterial, UMaterialInstance* InGasGiantMaterial,
    UMaterialInstance* InRingMaterial, UCosmicNoiseClass* InNoiseClass, UCosmicFoliageCollection* InFoliageCollection)
{
    BaseMaterial = InBaseMaterial;
    MoonMaterial = InMoonMaterial;
    OceanMaterial = InOceanMaterial;
    StarMaterial = InStarMaterial;
    GasGiantMaterial = InGasGiantMaterial;
    RingMaterial = InRingMaterial;
    NoiseClass = InNoiseClass;
    FoliageCollection = InFoliageCollection;

    UE_LOG(LogTemp, Log, TEXT("BenchmarkManager: Assets inicializados"));
}

void UCosmicBenchmarkManager::SetPlanetConfig(const FBenchmarkPlanetConfig& InConfig)
{
    CurrentPlanetConfig = InConfig;

    UE_LOG(LogTemp, Log, TEXT("[BenchmarkManager] PlanetConfig actualizado:"));
    UE_LOG(LogTemp, Log, TEXT("  RadiusKm=%.1f | SpawnCenter=(%.0f,%.0f,%.0f)"),
        InConfig.RadiusKm,
        InConfig.SpawnCenter.X, InConfig.SpawnCenter.Y, InConfig.SpawnCenter.Z);
    UE_LOG(LogTemp, Log, TEXT("  Ocean=%s (SeaLevel=%.2f, OceanRes=%d)"),
        InConfig.bHasOcean ? TEXT("ON") : TEXT("OFF"),
        InConfig.OceanSeaLevel, InConfig.OceanClipmapResolution);
    UE_LOG(LogTemp, Log, TEXT("  Foliage=%s | Capture=%.1fs | Stabilize=%.1fs"),
        InConfig.bUseFoliageByDefault ? TEXT("ON") : TEXT("OFF"),
        InConfig.CaptureDurationSeconds, InConfig.StabilizationDelaySeconds);
}

void UCosmicBenchmarkManager::RunAllTests()
{
    if (bIsRunningAllTests)
    {
        UE_LOG(LogTemp, Warning, TEXT("All tests already running"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("  STARTING ALL BENCHMARK TESTS"));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT(""));

    bIsRunningAllTests = true;
    AllTestsCurrentIndex = 0;
    RunNextAllTest();
}

void UCosmicBenchmarkManager::RunNextAllTest()
{
    if (!bIsRunningAllTests) return;

    // Si ya terminamos todos los tests
    if (AllTestsCurrentIndex >= 9)
    {
        bIsRunningAllTests = false;
        UE_LOG(LogTemp, Warning, TEXT(""));
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        UE_LOG(LogTemp, Warning, TEXT("  ALL BENCHMARK TESTS COMPLETED!"));
        UE_LOG(LogTemp, Warning, TEXT("========================================"));
        UE_LOG(LogTemp, Warning, TEXT(""));
        return;
    }

    // Limpiar estado previo
    StopBenchmark();
    ClearPlanets();
    ClearSimBodies();

    UE_LOG(LogTemp, Warning, TEXT("----------------------------------------"));
    UE_LOG(LogTemp, Warning, TEXT("  Test %d/9"), AllTestsCurrentIndex + 1);
    UE_LOG(LogTemp, Warning, TEXT("----------------------------------------"));

    // Ejecutar el test correspondiente
    switch (AllTestsCurrentIndex)
    {
    case 0: RunPlanetScalingTest(); break;
    case 1: RunClosePlanetTest(); break;
    case 2: RunFoliagePerFrameTest(); break;
    case 3: RunFoliageRadiusTest(); break;
    case 4: RunClipmapResolutionTest(); break;
    case 5: RunClipmapLevelsTest(); break;
    case 6: RunOrbitSimulationTest(); break;
    case 7: RunNBodySimulationTest(); break;
    case 8:
        // SystemGeneratorTest es síncrono, avanzar inmediatamente después
        RunSystemGeneratorTest();
        AllTestsCurrentIndex++;
        OnAllTestsStepComplete();
        return; // No continuar, OnAllTestsStepComplete ya programa el siguiente
    }

    // No incrementar el índice aquí, se incrementará cuando el test secuencial termine
}

// Este método se llama cuando un test secuencial termina
void UCosmicBenchmarkManager::OnAllTestsStepComplete()
{
    if (!bIsRunningAllTests) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // Pequeño delay para estabilización entre tests
    World->GetTimerManager().SetTimer(
        AllTestsTimerHandle,
        this,
        &UCosmicBenchmarkManager::RunNextAllTest,
        2.0f,  // 2 segundos de pausa entre tests
        false
    );
}

void UCosmicBenchmarkManager::StartBenchmark()
{
    UE_LOG(LogTemp, Warning, TEXT("Benchmark started"));
}

void UCosmicBenchmarkManager::StopBenchmark()
{
    // Detener test secuencial si está corriendo
    if (bIsRunningSequentialTest)
    {
        bIsRunningSequentialTest = false;
        UWorld* World = GetWorld();
        if (World)
        {
            World->GetTimerManager().ClearTimer(SequentialTestTimerHandle);
        }
    }

    // Detener captura
    if (bIsCapturing)
    {
        EndCapture();
    }

    UE_LOG(LogTemp, Warning, TEXT("Benchmark finished"));
}

// Clipmap Config
void UCosmicBenchmarkManager::SetClipmapConfig(int32 BaseRes, int32 Levels)
{
    CurrentClipmapConfig.BaseResolution = BaseRes;
    CurrentClipmapConfig.NumLevels = Levels;

    UE_LOG(LogTemp, Warning, TEXT("Clipmap updated: Res=%d Levels=%d"),
        BaseRes, Levels);
}


// Spawn / Clear Planets
void UCosmicBenchmarkManager::SpawnPlanets(int32 NumPlanets, bool UseFoliage)
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FBenchmarkPlanetConfig& PC = CurrentPlanetConfig;
    bool bFoliage = UseFoliage || PC.bUseFoliageByDefault;

    UE_LOG(LogTemp, Warning, TEXT("Spawning %d planets (R=%.1f km, Ocean=%s, Foliage=%s)"),
        NumPlanets, PC.RadiusKm,
        PC.bHasOcean ? TEXT("ON") : TEXT("OFF"),
        bFoliage ? TEXT("ON") : TEXT("OFF"));

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for (int32 i = 0; i < NumPlanets; i++)
    {
        FVector Location = PC.SpawnCenter + FVector(i * PC.SpawnSpacingCm, 0.f, 0.f);

        ACosmicPlanet* Planet = World->SpawnActor<ACosmicPlanet>(
            ACosmicPlanet::StaticClass(), Location, FRotator::ZeroRotator, Params);

        if (!Planet) continue;

        if (bFoliage)
        {
            Planet->SetFoliageParams(
                CurrentFoliageConfig.FoliageInstancesPerFrame,
                CurrentFoliageConfig.NearLayerRadiusKm,
                CurrentFoliageConfig.MediumLayerRadiusKm,
                CurrentFoliageConfig.FarLayerRadiusKm
            );
        }

        Planet->InitPlanet(
            PC.RadiusKm,
            NoiseClass,
            FColor::Red, FColor::Orange, FColor::White, FColor::Red, FColor::Black,
            100.f, 3.f, 1.f,
            BaseMaterial, nullptr,
            true,
            CurrentClipmapConfig.BaseResolution,
            CurrentClipmapConfig.NumLevels,
            100, 5.0f,
            PC.bHasOcean, PC.OceanSeaLevel, PC.OceanClipmapResolution, OceanMaterial,
            bFoliage ? FoliageCollection : nullptr
        );
    }
}

void UCosmicBenchmarkManager::SpawnPlanetsNear(int32 NumPlanets, bool UseFoliage)
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FBenchmarkPlanetConfig& PC = CurrentPlanetConfig;
    bool bFoliage = UseFoliage || PC.bUseFoliageByDefault;

    // Obtener posición de cámara (o usar SpawnCenter si no hay cámara)
    FVector Origin = PC.SpawnCenter;
    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (PlayerController)
    {
        if (APawn* Pawn = PlayerController->GetPawn())
            Origin = Pawn->GetActorLocation();
    }

    UE_LOG(LogTemp, Warning, TEXT("Spawning %d planets near (R=%.1f km, Ocean=%s)"),
        NumPlanets, PC.RadiusKm, PC.bHasOcean ? TEXT("ON") : TEXT("OFF"));

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for (int32 i = 0; i < NumPlanets; i++)
    {
        float Angle = (360.0f / FMath::Max(1, NumPlanets)) * i;
        float Radians = FMath::DegreesToRadians(Angle);

        FVector Location = Origin + FVector(
            FMath::Cos(Radians) * PC.NearSpawnRadiusCm,
            FMath::Sin(Radians) * PC.NearSpawnRadiusCm,
            0.0f
        );

        ACosmicPlanet* Planet = World->SpawnActor<ACosmicPlanet>(
            ACosmicPlanet::StaticClass(), Location, FRotator::ZeroRotator, Params);

        if (!Planet) continue;

        if (bFoliage)
        {
            Planet->SetFoliageParams(
                CurrentFoliageConfig.FoliageInstancesPerFrame,
                CurrentFoliageConfig.NearLayerRadiusKm,
                CurrentFoliageConfig.MediumLayerRadiusKm,
                CurrentFoliageConfig.FarLayerRadiusKm
            );
        }

        Planet->InitPlanet(
            PC.RadiusKm,
            NoiseClass,
            FColor::Red, FColor::Orange, FColor::White, FColor::Red, FColor::Black,
            100.f, 3.f, 1.f,
            BaseMaterial, nullptr,
            true,
            CurrentClipmapConfig.BaseResolution,
            CurrentClipmapConfig.NumLevels,
            100, 5.0f,
            PC.bHasOcean, PC.OceanSeaLevel, PC.OceanClipmapResolution, OceanMaterial,
            bFoliage ? FoliageCollection : nullptr
        );
    }
}

void UCosmicBenchmarkManager::SpawnPlanetsFar(int32 NumPlanets, bool UseFoliage)
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FBenchmarkPlanetConfig& PC = CurrentPlanetConfig;
    bool bFoliage = UseFoliage || PC.bUseFoliageByDefault;

    UE_LOG(LogTemp, Warning, TEXT("Spawning %d far planets (R=%.1f km)"),
        NumPlanets, PC.RadiusKm);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for (int32 i = 0; i < NumPlanets; i++)
    {
        FVector Location = PC.SpawnCenter + FVector(i * PC.FarSpawnSpacingCm, 0.f, 0.f);

        ACosmicPlanet* Planet = World->SpawnActor<ACosmicPlanet>(
            ACosmicPlanet::StaticClass(), Location, FRotator::ZeroRotator, Params);

        if (!Planet) continue;

        Planet->InitPlanet(
            PC.RadiusKm,
            NoiseClass,
            FColor::Red, FColor::Orange, FColor::White, FColor::Red, FColor::Black,
            100.f, 3.f, 1.f,
            BaseMaterial, nullptr,
            true,
            CurrentClipmapConfig.BaseResolution,
            CurrentClipmapConfig.NumLevels,
            100, 5.0f,
            PC.bHasOcean, PC.OceanSeaLevel, PC.OceanClipmapResolution, OceanMaterial,
            bFoliage ? FoliageCollection : nullptr
        );
    }
}

void UCosmicBenchmarkManager::ClearPlanets()
{
    UWorld* World = GetWorld();
    if (!World) return;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, ACosmicPlanet::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        Actor->Destroy();
    }

    UE_LOG(LogTemp, Warning, TEXT("Cleared %d planets"), FoundActors.Num());
}

void UCosmicBenchmarkManager::RunPlanetScalingTest()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Planet Scaling Test (Sequential) ==="));

    if (bIsRunningSequentialTest)
    {
        UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
        return;
    }

    ClearSimBodies();
    ClearPlanets();

    FCosmicBenchmarkRecorder::ClearCSVResults();

    // Configurar test secuencial
    SequentialTestSteps = { 1, 2, 4, 8, 16, 32, 64 };
    CurrentSequentialStepIndex = 0;
    CurrentSequentialTestType = ESequentialTestType::PlanetScaling;
    bIsRunningSequentialTest = true;

    // Iniciar primer paso
    RunNextSequentialStep();
}

void UCosmicBenchmarkManager::RunNextSequentialStep()
{
    if (!bIsRunningSequentialTest) return;

    // Verificar si ya terminamos todos los pasos
    if (CurrentSequentialStepIndex >= SequentialTestSteps.Num())
    {
        OnSequentialTestComplete();
        return;
    }

    int32 CurrentStep = SequentialTestSteps[CurrentSequentialStepIndex];

    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("--- Step %d/%d: Testing with %d ---"),
        CurrentSequentialStepIndex + 1, SequentialTestSteps.Num(), CurrentStep);

    // Limpiar planetas anteriores
    ClearSimBodies();
    ClearPlanets();



    // Generar planetas según el tipo de test
    switch (CurrentSequentialTestType)
    {
    case ESequentialTestType::PlanetScaling:
        SpawnPlanets(CurrentStep);
        break;

    case ESequentialTestType::ClosePlanetScaling:
        SpawnPlanetsNear(CurrentStep);
        break;
    case ESequentialTestType::FoliagePerFrame:
        SetFoliageConfig(CurrentStep);
        SpawnPlanets(1, true);
        break;
    case ESequentialTestType::FoliageViewDistance:
        if (CurrentStep < RadiusConfigs.Num())
        {
            FVector Radii = RadiusConfigs[CurrentStep];
            SetFoliageConfig(100, Radii.X, Radii.Y, Radii.Z);
            SpawnPlanets(1, true);
        }
        break;
    case ESequentialTestType::ClipmapResolution:
    {
        // Actualizar configuración de clipmap
        SetClipmapConfig(CurrentStep, CurrentClipmapConfig.NumLevels);
        SpawnPlanetsNear(1, false); // Solo 1 planeta para test de resolución
        break;
    }
    case ESequentialTestType::ClipmapLevels:
    {
        // Actualizar niveles manteniendo resolución base
        SetClipmapConfig(CurrentClipmapConfig.BaseResolution, CurrentStep);
        SpawnPlanetsNear(1, false);
        break;
    }
    case ESequentialTestType::OrbitSimulation:
        SpawnSimBodies(CurrentStep, false); // Órbitas elípticas
        break;

    case ESequentialTestType::NBodySimulation:
        SpawnSimBodies(CurrentStep, true); // N-Body completo
        break;

    default:
        break;
    }

    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().SetTimer(
            SequentialTestTimerHandle,
            [this]()
            {
                int32 CurrentStep = SequentialTestSteps[CurrentSequentialStepIndex];
                FString TestName;

                switch (CurrentSequentialTestType)
                {
                case ESequentialTestType::PlanetScaling:
                    TestName = FString::Printf(TEXT("PlanetScaling_%d"), CurrentStep);
                    break;
                case ESequentialTestType::ClosePlanetScaling:
                    TestName = FString::Printf(TEXT("ClosePlanet_%d"), CurrentStep);
                    break;
                case ESequentialTestType::FoliagePerFrame:
                    TestName = FString::Printf(TEXT("FoliagePerFrame_%d"), CurrentStep);
                    break;
                case ESequentialTestType::FoliageViewDistance:
                    TestName = FString::Printf(TEXT("FoliageRadius_%d"), CurrentStep);
                    break;
                case ESequentialTestType::ClipmapResolution:
                    TestName = FString::Printf(TEXT("ClipmapRes_%d"), CurrentStep);
                    break;
                case ESequentialTestType::ClipmapLevels:
                    TestName = FString::Printf(TEXT("ClipmapLevels_%d"), CurrentStep);
                    break;
                case ESequentialTestType::OrbitSimulation:
                    TestName = FString::Printf(TEXT("OrbitSim_%d"), CurrentStep);
                    break;
                case ESequentialTestType::NBodySimulation:
                    TestName = FString::Printf(TEXT("NBodySim_%d"), CurrentStep);
                    break;
                default:
                    TestName = FString::Printf(TEXT("Test_%d"), CurrentStep);
                    break;
                }

                SetCurrentTestParams(CurrentStep, TestName);
                BeginCapture(8.0f);
            },
            CurrentPlanetConfig.StabilizationDelaySeconds, // Tiempo de estabilización
            false
        );
    }
}

void UCosmicBenchmarkManager::OnSequentialTestComplete()
{
    bIsRunningSequentialTest = false;
    SequentialTestSteps.Empty();

    ClearPlanets();
    ClearSimBodies();

    FString CSVName;

    switch (CurrentSequentialTestType)
    {
    case ESequentialTestType::PlanetScaling:
        CSVName = TEXT("PlanetScaling");
        break;

    case ESequentialTestType::ClosePlanetScaling:
        CSVName = TEXT("ClosePlanetScaling");
        break;

    case ESequentialTestType::FoliagePerFrame:
        CSVName = TEXT("FoliagePerFrame");
        break;

    case ESequentialTestType::FoliageViewDistance:
        CSVName = TEXT("FoliageRadius");
        break;

    case ESequentialTestType::ClipmapResolution:
        CSVName = TEXT("ClipmapResolution");
        break;

    case ESequentialTestType::ClipmapLevels:
        CSVName = TEXT("ClipmapLevels");
        break;

    case ESequentialTestType::OrbitSimulation:
        CSVName = TEXT("OrbitSimulation");
        break;

    case ESequentialTestType::NBodySimulation:
        CSVName = TEXT("NBodySimulation");
        break;

    default:
        CSVName = TEXT("Benchmark");
        break;
    }

    CurrentSequentialTestType = ESequentialTestType::None;

    CSVName += TEXT("_") + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));

    FCosmicBenchmarkRecorder::ExportCSV(CSVName);

    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("=== Sequential Test Complete ==="));

    if (bIsRunningAllTests)
    {
        AllTestsCurrentIndex++;
        OnAllTestsStepComplete();
    }
}


// Captura de Métricas
void UCosmicBenchmarkManager::BeginCapture(float DurationSeconds)
{
    float ActualDuration = (DurationSeconds > 0.f)
        ? DurationSeconds
        : CurrentPlanetConfig.CaptureDurationSeconds;

    FCosmicBenchmarkRecorder::StartRecording();

    bIsCapturing = true;
    CaptureDuration = ActualDuration;
    AccumulatedCaptureTime = 0.0f;

    UE_LOG(LogTemp, Warning, TEXT("Capture Start — Duration: %.1f s — Test: %s"),
        ActualDuration, *CurrentTestName);
}

void UCosmicBenchmarkManager::EndCapture()
{
    bIsCapturing = false;
    FCosmicBenchmarkRecorder::StopRecording();
    FCosmicBenchmarkRecorder::LogCurrentData(CurrentTestName);
}

void UCosmicBenchmarkManager::SetCurrentTestParams(int32 NumObjects, const FString& TestName)
{
    CurrentNumObjects = NumObjects;
    CurrentTestName = TestName;

    FCosmicBenchmarkRecorder::SetCurrentNumObjects(NumObjects);
    FCosmicBenchmarkRecorder::SetCurrentTestName(TestName);
}

void UCosmicBenchmarkManager::RecordEvent(const FString& EventName, const FString& Description, float NumericValue)
{
    FCosmicBenchmarkRecorder::RecordEvent(EventName, Description, NumericValue);
}

void UCosmicBenchmarkManager::OnWorldEndPlay(UWorld& InWorld)
{
    EndCapture();

    Super::OnWorldEndPlay(InWorld);
}

void UCosmicBenchmarkManager::OnBenchmarkCaptureComplete()
{
    bIsCapturing = false;

    // Detener y mostrar resultados
    FCosmicBenchmarkRecorder::StopRecording();
    FCosmicBenchmarkRecorder::LogCurrentData(CurrentTestName);
    FCosmicBenchmarkRecorder::AddCSVResult(CurrentTestName);

    UE_LOG(LogTemp, Warning, TEXT("Capture completed for test: %s (%.1f seconds)"),
        *CurrentTestName, AccumulatedCaptureTime);



    // Si estamos en un test secuencial, programar el siguiente paso
    if (bIsRunningSequentialTest)
    {
        CurrentSequentialStepIndex++;

        UWorld* World = GetWorld();
        if (World)
        {
            // Pequeña pausa entre pasos (0.5 segundos)
            World->GetTimerManager().SetTimer(
                SequentialTestTimerHandle,
                this,
                &UCosmicBenchmarkManager::RunNextSequentialStep,
                0.5f,
                false
            );
        }
    }
}

// Tests individuales (no secuenciales)
void UCosmicBenchmarkManager::RunClosePlanetTest()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Close Planet Test ==="));

    SequentialTestSteps = { 1, 2, 4, 8, 16 };
    CurrentSequentialStepIndex = 0;
    CurrentSequentialTestType = ESequentialTestType::ClosePlanetScaling;
    bIsRunningSequentialTest = true;

    FCosmicBenchmarkRecorder::ClearCSVResults();

    RunNextSequentialStep();
}


void UCosmicBenchmarkManager::SetFoliageConfig(int32 InFoliageInstancesPerFrame, float NearLayerRadiusKm, float MediumLayerRadiusKm, float FarLayerRadiusKm)
{
    CurrentFoliageConfig.FoliageInstancesPerFrame = InFoliageInstancesPerFrame;
    CurrentFoliageConfig.NearLayerRadiusKm = NearLayerRadiusKm;
    CurrentFoliageConfig.MediumLayerRadiusKm = MediumLayerRadiusKm;
    CurrentFoliageConfig.FarLayerRadiusKm = FarLayerRadiusKm;
}

void UCosmicBenchmarkManager::RunFoliagePerFrameTest(int32 MaxInstancesPerFrame)
{
    FCosmicBenchmarkRecorder::ClearCSVResults();

    if (MaxInstancesPerFrame <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== Foliage Per Frame Test (Sequential) ==="));

        if (bIsRunningSequentialTest)
        {
            UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
            return;
        }

        // Pasos de instancias por frame
        SequentialTestSteps = { 10, 50, 100, 200, 500, 1000 };
        CurrentSequentialStepIndex = 0;
        CurrentSequentialTestType = ESequentialTestType::FoliagePerFrame;
        bIsRunningSequentialTest = true;

        RunNextSequentialStep();
    }
    else
    {
        // Test individual
        UE_LOG(LogTemp, Warning, TEXT("=== Foliage Per Frame Test: %d instances/frame ==="), MaxInstancesPerFrame);

        ClearSimBodies();
        ClearPlanets();

        SetFoliageConfig(MaxInstancesPerFrame);

        // Spawnear un planeta cerca
        SpawnPlanetsNear(1);

        SetCurrentTestParams(MaxInstancesPerFrame, FString::Printf(TEXT("FoliagePerFrame_%d"), MaxInstancesPerFrame));
        BeginCapture(5.0f);
    }
}

void UCosmicBenchmarkManager::RunFoliageRadiusTest()
{
    FCosmicBenchmarkRecorder::ClearCSVResults();

    UE_LOG(LogTemp, Warning, TEXT("=== Foliage Radius Test (Sequential) ==="));

    if (bIsRunningSequentialTest)
    {
        UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
        return;
    }

    RadiusConfigs = {
            FVector(0.01f, 0.05f, 0.1f),   // Muy cercano
            FVector(0.02f, 0.1f, 0.2f),     // Cercano
            FVector(0.05f, 0.2f, 0.5f),     // Default
            FVector(0.1f, 0.4f, 1.0f),      // Medio
            FVector(0.2f, 0.8f, 2.0f),      // Lejos
            FVector(0.5f, 2.0f, 5.0f)       // Muy lejos
    };

    // Este test variará los radios de las capas
    SequentialTestSteps = { 0, 1, 2, 3, 4, 5 }; // Índices para diferentes configuraciones
    CurrentSequentialStepIndex = 0;
    CurrentSequentialTestType = ESequentialTestType::FoliageViewDistance;
    bIsRunningSequentialTest = true;

    RunNextSequentialStep();
}

void UCosmicBenchmarkManager::RunClipmapResolutionTest(int32 Resolution)
{
    FCosmicBenchmarkRecorder::ClearCSVResults();

    if (Resolution <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== Clipmap Resolution Test (Sequential) ==="));

        if (bIsRunningSequentialTest)
        {
            UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
            return;
        }

        // Pasos de resolución a probar
        SequentialTestSteps = { 8, 16, 32, 64, 128, 256 };
        CurrentSequentialStepIndex = 0;
        CurrentSequentialTestType = ESequentialTestType::ClipmapResolution;
        bIsRunningSequentialTest = true;

        RunNextSequentialStep();
    }
    else
    {
        // Test individual con una resolución específica
        UE_LOG(LogTemp, Warning, TEXT("=== Clipmap Resolution Test: %d ==="), Resolution);

        SetClipmapConfig(Resolution, CurrentClipmapConfig.NumLevels);
        ClearSimBodies();
        ClearPlanets();
        SpawnPlanetsNear(1, false);
        SetCurrentTestParams(Resolution, FString::Printf(TEXT("ClipmapRes_%d"), Resolution));
        BeginCapture(5.0f);
    }
}

void UCosmicBenchmarkManager::RunClipmapLevelsTest(int32 Levels)
{
    FCosmicBenchmarkRecorder::ClearCSVResults();

    // Si se llama sin argumento (Levels=0), ejecutar test secuencial automático
    if (Levels <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== Clipmap Levels Test (Sequential) ==="));

        if (bIsRunningSequentialTest)
        {
            UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
            return;
        }

        SequentialTestSteps = { 1, 2, 4, 6, 8 };
        CurrentSequentialStepIndex = 0;
        CurrentSequentialTestType = ESequentialTestType::ClipmapLevels;
        bIsRunningSequentialTest = true;

        RunNextSequentialStep();
    }
    else
    {
        // Test individual con un número específico de niveles
        UE_LOG(LogTemp, Warning, TEXT("=== Clipmap Levels Test: %d levels ==="), Levels);

        SetClipmapConfig(CurrentClipmapConfig.BaseResolution, Levels);
        ClearSimBodies();
        ClearPlanets();
        SpawnPlanetsNear(1);
        SetCurrentTestParams(Levels, FString::Printf(TEXT("ClipmapLevels_%d"), Levels));
        BeginCapture(5.0f);
    }
}

void UCosmicBenchmarkManager::RunOrbitSimulationTest(int32 NumBodies)
{

    FCosmicBenchmarkRecorder::ClearCSVResults();

    // Si NumBodies <= 0, ejecutar test secuencial
    if (NumBodies <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== Orbit Simulation Test (Sequential) ==="));

        if (bIsRunningSequentialTest)
        {
            UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
            return;
        }
        SequentialTestSteps = { 10, 50, 100, 200, 500, 1000 };
        CurrentSequentialStepIndex = 0;
        CurrentSequentialTestType = ESequentialTestType::OrbitSimulation;
        bIsRunningSequentialTest = true;

        RunNextSequentialStep();
    }
    else
    {
        // Test individual
        UE_LOG(LogTemp, Warning, TEXT("=== Orbit Simulation Test: %d bodies ==="), NumBodies);

        ClearSimBodies();
        SpawnSimBodies(NumBodies, false); // false = órbitas simples

        SetCurrentTestParams(NumBodies, FString::Printf(TEXT("OrbitSim_%d"), NumBodies));
        BeginCapture(5.0f);
    }
}

void UCosmicBenchmarkManager::RunNBodySimulationTest(int32 NumBodies)
{

    FCosmicBenchmarkRecorder::ClearCSVResults();

    // Si NumBodies <= 0, ejecutar test secuencial
    if (NumBodies <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== NBody Simulation Test (Sequential) ==="));

        if (bIsRunningSequentialTest)
        {
            UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
            return;
        }

        // Pasos: 10, 50, 100, 200, 500, 1000, 2000 cuerpos
        SequentialTestSteps = { 10, 50, 100, 200, 500, 1000, 2000 };
        CurrentSequentialStepIndex = 0;
        CurrentSequentialTestType = ESequentialTestType::NBodySimulation;
        bIsRunningSequentialTest = true;

        RunNextSequentialStep();
    }
    else
    {
        // Test individual
        UE_LOG(LogTemp, Warning, TEXT("=== NBody Simulation Test: %d bodies ==="), NumBodies);

        ClearSimBodies();
        SpawnSimBodies(NumBodies, true); // nbody

        SetCurrentTestParams(NumBodies, FString::Printf(TEXT("NBodySim_%d"), NumBodies));
        BeginCapture(5.0f);
    }
}

void UCosmicBenchmarkManager::RunSystemGeneratorTest(int32 NumBodies)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (SystemGenerator)
    {
        SystemGenerator->ClearBodies();
    }
    else
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        SystemGenerator = World->SpawnActor<ACosmicSystemGenerator>(
            ACosmicSystemGenerator::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            Params);

        if (!SystemGenerator) return;

        SystemGenerator->BaseMaterial = BaseMaterial;
        SystemGenerator->MoonMaterial = MoonMaterial;
        SystemGenerator->StarMaterial = StarMaterial;
        SystemGenerator->GasGiantMaterial = GasGiantMaterial;
        SystemGenerator->OceanMaterial = OceanMaterial;
        SystemGenerator->RingMaterial = RingMaterial;
    }

    ClearSimBodies();
    ClearPlanets();

    if (NumBodies <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== System Generation Test ==="));

        SequentialTestSteps = { 1, 2, 5, 10, 20, 50 };

        for (size_t i = 0; i < SequentialTestSteps.Num(); i++)
        {
            SystemGenerator->SetNumBodies(SequentialTestSteps[i]);

            double StartTime = FPlatformTime::Seconds();

            SystemGenerator->GenerateBodies();

            double EndTime = FPlatformTime::Seconds();
            double GenerationTime = EndTime - StartTime;

            UE_LOG(LogTemp, Warning, TEXT("System generation time for %d bodies: %.3f seconds"),
                SequentialTestSteps[i], GenerationTime);

            SystemGenerator->ClearBodies();
        }

    }
    else
    {
        SystemGenerator->SetNumBodies(NumBodies);

        double StartTime = FPlatformTime::Seconds();

        SystemGenerator->GenerateBodies();

        double EndTime = FPlatformTime::Seconds();
        double GenerationTime = EndTime - StartTime;

        UE_LOG(LogTemp, Warning, TEXT("System generation time for %d bodies: %.3f seconds"),
            NumBodies, GenerationTime);

        SystemGenerator->ClearBodies();
    }

    if (SystemGenerator)
    {
        SystemGenerator->Destroy();
        SystemGenerator = nullptr;
    }
}

void UCosmicBenchmarkManager::SpawnSimBodies(int32 NumBodies, bool bNBodySimulation)
{
    UWorld* World = GetWorld();
    if (!World) return;

    UE_LOG(LogTemp, Warning, TEXT("Spawning %d simulation bodies (N-Body: %s)"),
        NumBodies, bNBodySimulation ? TEXT("true") : TEXT("false"));

    SimBodies.Empty();
    SimBodies.Reserve(NumBodies);

    UCosmicGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UCosmicGravitySubsystem>();

    if (!Subsystem && bNBodySimulation) return;

    // Crear cuerpo central
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    CentralBody = World->SpawnActor<ACosmicBenchmarkSimBody>(
        ACosmicBenchmarkSimBody::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        Params
    );

    if (CentralBody)
    {
        CentralBody->InitAsCentralBody();
        // Hacer la esfera central un poco más grande
        if (CentralBody->MeshComponent)
        {
            CentralBody->MeshComponent->SetWorldScale3D(FVector(20.f));
        }
    }

    // Crear cuerpos orbitales
    for (int32 i = 0; i < NumBodies; i++)
    {
        FVector Location = FVector(FMath::FRandRange(-10000.0f, 10000.0f),
            FMath::FRandRange(-10000.0f, 10000.0f),
            FMath::FRandRange(-10000.0f, 10000.0f));
        FRotator Rotation = FRotator::ZeroRotator;

        ACosmicBenchmarkSimBody* Body = World->SpawnActor<ACosmicBenchmarkSimBody>(
            ACosmicBenchmarkSimBody::StaticClass(),
            Location,
            Rotation,
            Params
        );

        if (Body)
        {
            if (bNBodySimulation)
            {
                Body->InitGravityComponent(bNBodySimulation);
                Subsystem->RegisterBody(Body->GravityComponent);
                Body->InitAsCentralBody();
            }
            else
            {
                // Órbita aleatoria alrededor del cuerpo central
                float SemiMajorAxis = FMath::FRandRange(0.3f, 1.5f);
                Body->AttachToActor(CentralBody, FAttachmentTransformRules::KeepWorldTransform);
                Body->InitRandomOrbit(CentralBody, SemiMajorAxis);
            }

            SimBodies.Add(Body);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Spawned %d orbital bodies"), SimBodies.Num());
}

void UCosmicBenchmarkManager::ClearSimBodies()
{
    for (ACosmicBenchmarkSimBody* Body : SimBodies)
    {
        if (Body)
        {
            Body->Destroy();
        }
    }
    SimBodies.Empty();

    // Destruir cuerpo central
    if (CentralBody)
    {
        CentralBody->Destroy();
        CentralBody = nullptr;
    }

    UE_LOG(LogTemp, Warning, TEXT("Cleared all simulation bodies"));
}

