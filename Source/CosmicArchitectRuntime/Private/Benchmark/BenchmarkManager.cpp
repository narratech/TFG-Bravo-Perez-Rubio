// Fill out your copyright notice in the Description page of Project Settings.

#include "Benchmark/BenchmarkManager.h"
#include "Benchmark/BenchmarkRecorder.h"
#include "Engine/World.h"
#include "Planet/CosmicPlanet.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UBenchmarkManager* UBenchmarkManager::Get(UWorld* World)
{
    if (!World) return nullptr;
    return World->GetSubsystem<UBenchmarkManager>();
}

// ============================================================
// FTickableGameObject Implementation
// ============================================================

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

// ============================================================
// Control General
// ============================================================

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
void UBenchmarkManager::SpawnPlanets(int32 NumPlanets)
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

        Planet->InitPlanet(
            10.0f,
            nullptr,
            FColor::Red,
            FColor::Orange,
            FColor::White,
            FColor::Red,
            FColor::Black,
            100.f, 3.f, 1.f,
            nullptr, nullptr,
            true,
            CurrentClipmapConfig.BaseResolution,
            CurrentClipmapConfig.NumLevels,
            100, 5.0f,
            true, 0.0, 128, nullptr,
            nullptr
        );
    }
}

void UBenchmarkManager::SpawnPlanetsNear(int32 NumPlanets)
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

    float Spacing = 3000000.0f;

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

        Planet->InitPlanet(
            10.0f, nullptr,
            FColor::Red, FColor::Orange,
            FColor::White, FColor::Red, FColor::Black,
            100.f, 3.f, 1.f,
            nullptr, nullptr,
            true,
            CurrentClipmapConfig.BaseResolution,
            CurrentClipmapConfig.NumLevels,
            100, 5.0f,
            true, 0.0, 128, nullptr,
            nullptr
        );
    }
}

void UBenchmarkManager::SpawnPlanetsFar(int32 NumPlanets)
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
            10.0f, nullptr,
            FColor::Red, FColor::Orange,
            FColor::White, FColor::Red, FColor::Black,
            100.f, 3.f, 1.f,
            nullptr, nullptr,
            true,
            CurrentClipmapConfig.BaseResolution,
            CurrentClipmapConfig.NumLevels,
            100, 5.0f,
            true, 0.0, 128, nullptr,
            nullptr
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

// ============================================================
// Tests Secuenciales
// ============================================================

void UBenchmarkManager::RunPlanetScalingTest()
{
    UE_LOG(LogTemp, Warning, TEXT("============================================"));
    UE_LOG(LogTemp, Warning, TEXT("=== Planet Scaling Test (Sequential) ==="));
    UE_LOG(LogTemp, Warning, TEXT("============================================"));

    if (bIsRunningSequentialTest)
    {
        UE_LOG(LogTemp, Warning, TEXT("Test already running. Stop it first."));
        return;
    }

    ClearPlanets();

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

    case ESequentialTestType::ClipmapResolution:
    {
        // Actualizar configuración de clipmap
        SetClipmapConfig(CurrentStep, CurrentClipmapConfig.NumLevels);
        SpawnPlanetsNear(1); // Solo 1 planeta para test de resolución
        break;
    }

    case ESequentialTestType::ClipmapLevels:
    {
        // Actualizar niveles manteniendo resolución base
        SetClipmapConfig(CurrentClipmapConfig.BaseResolution, CurrentStep);
        SpawnPlanetsNear(1);
        break;
    }

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
                case ESequentialTestType::ClipmapResolution:
                    TestName = FString::Printf(TEXT("ClipmapRes_%d"), CurrentStep);
                    break;
                case ESequentialTestType::ClipmapLevels:
                    TestName = FString::Printf(TEXT("ClipmapLevels_%d"), CurrentStep);
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
    CurrentSequentialTestType = ESequentialTestType::None;
    SequentialTestSteps.Empty();

    ClearPlanets();

    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("============================================"));
    UE_LOG(LogTemp, Warning, TEXT("=== Sequential Test Complete ==="));
    UE_LOG(LogTemp, Warning, TEXT("============================================"));
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

void UBenchmarkManager::OnBenchmarkCaptureComplete()
{
    bIsCapturing = false;

    // Detener y mostrar resultados
    FBenchmarkRecorder::StopRecording();
    FBenchmarkRecorder::LogCurrentData(CurrentTestName);

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

    ClearPlanets();
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


// Stubs para otros tests
void UBenchmarkManager::RunFoliageDensityTest(int32 TotalInstances)
{
    SetCurrentTestParams(TotalInstances, FString::Printf(TEXT("FoliageDensity_%d"), TotalInstances));
    BeginCapture(5.0f);
}

void UBenchmarkManager::RunFoliagePerFrameTest(int32 MaxInstancesPerFrame)
{
    SetCurrentTestParams(MaxInstancesPerFrame, FString::Printf(TEXT("FoliagePerFrame_%d"), MaxInstancesPerFrame));
    BeginCapture(5.0f);
}

void UBenchmarkManager::RunClipmapResolutionTest(int32 Resolution)
{
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

        ClearPlanets();
        RunNextSequentialStep();
    }
    else
    {
        // Test individual con una resolución específica
        UE_LOG(LogTemp, Warning, TEXT("=== Clipmap Resolution Test: %d ==="), Resolution);

        SetClipmapConfig(Resolution, CurrentClipmapConfig.NumLevels);
        ClearPlanets();
        SpawnPlanetsNear(1);
        SetCurrentTestParams(Resolution, FString::Printf(TEXT("ClipmapRes_%d"), Resolution));
        BeginCapture(5.0f);
    }
}

void UBenchmarkManager::RunClipmapLevelsTest(int32 Levels)
{
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

        ClearPlanets();
        RunNextSequentialStep();
    }
    else
    {
        // Test individual con un número específico de niveles
        UE_LOG(LogTemp, Warning, TEXT("=== Clipmap Levels Test: %d levels ==="), Levels);

        SetClipmapConfig(CurrentClipmapConfig.BaseResolution, Levels);
        ClearPlanets();
        SpawnPlanetsNear(1);
        SetCurrentTestParams(Levels, FString::Printf(TEXT("ClipmapLevels_%d"), Levels));
        BeginCapture(5.0f);
    }
}

void UBenchmarkManager::RunOrbitSimulationTest(int32 NumBodies)
{
    SetCurrentTestParams(NumBodies, FString::Printf(TEXT("OrbitSim_%d"), NumBodies));
    BeginCapture(5.0f);
}

void UBenchmarkManager::RunNBodySimulationTest(int32 NumBodies)
{
    SetCurrentTestParams(NumBodies, FString::Printf(TEXT("NBodySim_%d"), NumBodies));
    BeginCapture(5.0f);
}

void UBenchmarkManager::RunSystemGeneratorTest(int32 NumBodies)
{
    double StartTime = FPlatformTime::Seconds();

    // GenerateSystem(NumBodies); // Aquí iría la llamada real

    double EndTime = FPlatformTime::Seconds();
    double GenerationTime = EndTime - StartTime;

    UE_LOG(LogTemp, Warning, TEXT("System generation time for %d bodies: %.3f seconds"),
        NumBodies, GenerationTime);
}