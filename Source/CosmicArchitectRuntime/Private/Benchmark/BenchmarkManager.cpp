// Fill out your copyright notice in the Description page of Project Settings.

#include "Benchmark/BenchmarkManager.h"
#include "Benchmark/BenchmarkRecorder.h"
#include "Benchmark/BenchmarkSimBody.h"
#include "Simulation/CosmicGravitySubsystem.h"
#include "System/CosmicSystemGenerator.h"
#include "CosmicFoliageCollection.h"
#include "Engine/World.h"
#include "Planet/CosmicPlanet.h"
#include "Kismet/GameplayStatics.h"
#include "CosmicNoiseClass.h"
#include "TimerManager.h"

UBenchmarkManager* UBenchmarkManager::Get(UWorld* World)
{
    if (!World) return nullptr;
    return World->GetSubsystem<UBenchmarkManager>();
}

void UBenchmarkManager::Tick(float DeltaTime)
{
    if (bIsCapturing)
    {
        // Registrar datos de este frame
        FBenchmarkRecorder::RecordFrame(DeltaTime);

        // Acumular tiempo transcurrido
        AccumulatedCaptureTime += DeltaTime;

        // Verificar si se alcanzó la duración de captura
        if (AccumulatedCaptureTime >= CaptureDuration)
        {
            OnBenchmarkCaptureComplete();
        }
    }
}

void UBenchmarkManager::InitializeAssets(UMaterialInstance* InBaseMaterial, UMaterialInstance* InMoonMaterial,
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

void UBenchmarkManager::RunAllTests()
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

void UBenchmarkManager::RunNextAllTest()
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

    // NO incrementar el índice aquí
    // Se incrementará cuando el test secuencial termine
}

// Este método se llama cuando un test secuencial termina
void UBenchmarkManager::OnAllTestsStepComplete()
{
    if (!bIsRunningAllTests) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // Pequeño delay para estabilización entre tests
    World->GetTimerManager().SetTimer(
        AllTestsTimerHandle,
        this,
        &UBenchmarkManager::RunNextAllTest,
        2.0f,  // 2 segundos de pausa entre tests
        false
    );
}

void UBenchmarkManager::StartBenchmark()
{
    UE_LOG(LogTemp, Warning, TEXT("Benchmark started"));
}

void UBenchmarkManager::StopBenchmark()
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
void UBenchmarkManager::SetClipmapConfig(int32 BaseRes, int32 Levels)
{
    CurrentClipmapConfig.BaseResolution = BaseRes;
    CurrentClipmapConfig.NumLevels = Levels;

    UE_LOG(LogTemp, Warning, TEXT("Clipmap updated: Res=%d Levels=%d"),
        BaseRes, Levels);
}


// Spawn / Clear Planets
void UBenchmarkManager::SpawnPlanets(int32 NumPlanets, bool UseFoliage)
{
    UWorld* World = GetWorld();
    if (!World) return;

    UE_LOG(LogTemp, Warning, TEXT("Spawning %d planets"), NumPlanets);

    for (int32 i = 0; i < NumPlanets; i++)
    {
        FVector Location = FVector(i * 500000.0f, 0.f, 0.f);
        FRotator Rotation = FRotator::ZeroRotator;

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ACosmicPlanet* Planet = World->SpawnActor<ACosmicPlanet>(
            ACosmicPlanet::StaticClass(),
            Location,
            Rotation,
            Params
        );

        if (!Planet) continue;

        if (UseFoliage)
        {
            Planet->SetFoliageParams(
                CurrentFoliageConfig.FoliageInstancesPerFrame,
                CurrentFoliageConfig.NearLayerRadiusKm,
                CurrentFoliageConfig.MediumLayerRadiusKm,
                CurrentFoliageConfig.FarLayerRadiusKm
            );
        }      

        Planet->InitPlanet(
            10.0f,
            NoiseClass,
            FColor::Red,
            FColor::Orange,
            FColor::White,
            FColor::Red,
            FColor::Black,
            100.f, 3.f, 1.f,
            BaseMaterial, nullptr,
            true,
            CurrentClipmapConfig.BaseResolution,
            CurrentClipmapConfig.NumLevels,
            100, 5.0f,
            true, 0.0, 128, OceanMaterial,
            UseFoliage ? FoliageCollection : nullptr
        );

        
    }
}

void UBenchmarkManager::SpawnPlanetsNear(int32 NumPlanets, bool UseFoliage)
{
    UWorld* World = GetWorld();
    if (!World) return;

    FVector CameraLocation = FVector::ZeroVector;
    APlayerController* PC = World->GetFirstPlayerController();
    if (PC)
    {
        APawn* Pawn = PC->GetPawn();
        if (Pawn)
        {
            CameraLocation = Pawn->GetActorLocation();
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Spawning %d planets near camera"), NumPlanets);

    float Spacing = 1005000.0f;

    for (int32 i = 0; i < NumPlanets; i++)
    {
        float Angle = (360.0f / FMath::Max(1, NumPlanets)) * i;
        float Radians = FMath::DegreesToRadians(Angle);

        FVector Location = CameraLocation + FVector(
            FMath::Cos(Radians) * Spacing,
            FMath::Sin(Radians) * Spacing,
            0.0f
        );

        FRotator Rotation = FRotator::ZeroRotator;

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ACosmicPlanet* Planet = World->SpawnActor<ACosmicPlanet>(
            ACosmicPlanet::StaticClass(),
            Location,
            Rotation,
            Params
        );

        if (!Planet) continue;

        if (UseFoliage)
        {
            Planet->SetFoliageParams(
                CurrentFoliageConfig.FoliageInstancesPerFrame,
                CurrentFoliageConfig.NearLayerRadiusKm,
                CurrentFoliageConfig.MediumLayerRadiusKm,
                CurrentFoliageConfig.FarLayerRadiusKm
            );
        }
        

        Planet->InitPlanet(
            10.0f, NoiseClass,
            FColor::Red, FColor::Orange,
            FColor::White, FColor::Red, FColor::Black,
            100.f, 3.f, 1.f,
            BaseMaterial, nullptr,
            true,
            CurrentClipmapConfig.BaseResolution,
            CurrentClipmapConfig.NumLevels,
            100, 5.0f,
            true, 0.0, 128, OceanMaterial,
            UseFoliage ? FoliageCollection : nullptr
        );   
    }
}

void UBenchmarkManager::SpawnPlanetsFar(int32 NumPlanets, bool UseFoliage)
{
    UWorld* World = GetWorld();
    if (!World) return;

    UE_LOG(LogTemp, Warning, TEXT("Spawning %d far planets"), NumPlanets);

    float Spacing = 5000000.0f;

    for (int32 i = 0; i < NumPlanets; i++)
    {
        FVector Location = FVector(i * Spacing, 0.f, 0.f);
        FRotator Rotation = FRotator::ZeroRotator;

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ACosmicPlanet* Planet = World->SpawnActor<ACosmicPlanet>(
            ACosmicPlanet::StaticClass(),
            Location,
            Rotation,
            Params
        );

        if (!Planet) continue;

        Planet->InitPlanet(
            10.0f, NoiseClass,
            FColor::Red, FColor::Orange,
            FColor::White, FColor::Red, FColor::Black,
            100.f, 3.f, 1.f,
            BaseMaterial, nullptr,
            true,
            CurrentClipmapConfig.BaseResolution,
            CurrentClipmapConfig.NumLevels,
            100, 5.0f,
            true, 0.0, 128, OceanMaterial,
            UseFoliage ? FoliageCollection : nullptr
        );
    }
}

void UBenchmarkManager::ClearPlanets()
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

void UBenchmarkManager::RunPlanetScalingTest()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Planet Scaling Test (Sequential) ==="));

    if (bIsRunningSequentialTest)
    {
        UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
        return;
    }

    ClearSimBodies();
    ClearPlanets();

    FBenchmarkRecorder::ClearCSVResults();

    // Configurar test secuencial
    SequentialTestSteps = { 1, 2, 4, 8, 16, 32, 64 };
    CurrentSequentialStepIndex = 0;
    CurrentSequentialTestType = ESequentialTestType::PlanetScaling;
    bIsRunningSequentialTest = true;

    // Iniciar primer paso
    RunNextSequentialStep();
}

void UBenchmarkManager::RunNextSequentialStep()
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
        SpawnPlanetsNear(1);
        break;
    case ESequentialTestType::FoliageViewDistance:
        if (CurrentStep < RadiusConfigs.Num())
        {
            FVector Radii = RadiusConfigs[CurrentStep];
            SetFoliageConfig(100, Radii.X, Radii.Y, Radii.Z);
            SpawnPlanetsNear(1);
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
                BeginCapture(5.0f);
            },
            1.5f, // Tiempo de estabilización
            false
        );
    }
}

void UBenchmarkManager::OnSequentialTestComplete()
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

    FBenchmarkRecorder::ExportCSV(CSVName);

    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("=== Sequential Test Complete ==="));

    if (bIsRunningAllTests)
    {
        AllTestsCurrentIndex++;
        OnAllTestsStepComplete();
    }
}


// Captura de Métricas
void UBenchmarkManager::BeginCapture(float DurationSeconds)
{
    FBenchmarkRecorder::StartRecording();

    bIsCapturing = true;
    CaptureDuration = DurationSeconds;
    AccumulatedCaptureTime = 0.0f;

    UE_LOG(LogTemp, Warning, TEXT("Capture Start - Duration: %.1f seconds - Test: %s"),
        DurationSeconds, *CurrentTestName);
}

void UBenchmarkManager::EndCapture()
{
    bIsCapturing = false;
    FBenchmarkRecorder::StopRecording();
    FBenchmarkRecorder::LogCurrentData(CurrentTestName);
}

void UBenchmarkManager::SetCurrentTestParams(int32 NumObjects, const FString& TestName)
{
    CurrentNumObjects = NumObjects;
    CurrentTestName = TestName;
}

void UBenchmarkManager::OnWorldEndPlay(UWorld& InWorld)
{
    EndCapture();

    Super::OnWorldEndPlay(InWorld);
}

void UBenchmarkManager::OnBenchmarkCaptureComplete()
{
    bIsCapturing = false;

    // Detener y mostrar resultados
    FBenchmarkRecorder::StopRecording();
    FBenchmarkRecorder::LogCurrentData(CurrentTestName);
    FBenchmarkRecorder::AddCSVResult(CurrentTestName);

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
                &UBenchmarkManager::RunNextSequentialStep,
                0.5f,
                false
            );
        }
    }
}

// Tests individuales (no secuenciales)
void UBenchmarkManager::RunClosePlanetTest()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Close Planet Test ==="));

    SequentialTestSteps = { 1, 2, 4, 8, 16 };
    CurrentSequentialStepIndex = 0;
    CurrentSequentialTestType = ESequentialTestType::ClosePlanetScaling;
    bIsRunningSequentialTest = true;

    FBenchmarkRecorder::ClearCSVResults();

    RunNextSequentialStep();
}

void UBenchmarkManager::RunFoliageTest()
{
    UE_LOG(LogTemp, Warning, TEXT("RunFoliageTest - Not implemented yet"));
}

void UBenchmarkManager::RunSimulationTest()
{
    UE_LOG(LogTemp, Warning, TEXT("RunSimulationTest - Not implemented yet"));
}


void UBenchmarkManager::SetFoliageConfig(int32 InFoliageInstancesPerFrame, float NearLayerRadiusKm, float MediumLayerRadiusKm, float FarLayerRadiusKm)
{
    CurrentFoliageConfig.FoliageInstancesPerFrame = InFoliageInstancesPerFrame;
    CurrentFoliageConfig.NearLayerRadiusKm = NearLayerRadiusKm;
    CurrentFoliageConfig.MediumLayerRadiusKm = MediumLayerRadiusKm;
    CurrentFoliageConfig.FarLayerRadiusKm = FarLayerRadiusKm;
}

void UBenchmarkManager::RunFoliagePerFrameTest(int32 MaxInstancesPerFrame)
{
    FBenchmarkRecorder::ClearCSVResults();

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

void UBenchmarkManager::RunFoliageRadiusTest()
{
    FBenchmarkRecorder::ClearCSVResults();

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
    // Usaremos pasos predefinidos para las 3 capas
    SequentialTestSteps = { 0, 1, 2, 3, 4, 5 }; // Índices para diferentes configuraciones
    CurrentSequentialStepIndex = 0;
    CurrentSequentialTestType = ESequentialTestType::FoliageViewDistance;
    bIsRunningSequentialTest = true;

    RunNextSequentialStep();
}

void UBenchmarkManager::RunClipmapResolutionTest(int32 Resolution)
{
    FBenchmarkRecorder::ClearCSVResults();

    if (Resolution <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== Clipmap Resolution Test (Sequential) ==="));

        if (bIsRunningSequentialTest)
        {
            UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
            return;
        }

        // Pasos de resolución a probar
        SequentialTestSteps = {8, 16, 32, 64, 128, 256 };
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

void UBenchmarkManager::RunClipmapLevelsTest(int32 Levels)
{
    FBenchmarkRecorder::ClearCSVResults();

    // Si se llama sin argumento (Levels=0), ejecutar test secuencial automático
    if (Levels <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== Clipmap Levels Test (Sequential) ==="));

        if (bIsRunningSequentialTest)
        {
            UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
            return;
        }

        // Pasos de niveles a probar: 1, 2, 4, 6, 8
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

void UBenchmarkManager::RunOrbitSimulationTest(int32 NumBodies)
{

    FBenchmarkRecorder::ClearCSVResults();

    // Si NumBodies <= 0, ejecutar test secuencial
    if (NumBodies <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== Orbit Simulation Test (Sequential) ==="));

        if (bIsRunningSequentialTest)
        {
            UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
            return;
        }

        // Pasos: 10, 50, 100, 200, 500 cuerpos
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

void UBenchmarkManager::RunNBodySimulationTest(int32 NumBodies)
{

    FBenchmarkRecorder::ClearCSVResults();

    // Si NumBodies <= 0, ejecutar test secuencial
    if (NumBodies <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("=== NBody Simulation Test (Sequential) ==="));

        if (bIsRunningSequentialTest)
        {
            UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
            return;
        }

        // Pasos: 10, 20, 50, 100, 200 cuerpos
        SequentialTestSteps = { 10, 20, 50, 100, 200 };
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

void UBenchmarkManager::RunSystemGeneratorTest(int32 NumBodies)
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

void UBenchmarkManager::SpawnSimBodies(int32 NumBodies, bool bNBodySimulation)
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

    CentralBody = World->SpawnActor<ABenchmarkSimBody>(
        ABenchmarkSimBody::StaticClass(),
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

        ABenchmarkSimBody* Body = World->SpawnActor<ABenchmarkSimBody>(
            ABenchmarkSimBody::StaticClass(),
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

void UBenchmarkManager::ClearSimBodies()
{
    for (ABenchmarkSimBody* Body : SimBodies)
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

