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

    /** Definición del mesh de foliage asociado */
    const FCosmicFoliageMesh* MeshDef = nullptr;
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
        FVector Direction;      // Dirección desde el centro del planeta
        FVector WorldPosition;  // Posición en el mundo
        FVector CachedNormal;
        float Temperature;
        float Humidity;
        float Height;
        float Slope;
    }; 
    /**
     * Constructor de la tarea de generación de foliage.
     */
    FFoliageGenerationTask(
        const FCubeMapCell& InCell,
        ECosmicFoliageLayer InLayer,
        UCosmicFoliageCollection* InCollection,
        const FVector& InPlanetCenter,
        float InPlanetRadius,
        TSharedPtr<ICosmicNoiseStrategy> InNoiseGenerationStrategy,
        float InCellAreaKm2)
        : Cell(InCell)
        , Layer(InLayer)
        , Collection(InCollection)
        , PlanetCenter(InPlanetCenter)
        , PlanetRadius(InPlanetRadius)
        , NoiseGenerationStrategy(InNoiseGenerationStrategy)
        , CellAreaKm2(InCellAreaKm2)
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

    /** Colección de foliage utilizada como base */
    UCosmicFoliageCollection* Collection;

    /** Centro del planeta */
    FVector PlanetCenter;

    /** Radio del planeta */
    float PlanetRadius;

    /** Estrategia de generación de ruido */
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

    /** Área de la celda en km² */
    float CellAreaKm2;

    /** Puntos generados para evaluación */
    TArray<FSeedPoint> SeedPoints;

    /**
     * Genera puntos de semilla dentro de la celda.
     */
    void GenerateSeedPoints(FRandomStream& Random);

    /**
     * Evalúa condiciones ambientales (temperatura, humedad, altura, etc).
     */
    void EvaluateEnvironmentalConditions(FRandomStream& Random);

    /**
     * Crea instancias finales de foliage a partir de los seed points.
     */
    void CreateFoliageInstances(FRandomStream& Random);

    /**
     * Calcula pendiente y normal del terreno en un punto.
     */
    void CalculateSlopeAndNormal(
        const FVector& Direction,
        float& OutSlope,
        FVector& OutNormal);
};
