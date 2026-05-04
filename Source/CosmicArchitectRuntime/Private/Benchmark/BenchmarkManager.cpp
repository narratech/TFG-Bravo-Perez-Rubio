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

void UBenchmarkManager::StartBenchmark()
{
    UE_LOG(LogTemp, Warning, TEXT("Benchmark started"));
}

void UBenchmarkManager::StopBenchmark()
{
    UE_LOG(LogTemp, Warning, TEXT("Benchmark finished"));
}

void UBenchmarkManager::SetClipmapConfig(int32 BaseRes, int32 Levels)
{
    CurrentClipmapConfig.BaseResolution = BaseRes;
    CurrentClipmapConfig.NumLevels = Levels;

    UE_LOG(LogTemp, Warning, TEXT("Clipmap updated: Res=%d Levels=%d"),
        BaseRes, Levels);
}

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
            10.0f,                 // RadiusKm
            nullptr,                 // Noise
            FColor::Red,
            FColor::Orange,
            FColor::White,
            FColor::Red,
            FColor::Black,
            100.f,                  // ScaleL
            3.f,                    // ScaleM
            1.f,                    // ScaleS
            nullptr,                // Material
            nullptr,                // Texture

            // Clipmap
            true,
            CurrentClipmapConfig.BaseResolution,
            CurrentClipmapConfig.NumLevels,
            100,
            5.0f,

            // Ocean
            true,
            0.0,
            128,
            nullptr,

            // Foliage
            nullptr
        );
    }
}

void UBenchmarkManager::SpawnPlanetsNear(int32 NumPlanets)
{
    UWorld* World = GetWorld();
    if (!World) return;

    // Obtener posición de la cámara
    FVector CameraLocation = FVector::ZeroVector;
    if (GEngine && GEngine->GameViewport)
    {
        // Obtener posición del jugador o usar origen
        APlayerController* PC = World->GetFirstPlayerController();
        if (PC)
        {
            APawn* Pawn = PC->GetPawn();
            if (Pawn)
            {
                CameraLocation = Pawn->GetActorLocation();
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Spawning %d planets near camera"), NumPlanets);

    // Espaciamiento para planetas cercanos (que estén dentro del rango de alto detalle)
    float Spacing = 50000.0f; // 50 km entre planetas

    for (int32 i = 0; i < NumPlanets; i++)
    {
        // Distribuir en un patrón circular alrededor de la cámara
        float Angle = (360.0f / NumPlanets) * i;
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

void UBenchmarkManager::SpawnPlanetsFar(int32 NumPlanets)
{
    UWorld* World = GetWorld();
    if (!World) return;

    UE_LOG(LogTemp, Warning, TEXT("Spawning %d far planets (low detail mode)"), NumPlanets);

    // Espaciamiento grande para que estén en modo bajo detalle
    float Spacing = 5000000.0f; // 5000 km entre planetas

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
            10.0f,
            nullptr,
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

void UBenchmarkManager::RunPlanetScalingTest()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Planet Scaling Test ==="));

    ClearPlanets();

    TArray<int32> Steps = { 1, 2, 4, 8, 16, 32 };

    for (int32 Num : Steps)
    {
        ClearPlanets();
        SpawnPlanets(Num);

        SetCurrentTestParams(Num, FString::Printf(TEXT("PlanetScaling_%d"), Num));
        BeginCapture(5.0f);

        // Esperar a que termine la captura antes de continuar
        // En una implementación real, esto sería asíncrono
    }
}

void UBenchmarkManager::RunClosePlanetTest()
{
    UE_LOG(LogTemp, Warning, TEXT("=== Close Planet Test ==="));

    ClearPlanets();

    TArray<int32> Steps = { 1, 2, 4, 8 };

    for (int32 Num : Steps)
    {
        ClearPlanets();
        SpawnPlanetsNear(Num);

        SetCurrentTestParams(Num, FString::Printf(TEXT("ClosePlanets_%d"), Num));
        BeginCapture(5.0f);
    }
}

void UBenchmarkManager::RunFoliageDensityTest(int32 TotalInstances)
{
    UE_LOG(LogTemp, Warning, TEXT("=== Foliage Density Test: %d instances ==="), TotalInstances);
    SetCurrentTestParams(TotalInstances, FString::Printf(TEXT("FoliageDensity_%d"), TotalInstances));
}

void UBenchmarkManager::RunFoliagePerFrameTest(int32 MaxInstancesPerFrame)
{
    UE_LOG(LogTemp, Warning, TEXT("=== Foliage Per Frame Test: %d max/frame ==="), MaxInstancesPerFrame);
    SetCurrentTestParams(MaxInstancesPerFrame, FString::Printf(TEXT("FoliagePerFrame_%d"), MaxInstancesPerFrame));
}

void UBenchmarkManager::RunClipmapResolutionTest(int32 Resolution)
{
    UE_LOG(LogTemp, Warning, TEXT("=== Clipmap Resolution Test: %d ==="), Resolution);

    // Actualizar configuración
    SetClipmapConfig(Resolution, CurrentClipmapConfig.NumLevels);
    ClearPlanets();
    SpawnPlanets(1); // Un solo planeta para prueba controlada

    SetCurrentTestParams(Resolution, FString::Printf(TEXT("ClipmapRes_%d"), Resolution));
    BeginCapture(5.0f);
}

void UBenchmarkManager::RunClipmapLevelsTest(int32 Levels)
{
    UE_LOG(LogTemp, Warning, TEXT("=== Clipmap Levels Test: %d ==="), Levels);

    SetClipmapConfig(CurrentClipmapConfig.BaseResolution, Levels);
    ClearPlanets();
    SpawnPlanets(1);

    SetCurrentTestParams(Levels, FString::Printf(TEXT("ClipmapLevels_%d"), Levels));
    BeginCapture(5.0f);
}

void UBenchmarkManager::RunOrbitSimulationTest(int32 NumBodies)
{
    UE_LOG(LogTemp, Warning, TEXT("=== Orbit Simulation Test: %d bodies ==="), NumBodies);
    SetCurrentTestParams(NumBodies, FString::Printf(TEXT("OrbitSim_%d"), NumBodies));
    BeginCapture(5.0f);
}

void UBenchmarkManager::RunNBodySimulationTest(int32 NumBodies)
{
    UE_LOG(LogTemp, Warning, TEXT("=== N-Body Simulation Test: %d bodies ==="), NumBodies);
    SetCurrentTestParams(NumBodies, FString::Printf(TEXT("NBodySim_%d"), NumBodies));
    BeginCapture(5.0f);
}

void UBenchmarkManager::RunSystemGeneratorTest(int32 NumBodies)
{
    UE_LOG(LogTemp, Warning, TEXT("=== System Generator Test: %d bodies ==="), NumBodies);

    double StartTime = FPlatformTime::Seconds();

    // Aquí iría la llamada real al generador de sistemas
    // Ejemplo: GenerateSystem(NumBodies);

    double EndTime = FPlatformTime::Seconds();
    double GenerationTime = EndTime - StartTime;

    UE_LOG(LogTemp, Warning, TEXT("System generation time for %d bodies: %.3f seconds"),
        NumBodies, GenerationTime);
}

void UBenchmarkManager::RunFoliageTest()
{
    UE_LOG(LogTemp, Warning, TEXT("RunFoliageTest"));
}

void UBenchmarkManager::RunSimulationTest()
{
    UE_LOG(LogTemp, Warning, TEXT("RunSimulationTest"));
}

void UBenchmarkManager::BeginCapture(float DurationSeconds)
{
    FBenchmarkRecorder::StartRecording();
    SetCurrentTestParams(CurrentNumObjects, CurrentTestName);

    // Programar fin de captura
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().SetTimer(
            BenchmarkTimerHandle,
            this,
            &UBenchmarkManager::OnBenchmarkCaptureComplete,
            DurationSeconds,
            false
        );
    }

    UE_LOG(LogTemp, Warning, TEXT("Capture Start - Duration: %.1f seconds"), DurationSeconds);
}

void UBenchmarkManager::EndCapture()
{
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
    EndCapture();
}