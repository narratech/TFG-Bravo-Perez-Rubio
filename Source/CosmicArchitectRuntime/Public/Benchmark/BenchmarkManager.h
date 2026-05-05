// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "BenchmarkManager.generated.h"

class ABenchmarkSimBody;

/**
 *
 */
UCLASS()
class COSMICARCHITECTRUNTIME_API UBenchmarkManager : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    static UBenchmarkManager* Get(UWorld* World);

    // --- FTickableGameObject interface ---
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UBenchmarkManager, STATGROUP_Tickables);
    }
    virtual bool IsTickable() const override { return bIsCapturing; }
    virtual bool IsTickableInEditor() const override { return false; }

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
    void SpawnSimBodies(int32 NumBodies, bool bNBodySimulation);
    void ClearSimBodies();

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
    FString CurrentTestName;
    int32 CurrentNumObjects;

    TArray<ABenchmarkSimBody*> SimBodies;
    ABenchmarkSimBody* CentralBody = nullptr;

    // Variables para el sistema de captura por tick
    bool bIsCapturing = false;
    float CaptureDuration = 0.0f;
    float AccumulatedCaptureTime = 0.0f;

    // Variables para tests secuenciales
    bool bIsRunningSequentialTest = false;
    TArray<int32> SequentialTestSteps;
    int32 CurrentSequentialStepIndex = 0;
    FTimerHandle SequentialTestTimerHandle;

    // Tipo de test secuencial
    enum class ESequentialTestType : uint8
    {
        PlanetScaling,
        ClosePlanetScaling,
        FoliageDensity,
        FoliagePerFrame,
        ClipmapResolution,
        ClipmapLevels,
        OrbitSimulation,
        NBodySimulation,
        None
    };
    ESequentialTestType CurrentSequentialTestType = ESequentialTestType::None;

    void OnBenchmarkCaptureComplete();
    void RunNextSequentialStep();
    void OnSequentialTestComplete();
};