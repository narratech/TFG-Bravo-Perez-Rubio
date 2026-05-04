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
    void ClearPlanets();

    // --- Tests ---
    void RunPlanetScalingTest();
    void RunFoliageTest();
    void RunSimulationTest();

    // --- Métricas ---
    void BeginCapture();
    void EndCapture();

    struct FClipmapConfig
    {
        int32 BaseResolution = 128;
        int32 NumLevels = 4;
    };

private:
    FClipmapConfig CurrentClipmapConfig;
};
