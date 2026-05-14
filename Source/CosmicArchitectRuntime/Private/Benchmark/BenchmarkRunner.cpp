// Fill out your copyright notice in the Description page of Project Settings.

#include "Benchmark/BenchmarkRunner.h"
#include "Benchmark/BenchmarkManager.h"
#include "Benchmark/BenchmarkRecorder.h"
#include "Engine/World.h"
#include "TimerManager.h"

// Variable estática para control interno
static FTimerHandle RunnerTimerHandle;
static TArray<int32> PlanetSteps;
static int32 CurrentStepIndex = 0;
static UBenchmarkManager* CurrentManager = nullptr;
static bool bIsRunningTest = false;

void FBenchmarkRunner::RunPlanetScaling(UBenchmarkManager* Manager)
{
    if (!Manager || bIsRunningTest) return;

    CurrentManager = Manager;
    PlanetSteps = { 1, 2, 4, 8, 16, 32 };
    CurrentStepIndex = 0;
    bIsRunningTest = true;

    UE_LOG(LogTemp, Warning, TEXT("=== Starting Planet Scaling Benchmark ==="));
    UE_LOG(LogTemp, Warning, TEXT("Steps: 1, 2, 4, 8, 16, 32 planets"));

    // Limpiar antes de empezar
    Manager->ClearPlanets();

    // Ejecutar primer paso
    ExecuteNextStep();
}

void FBenchmarkRunner::ExecuteNextStep()
{
    if (!CurrentManager || CurrentStepIndex >= PlanetSteps.Num())
    {
        OnTestComplete();
        return;
    }

    int32 NumPlanets = PlanetSteps[CurrentStepIndex];

    UE_LOG(LogTemp, Warning, TEXT("--- Step %d/%d: Testing with %d planets ---"),
        CurrentStepIndex + 1, PlanetSteps.Num(), NumPlanets);

    // Limpiar y generar planetas
    CurrentManager->ClearPlanets();
    CurrentManager->SpawnPlanets(NumPlanets);

    // Configurar parámetros de la prueba
    CurrentManager->SetCurrentTestParams(NumPlanets,
        FString::Printf(TEXT("PlanetScaling_%d"), NumPlanets));

    // Esperar un frame para que los planetas se inicialicen
    UWorld* World = CurrentManager->GetWorld();
    if (World)
    {
        // Dar tiempo a que los sistemas se estabilicen (2 segundos)
        World->GetTimerManager().SetTimer(
            RunnerTimerHandle,
            []()
            {
                if (CurrentManager)
                {
                    // Iniciar captura de 5 segundos
                    CurrentManager->BeginCapture(5.0f);

                    // Programar siguiente paso después de la captura
                    UWorld* W = CurrentManager->GetWorld();
                    if (W)
                    {
                        W->GetTimerManager().SetTimer(
                            RunnerTimerHandle,
                            []()
                            {
                                CurrentStepIndex++;
                                ExecuteNextStep();
                            },
                            6.0f, // 5s captura + 1s margen
                            false
                        );
                    }
                }
            },
            2.0f,
            false
        );
    }
}

void FBenchmarkRunner::OnTestComplete()
{
    bIsRunningTest = false;

    UE_LOG(LogTemp, Warning, TEXT("=== Planet Scaling Benchmark Complete ==="));

    if (CurrentManager)
    {
        CurrentManager->ClearPlanets();
    }

    CurrentManager = nullptr;
}

void FBenchmarkRunner::RunClosePlanetScaling(UBenchmarkManager* Manager)
{
    if (!Manager) return;

    UE_LOG(LogTemp, Warning, TEXT("=== Close Planet Scaling Test ==="));

    TArray<int32> Steps = { 1, 2, 4, 8 };

    for (int32 Num : Steps)
    {
        Manager->ClearPlanets();
        Manager->SpawnPlanetsNear(Num);

        Manager->SetCurrentTestParams(Num, FString::Printf(TEXT("ClosePlanet_%d"), Num));
        Manager->BeginCapture(5.0f);

        UE_LOG(LogTemp, Warning, TEXT("Testing %d close planets"), Num);
    }
}

void FBenchmarkRunner::RunFoliageBenchmark(UBenchmarkManager* Manager)
{
    if (!Manager) return;

    UE_LOG(LogTemp, Warning, TEXT("=== Foliage Benchmark ==="));

    // Experimento A: Densidad de foliage
    TArray<int32> DensitySteps = { 1000, 5000, 10000, 50000, 100000 };

    for (int32 Instances : DensitySteps)
    {
        Manager->SetCurrentTestParams(Instances, FString::Printf(TEXT("FoliageDensity_%d"), Instances));
        //Manager->RunFoliageDensityTest(Instances);
        Manager->BeginCapture(5.0f);
    }

    // Experimento B: Instancias por frame
    TArray<int32> PerFrameSteps = { 10, 50, 100, 500, 1000 };

    for (int32 MaxPerFrame : PerFrameSteps)
    {
        Manager->SetCurrentTestParams(MaxPerFrame, FString::Printf(TEXT("FoliagePerFrame_%d"), MaxPerFrame));
        //Manager->RunFoliagePerFrameTest(MaxPerFrame);
        Manager->BeginCapture(5.0f);
    }
}

void FBenchmarkRunner::RunClipmapBenchmark(UBenchmarkManager* Manager)
{
    if (!Manager) return;

    UE_LOG(LogTemp, Warning, TEXT("=== Clipmap Benchmark ==="));

    // Experimento A: Resolución base
    TArray<int32> Resolutions = { 8, 16, 32, 64, 128, 256};

    for (int32 Res : Resolutions)
    {
        Manager->ClearPlanets();
        Manager->SetClipmapConfig(Res, 4);
        Manager->SpawnPlanets(1);

        Manager->SetCurrentTestParams(Res, FString::Printf(TEXT("ClipmapRes_%d"), Res));
        Manager->BeginCapture(5.0f);
    }

    // Experimento B: Número de niveles
    TArray<int32> Levels = { 1, 2, 4, 6, 8 };

    for (int32 Lvl : Levels)
    {
        Manager->ClearPlanets();
        Manager->SetClipmapConfig(128, Lvl);
        Manager->SpawnPlanets(1);

        Manager->SetCurrentTestParams(Lvl, FString::Printf(TEXT("ClipmapLevels_%d"), Lvl));
        Manager->BeginCapture(5.0f);
    }
}

void FBenchmarkRunner::RunSimulationBenchmark(UBenchmarkManager* Manager)
{
    if (!Manager) return;

    UE_LOG(LogTemp, Warning, TEXT("=== Simulation Benchmark ==="));

    // Experimento 1: Órbitas simplificadas
    TArray<int32> OrbitBodies = { 10, 50, 100, 200, 500 };

    UE_LOG(LogTemp, Warning, TEXT("--- Orbit Simulation ---"));
    for (int32 Bodies : OrbitBodies)
    {
        Manager->RunOrbitSimulationTest(Bodies);
        Manager->BeginCapture(5.0f);
    }

    // Experimento 2: N-Body completo
    TArray<int32> NBodyBodies = { 10, 20, 50, 100 };

    UE_LOG(LogTemp, Warning, TEXT("--- N-Body Simulation ---"));
    for (int32 Bodies : NBodyBodies)
    {
        Manager->RunNBodySimulationTest(Bodies);
        Manager->BeginCapture(5.0f);
    }
}

void FBenchmarkRunner::RunSystemGeneratorBenchmark(UBenchmarkManager* Manager)
{
    if (!Manager) return;

    UE_LOG(LogTemp, Warning, TEXT("=== System Generator Benchmark ==="));

    TArray<int32> BodyCounts = { 10, 50, 100, 200, 500, 1000 };

    for (int32 Bodies : BodyCounts)
    {
        Manager->RunSystemGeneratorTest(Bodies);
    }
}

void FBenchmarkRunner::RunFullBenchmarkSuite(UBenchmarkManager* Manager)
{
    if (!Manager) return;

    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("=== FULL BENCHMARK SUITE STARTING ==="));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));

    // Planet Scaling
    UE_LOG(LogTemp, Warning, TEXT("\n[1/5] Planet Scaling Test"));
    RunPlanetScaling(Manager);

    // Close Planets
    UE_LOG(LogTemp, Warning, TEXT("\n[2/5] Close Planet Test"));
    RunClosePlanetScaling(Manager);

    // Foliage
    UE_LOG(LogTemp, Warning, TEXT("\n[3/5] Foliage Test"));
    RunFoliageBenchmark(Manager);

    // Clipmap
    UE_LOG(LogTemp, Warning, TEXT("\n[4/5] Clipmap Test"));
    RunClipmapBenchmark(Manager);

    // Simulation
    UE_LOG(LogTemp, Warning, TEXT("\n[5/5] Simulation Test"));
    RunSimulationBenchmark(Manager);

    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("=== FULL BENCHMARK SUITE COMPLETE ==="));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
}