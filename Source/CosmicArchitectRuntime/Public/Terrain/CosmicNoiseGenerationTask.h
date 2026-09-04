// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

class ICosmicNoiseStrategy;

/**
 * Datos opcionales para generar un nivel de clipmap sobre un marco tangente
 * estable. Las alturas y colores incluyen un halo de dos texels para poder
 * reconstruir normales y la transicion al siguiente nivel sin reevaluar ruido.
 */
struct FCosmicPlanetClipmapGenerationSettings
{
    bool bEnabled = false;
    bool bHasCoarserLevel = false;
    int32 GridResolution = 0;
    FTransform ProjectionFrame = FTransform::Identity;
    FIntPoint DesiredGridCenter = FIntPoint::ZeroValue;
    FIntPoint PreviousGridCenter = FIntPoint::ZeroValue;
    uint64 ProjectionRevision = 0;
    uint64 PreviousProjectionRevision = MAX_uint64;
    TArray<float> CachedHeights;
    TArray<FLinearColor> CachedColors;
};

/**
 * Tarea asincrona encargada de generar deformaciones de ruido,
 * normales y colores para una malla procedural.
 *
 * Esta tarea se ejecuta en segundo plano utilizando el sistema
 * de tareas asincronas de Unreal Engine para evitar bloquear
 * el hilo principal durante la generacion del terreno.
 */
class COSMICARCHITECTRUNTIME_API FCosmicNoiseGenerationTask : public FNonAbandonableTask
{
public:

    /** 
     * Referencia a los vertices base de la malla.
     *
     * Estos vertices representan la geometria original antes
     * de aplicar desplazamientos de ruido.
     */
    const TArray<FVector>& BaseVertices;

    /**
     * Vertices calculados tras aplicar el ruido procedural.
     */
    TArray<FVector> CalculatedVertices;

    /**
     * Normales calculadas para la geometria final.
     */
    TArray<FVector> CalculatedNormals;

    /**
     * Colores calculados para cada vertice.
     */
    TArray<FLinearColor> CalculatedColors;

    /** Cache desplazable que se devuelve al componente al terminar la tarea. */
    TArray<float> CalculatedHeightCache;
    TArray<FLinearColor> CalculatedColorCache;

    /** Centro y revision a los que pertenecen los resultados del clipmap. */
    FIntPoint CalculatedGridCenter = FIntPoint::ZeroValue;
    uint64 CalculatedProjectionRevision = 0;

    /**
     * Transformacion del componente propietario.
     */
    FTransform ComponentTransform;

    /**
     * Centro global del planeta.
     */
    FVector PlanetCenter;

    /**
     * Radio del planeta utilizado para las proyecciones esfericas.
     */
    double PlanetRadius;

    /**
     * Espaciado entre vertices de la grilla.
     */
    double GridSpacing;

    /**
     * Indica si la malla representa un planeta.
     */
    bool IsPlanet;

    /**
     * Indica si la malla corresponde a una esfera completa.
     */
    bool IsSphere;

    /**
     * Estrategia de ruido utilizada para evaluar alturas y colores.
     */
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

    /** Configuracion de la ruta incremental de clipmap planetario. */
    FCosmicPlanetClipmapGenerationSettings ClipmapSettings;

    /**
     * Constructor de la tarea asincrona de generacion procedural.
     *
     * @param InBaseVerts Vertices base de la malla.
     * @param InTransform Transformacion del componente.
     * @param InPlanetCenter Centro global del planeta.
     * @param InPlanetRadius Radio del planeta.
     * @param InGridSpacing Espaciado entre vertices.
     * @param InPlanet Indica si la malla es planetaria.
     * @param InIsSphere Indica si la geometria es una esfera.
     * @param InNoiseGenerationStrategy Estrategia de ruido utilizada.
     */
    FCosmicNoiseGenerationTask(
        const TArray<FVector>& InBaseVerts,
        FTransform InTransform,
        FVector InPlanetCenter,
        double InPlanetRadius,
        double InGridSpacing,
        bool InPlanet,
        bool InIsSphere,
        TSharedPtr<ICosmicNoiseStrategy> InNoiseGenerationStrategy,
        FCosmicPlanetClipmapGenerationSettings InClipmapSettings = {}
    );

    /**
     * Devuelve las estadisticas de ejecucion para profiling.
     */
    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FCosmicArchitectNoiseGenerator, STATGROUP_ThreadPoolAsyncTasks);
    }

    /**
     * Ejecuta el calculo procedural de vertices, normales y colores.
     */
    void DoWork();

private:

    /** Genera o desplaza la cache del clipmap y reconstruye solo sus salidas. */
    void DoSnappedPlanetClipmapWork();
};
