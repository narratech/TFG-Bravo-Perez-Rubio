// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BenchmarkManager.generated.h"

/**
 *
 */
UCLASS()
class COSMICARCHITECTRUNTIME_API UBenchmarkManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    static UBenchmarkManager* Get(UWorld* World);

    // --- Control general ---
    void StartBenchmark();
    void StopBenchmark();

    // --- Planets ---
    void SetClipmapConfig(int32 BaseRes, int32 Levels);
    void SpawnPlanets(int32 NumPlanets);
    void SpawnPlanetsNear(int32 NumPlanets);
    void SpawnPlanetsFar(int32 NumPlanets);
    void ClearPlanets();

    // --- Tests ---
    void RunPlanetScalingTest();
    void RunClosePlanetTest();
    void RunFoliageTest();
    void RunSimulationTest();

    // --- Foliage Tests ---
    void RunFoliageDensityTest(int32 TotalInstances);
    void RunFoliagePerFrameTest(int32 MaxInstancesPerFrame);

    // --- Clipmap Tests ---
    void RunClipmapResolutionTest(int32 Resolution);
    void RunClipmapLevelsTest(int32 Levels);

    // --- Simulation Tests ---
    void RunOrbitSimulationTest(int32 NumBodies);
    void RunNBodySimulationTest(int32 NumBodies);

    // --- System Generator Test ---
    void RunSystemGeneratorTest(int32 NumBodies);

    // --- Métricas ---
    void BeginCapture(float DurationSeconds = 5.0f);
    void EndCapture();
    void SetCurrentTestParams(int32 NumObjects, const FString& TestName);

    struct FClipmapConfig
    {
        int32 BaseResolution = 128;
        int32 NumLevels = 4;
    };

private:
    FClipmapConfig CurrentClipmapConfig;
    FTimerHandle BenchmarkTimerHandle;
    FString CurrentTestName;
    int32 CurrentNumObjects;

    void OnBenchmarkCaptureComplete();
};