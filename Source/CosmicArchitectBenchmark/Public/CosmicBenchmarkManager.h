// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CosmicBenchmarkManager.generated.h"

class ACosmicBenchmarkSimBody;
class UCosmicNoiseClass;
class UCosmicFoliageCollection;
class ACosmicSystemGenerator;
class UMaterialInstance;

DECLARE_DELEGATE(FOnBenchmarkTestComplete);

USTRUCT(BlueprintType)
struct FBenchmarkPlanetConfig
{
    GENERATED_BODY()

    // Geometría 
    /** Radio del planeta en Km */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Geometry")
    float RadiusKm = 10.0f;

    // Spawn 
    /** Centro del mundo usado como origen para SpawnPlanets (modo "far") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Spawn")
    FVector SpawnCenter = FVector::ZeroVector;

    /** Separación entre planetas en SpawnPlanets (unidades UE = cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Spawn")
    float SpawnSpacingCm = 500000.0f;

    /** Radio del círculo en SpawnPlanetsNear (unidades UE = cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Spawn")
    float NearSpawnRadiusCm = 1005000.0f;

    /** Separación entre planetas en SpawnPlanetsFar (unidades UE = cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Spawn")
    float FarSpawnSpacingCm = 5000000.0f;

    // Océano 
    /** ¿El planeta tiene océano? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Ocean")
    bool bHasOcean = true;

    /** Nivel del mar (0 = ecuador de altura 0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Ocean",
        meta = (EditCondition = "bHasOcean"))
    float OceanSeaLevel = 0.0f;

    /** Resolución del clipmap del océano */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Ocean",
        meta = (EditCondition = "bHasOcean"))
    int32 OceanClipmapResolution = 128;

    // Foliaje 
    /** ¿Activar foliaje por defecto en SpawnPlanets? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Foliage")
    bool bUseFoliageByDefault = false;

    // Captura 
    /** Duración de cada paso en los tests secuenciales (segundos) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Benchmark|Capture",
        meta = (ClampMin = "1.0", ClampMax = "120.0"))
    float CaptureDurationSeconds = 8.0f;

    /** Pausa de estabilización entre pasos de test secuencial (segundos) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Benchmark|Capture",
        meta = (ClampMin = "0.5", ClampMax = "30.0"))
    float StabilizationDelaySeconds = 1.5f;
};

/**
 * 
 */
UCLASS()
class COSMICARCHITECTBENCHMARK_API UCosmicBenchmarkManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
    static UCosmicBenchmarkManager* Get(UWorld* World);

    // FTickableGameObject 
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UCosmicBenchmarkManager, STATGROUP_Tickables);
    }
    virtual bool IsTickable() const override { return bIsCapturing; }
    virtual bool IsTickableInEditor() const override { return false; }

    // Inicialización 
    void InitializeAssets(
        UMaterialInstance* InBaseMaterial,
        UMaterialInstance* InMoonMaterial,
        UMaterialInstance* InOceanMaterial,
        UMaterialInstance* InStarMaterial,
        UMaterialInstance* InGasGiantMaterial,
        UMaterialInstance* InRingMaterial,
        UCosmicNoiseClass* InNoiseClass,
        UCosmicFoliageCollection* InFoliageCollection
    );

    /** Aplica la configuración de planeta para todos los tests posteriores.
     *  Llámalo desde ABenchMarkConfig::BeginPlay (ya integrado). */
    UFUNCTION(BlueprintCallable, Category = "Benchmark|Config")
    void SetPlanetConfig(const FBenchmarkPlanetConfig& InConfig);

    /** Devuelve la configuración de planeta activa (read-only). */
    UFUNCTION(BlueprintCallable, Category = "Benchmark|Config")
    const FBenchmarkPlanetConfig& GetPlanetConfig() const { return CurrentPlanetConfig; }

    // Control general 
    void StartBenchmark();
    void StopBenchmark();

    // Planets 
    void SetClipmapConfig(int32 BaseRes, int32 Levels);
    void SpawnPlanets(int32 NumPlanets, bool UseFoliage = false);
    void SpawnPlanetsNear(int32 NumPlanets, bool UseFoliage = true);
    void SpawnPlanetsFar(int32 NumPlanets, bool UseFoliage = false);
    void ClearPlanets();

    //  Tests 
    void RunPlanetScalingTest();
    void RunClosePlanetTest();

    //  Foliage Tests 
    void SetFoliageConfig(
        int32 InFoliageInstancesPerFrame = 50,
        float NearLayerRadiusKm = 0.05f,
        float MediumLayerRadiusKm = 0.2f,
        float FarLayerRadiusKm = 0.5f
    );
    void RunFoliagePerFrameTest(int32 MaxInstancesPerFrame = 0);
    void RunFoliageRadiusTest();

    //  Clipmap Tests 
    void RunClipmapResolutionTest(int32 Resolution = 0);
    void RunClipmapLevelsTest(int32 Levels = 0);

    //  Simulation Tests 
    void RunOrbitSimulationTest(int32 NumBodies = 0);
    void RunNBodySimulationTest(int32 NumBodies = 0);
    void SpawnSimBodies(int32 NumBodies, bool bNBodySimulation);
    void ClearSimBodies();

    //  System Generator Test 
    void RunSystemGeneratorTest(int32 NumBodies = 0);

    //  Métricas 
    void BeginCapture(float DurationSeconds = 5.0f);
    void EndCapture();
    void SetCurrentTestParams(int32 NumObjects, const FString& TestName);

    void RunAllTests();

    // 
    //  EVENTOS PERSONALIZADOS
    //
    //  Registra un evento desde cualquier sistema externo:
    //
    //    C++:
    //      if (UBenchmarkManager* BM = UBenchmarkManager::Get(GetWorld()))
    //          BM->RecordEvent(TEXT("TerrainReady"), TEXT("Chunks generados"), ChunkCount);
    //
    //    Blueprint:
    //      (nodo "Record Benchmark Event" en cualquier actor/component)
    //
    //  El evento aparecerá en la sección "# CUSTOM EVENTS" del CSV exportado.
    // 
    UFUNCTION(BlueprintCallable, Category = "Benchmark|Events",
        meta = (DisplayName = "Record Benchmark Event",
            ToolTip = "Registra un evento personalizado en el CSV del benchmark actual."))
    void RecordEvent(const FString& EventName,
        const FString& Description = TEXT(""),
        float          NumericValue = 0.0f);

    FOnBenchmarkTestComplete OnSequentialTestCompleteDelegate;

protected:
    virtual void OnWorldEndPlay(UWorld& InWorld) override;

    struct FClipmapConfig
    {
        int32 BaseResolution = 128;
        int32 NumLevels = 4;
    };

    struct FFoliageConfig
    {
        int32 FoliageInstancesPerFrame = 50;
        float NearLayerRadiusKm = 0.05f;
        float MediumLayerRadiusKm = 0.2f;
        float FarLayerRadiusKm = 0.5f;
    };

private:
    UPROPERTY() UMaterialInstance* BaseMaterial = nullptr;
    UPROPERTY() UMaterialInstance* MoonMaterial = nullptr;
    UPROPERTY() UMaterialInstance* OceanMaterial = nullptr;
    UPROPERTY() UMaterialInstance* StarMaterial = nullptr;
    UPROPERTY() UMaterialInstance* GasGiantMaterial = nullptr;
    UPROPERTY() UMaterialInstance* RingMaterial = nullptr;
    UPROPERTY() UCosmicNoiseClass* NoiseClass = nullptr;
    UPROPERTY() UCosmicFoliageCollection* FoliageCollection = nullptr;

    FBenchmarkPlanetConfig CurrentPlanetConfig;
    FClipmapConfig         CurrentClipmapConfig;
    FFoliageConfig         CurrentFoliageConfig;
    FString                CurrentTestName;
    int32                  CurrentNumObjects = 0;

    TArray<ACosmicBenchmarkSimBody*>  SimBodies;
    ACosmicBenchmarkSimBody* CentralBody = nullptr;
    ACosmicSystemGenerator* SystemGenerator = nullptr;

    TArray<FVector> RadiusConfigs;

    // Captura
    bool  bIsCapturing = false;
    float CaptureDuration = 0.0f;
    float AccumulatedCaptureTime = 0.0f;

    // Tests secuenciales
    bool            bIsRunningSequentialTest = false;
    TArray<int32>   SequentialTestSteps;
    int32           CurrentSequentialStepIndex = 0;
    FTimerHandle    SequentialTestTimerHandle;

    enum class ESequentialTestType : uint8
    {
        PlanetScaling,
        ClosePlanetScaling,
        FoliageViewDistance,
        FoliagePerFrame,
        ClipmapResolution,
        ClipmapLevels,
        OrbitSimulation,
        NBodySimulation,
        SystemGeneration,
        None
    };
    ESequentialTestType CurrentSequentialTestType = ESequentialTestType::None;

    int32        AllTestsCurrentIndex = 0;
    bool         bIsRunningAllTests = false;
    FTimerHandle AllTestsTimerHandle;

    void RunNextAllTest();
    void OnAllTestsStepComplete();
    void OnBenchmarkCaptureComplete();
    void RunNextSequentialStep();
    void OnSequentialTestComplete();
};
