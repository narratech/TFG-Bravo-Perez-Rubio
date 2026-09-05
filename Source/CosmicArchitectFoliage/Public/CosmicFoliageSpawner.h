// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicOctree.h"
#include "CosmicFoliageGenerationTask.h"
#include "CosmicFoliageSpawner.generated.h"

class UCosmicFoliageCollection;
class ICosmicNoiseStrategy;
 
/**
 * Componente encargado de gestionar la generación y streaming de foliage
 * alrededor del jugador utilizando un sistema de CubeMap + Octree.
 *
 * Controla:
 * - Activación/desactivación de celdas por distancia.
 * - Ejecución de tareas asíncronas de generación.
 * - Aplicación de instancias mediante HISM.
 * - Gestión de capas de vegetación.
 */ 
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), 
    HideCategories = (Rendering, Lighting, Navigation, Replication, Physics,Collision,
        Activation, AssetUserData, HLOD, Cooking, Tags, ComponentReplication))
class COSMICARCHITECTFOLIAGE_API UCosmicFoliageSpawner : public UActorComponent
{
	GENERATED_BODY()

public:
    /**
     * Constructor por defecto.
     */
    UCosmicFoliageSpawner();

    /**
     * Inicializa el sistema de spawning de foliage.
     *
     * @param PlanetRadius Radio del planeta.
     */
    void InitFoliageSpawner(float PlanetRadius);

    /**
     * Actualiza el sistema de foliage según la posición del jugador.
     *
     * @param DeltaTime Tiempo entre frames.
     * @param ViewerLocation Posición del observador.
     * @param PlanetCenter Centro del planeta.
     * @param PlanetRadius Radio del planeta.
     * @param DistanceToSurface Distancia a la superficie.
     * @param NoiseGenerationStrategy Estrategia de ruido ambiental.
     */
    void UpdateFoliageSpawner(float DeltaTime, const FVector& ViewerLocation, const FVector& PlanetCenter, double PlanetRadius, double DistanceToSurface, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);

    /**
     * Cancela todas las tareas asíncronas activas.
     */
    void CancelAsyncWork();

    /**
     * Limpia todas las instancias de foliage generadas.
     */
    void ClearFoliage();

    /** Colección de foliage utilizada para la generación */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    UCosmicFoliageCollection* FoliageCollection;

    /** Radio de activación de capa cercana */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage", meta = (ClampMin = "0.02"))
    float NearLayerRadiusKm = 0.05f;

    /** Radio de activación de capa media */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage", meta = (ClampMin = "0.02"))
    float MediumLayerRadiusKm = 0.2f;

    /** Radio de activación de capa lejana */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage", meta = (ClampMin = "0.02"))
    float FarLayerRadiusKm = 0.5f;

    /** Máximo de instancias aplicadas al mundo por frame */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance", meta = (ClampMin = "1"))
    int32 MaxInstancesGeneratedPerFrame = 100;

    /** Numero maximo de celdas calculadas simultaneamente en el thread pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance", meta = (ClampMin = "1", ClampMax = "64"))
    int32 MaxConcurrentGenerationTasks = 16;

    /** Limite de seguridad para impedir celdas configuradas con millones de instancias. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance", meta = (ClampMin = "1", ClampMax = "100000"))
    int32 MaxInstancesPerCell = 1000;

    /** Distancia usada para estimar la normal procedural del terreno. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance", meta = (ClampMin = "1.0"))
    float NormalSampleDistanceCm = 500.0f;

    /** Fraccion del radio que debe desplazarse el observador antes de consultar de nuevo el octree. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance", meta = (ClampMin = "0.001", ClampMax = "0.25"))
    float VisibilityUpdateDistanceRatio = 0.02f;

    /** Orden de prioridad para generar, aplicar y retirar las capas (por defecto: 1.Far 2.Medium 3.Near). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Performance")
    TArray<ECosmicFoliageLayer> FoliageLayerPriority;

protected:
    virtual void BeginDestroy() override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
    virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    /**
     * Octree responsable de la subdivisión espacial del planeta.
     */
    FCosmicOctree Octree;

    /**
     * Estructura de celdas activas por capa de foliage.
     */
    UPROPERTY()
    FCosmicFoliageLayerCells LayerCells[3];

    void ProcessApplyQueue(const FVector& ViewerDir, int32& RemainingInstanceBudget);
    void ProcessDeactivationQueue(int32& RemainingInstanceBudget);
    void UpdateOctreeAndGenerate(const FVector& ViewerLocation, double DistanceToSurface, const FVector& PlanetCenter);
    void UpdateFoliageGeneration();
    void GenerateCellFoliage(const FCubeMapCell& Cell, double PlanetRadius, ECosmicFoliageLayer Layer, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);
    void StartQueuedGenerationTasks(const FVector& ViewerDir, double PlanetRadius, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);
    void ClearDelegates();

private:

    struct FPendingQueuedCell
    {
        FCubeMapCell Cell;
        FVector UnitDirection = FVector::UpVector;
    };

    struct FPendingApplyCell
    {
        FCubeMapCell Cell;
        ECosmicFoliageLayer Layer;
        FVector UnitDirection = FVector::UpVector;
        TArray<FCosmicFoliageInstance> Instances;
        int32 NextInstanceIndex = 0;
    };

    TArray<FPendingApplyCell> ApplyQueues[3];
    TArray<FCubeMapCell> PendingDeactivation[3];
    TArray<FPendingQueuedCell> QueuedCells[3];
    TSet<FCubeMapCell> PendingDeactivationCells[3];
    TSet<FCubeMapCell> CellsBeingDeactivated[3];

    TSet<FCubeMapCell> CurrentVisibleCells[3];

    UPROPERTY()
    TMap<FCosmicHISMKey, FCosmicSharedHISMData> SharedHISMs;

    TSet<FCubeMapCell> PendingCells[3];
    TArray<FAsyncTask<FFoliageGenerationTask>*> ActiveTasks[3];

    /** Cache ligera para no recorrer el octree cuando no puede cambiar la cobertura. */
    FVector LastVisibilityQueryLocation[3];
    float LastVisibilityRadiusKm[3] = { 0.0f, 0.0f, 0.0f };
    bool bVisibilityQueryValid[3] = { false, false, false };
    bool bLayerWasEnabled[3] = { false, false, false };
    uint8 ConfiguredLayerMask = 0;
    bool bLayerMaskDirty = true;
    TSharedPtr<const TArray<FCosmicFoliageCollectionEntry>, ESPMode::ThreadSafe> FoliageEntriesSnapshot;

    float GetLayerRadius(ECosmicFoliageLayer Layer) const;
    int32 GetActiveTaskCount() const;
    void GetLayerPriorityIndices(int32 OutLayerIndices[3]) const;
    void RefreshConfiguredLayerMask();
    void CancelLayerAsyncWork(int32 LayerIndex);
    /** Reinicia las colas y la cache de visibilidad de una capa. */
    void ResetLayerState(int32 LayerIndex);
    /** Cancela la generacion y retira las instancias de una sola capa. */
    void ClearFoliageLayer(ECosmicFoliageLayer Layer);
    /** Aplica las instancias generadas al mundo */
    void ApplyGeneratedInstances(const FCubeMapCell& Cell, ECosmicFoliageLayer Layer, TArrayView<const FCosmicFoliageInstance> Instances);

    FCosmicSharedHISMData* GetOrCreateSharedHISM(const FCosmicHISMKey& Key);
    int32 RemoveCellInstances(int32 LayerIndex, const FCubeMapCell& Cell, int32 InstanceBudget);

};
