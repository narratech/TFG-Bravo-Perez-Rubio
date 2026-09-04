// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CosmicFoliageCollection.h"
#include "CosmicArchitectCommon/Public/CosmicCubeMapCell.h"
#include "CosmicFoliageGenerationTask.generated.h"

class ICosmicNoiseStrategy;

/**
 * Instancia generada de foliage resultante del sistema procedural.
 */
USTRUCT()
struct FCosmicFoliageInstance
{
    GENERATED_BODY()

    /** HISM compartido al que pertenece la instancia. */
    FCosmicHISMKey HISMKey;

    /** Transform final de la instancia */
    FTransform Transform;
};


/**
 * Tarea asíncrona encargada de generar instancias de foliage
 * para una celda del CubeMap del planeta.
 *
 * Ejecuta la generación procedural basada en ruido, condiciones
 * ambientales y distribución por capas de vegetación.
 */
class FFoliageGenerationTask : public FNonAbandonableTask
{
public:
    /** Resultados generados de instancias de foliage */
    TArray<FCosmicFoliageInstance> ResultInstances;
    /** Celda del CubeMap a procesar */
    FCubeMapCell Cell;
    /** Capa de foliage que se está generando */
    ECosmicFoliageLayer Layer;

    /**
     * Representa un punto de muestreo utilizado durante la generación de foliage.
     */
    struct FSeedPoint
    {
        FVector Direction = FVector::UpVector;      // Dirección desde el centro del planeta
        FVector WorldPosition = FVector::ZeroVector; // Posición relativa al planeta
        FVector CachedNormal = FVector::UpVector;
        int32 AllocationIndex = INDEX_NONE;
        float Temperature = 0.0f;
        float Humidity = 0.0f;
        float Height = 0.0f;
        float Slope = 0.0f;
    }; 
    /**
     * Constructor de la tarea de generación de foliage.
     */
    FFoliageGenerationTask(
        const FCubeMapCell& InCell,
        ECosmicFoliageLayer InLayer,
        TSharedPtr<const TArray<FCosmicFoliageCollectionEntry>, ESPMode::ThreadSafe> InFoliageEntries,
        double InPlanetRadius,
        TSharedPtr<ICosmicNoiseStrategy> InNoiseGenerationStrategy,
        int32 InMaxInstancesPerCell,
        float InNormalSampleDistanceCm)
        : Cell(InCell)
        , Layer(InLayer)
        , FoliageEntries(MoveTemp(InFoliageEntries))
        , PlanetRadius(InPlanetRadius)
        , NoiseGenerationStrategy(InNoiseGenerationStrategy)
        , MaxInstancesPerCell(InMaxInstancesPerCell)
        , NormalSampleDistanceCm(InNormalSampleDistanceCm)
    {
    }

    /** Identificador de estadísticas para el sistema de threading */
    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FFoliageGenerationTask, STATGROUP_ThreadPoolAsyncTasks);
    }
    /** Ejecución principal de la tarea */
    void DoWork();

private:

    /** Snapshot inmutable creado una sola vez en el game thread y compartido entre tareas. */
    TSharedPtr<const TArray<FCosmicFoliageCollectionEntry>, ESPMode::ThreadSafe> FoliageEntries;

    /** Radio del planeta */
    double PlanetRadius;

    /** Estrategia de generación de ruido */
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

    /** Área de la celda en km² */
    double CellAreaKm2 = 0.0;

    /** Limites de seguridad y muestreo configurados por el spawner. */
    int32 MaxInstancesPerCell;
    float NormalSampleDistanceCm;

    /** Puntos generados para evaluación */
    TArray<FSeedPoint> SeedPoints;

    struct FMeshAllocation
    {
        int32 EntryIndex = INDEX_NONE;
        int32 MeshIndex = INDEX_NONE;
        int32 TargetCount = 0;
        bool bNeedsSurfaceNormal = true;
    };

    TArray<FMeshAllocation> Allocations;

    /** Calcula el area real del cuadrilatero esferico de la celda. */
    double CalculateCellAreaKm2() const;

    /** Precalcula cuotas deterministas, incluyendo redondeo estocastico. */
    int32 PrepareAllocations(FRandomStream& Random);

    /**
     * Genera puntos de semilla dentro de la celda.
     */
    void GenerateSeedPoints(FRandomStream& Random);

    /**
     * Evalúa condiciones ambientales (temperatura, humedad, altura, etc).
     */
    void EvaluateEnvironmentalConditions();

    /**
     * Crea instancias finales de foliage a partir de los seed points.
     */
    void CreateFoliageInstances(FRandomStream& Random);

    /**
     * Calcula pendiente y normal del terreno en un punto.
     */
    void CalculateSlopeAndNormal(
        const FVector& Direction,
        float CenterHeight,
        float& OutSlope,
        FVector& OutNormal);
};
