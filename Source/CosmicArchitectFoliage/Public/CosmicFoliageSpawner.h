// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.

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

    /** Máximo de instancias procesadas por frame */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    int32 MaxInstancesPerFrame = 100;

    /** Activa debug visual de celdas */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Debug")
    bool bDrawDebugCells = false;

    /** Grosor de líneas de debug */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Debug")
    float DebugCellThickness = 20.0f;

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

    // Debug: Dibujar celdas activas
    void DrawDebugCells(const FVector& PlanetCenter, double PlanetRadius);

    void ProcessApplyQueue();
    void ProcessDeactivationQueue();
    void UpdateOctreeAndGenerate(const FVector& ViewerLocation, double DistanceToSurface, const FVector& PlanetCenter, double PlanetRadius, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);
    void UpdateFoliageGeneration();
    void GenerateCellFoliage(const FCubeMapCell& Cell, const FVector& PlanetCenter, double PlanetRadius, ECosmicFoliageLayer Layer, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);
    void ClearDelegates();

private:

    struct FPendingApplyCell
    {
        FCubeMapCell Cell;
        ECosmicFoliageLayer Layer;
        TArray<FCosmicFoliageInstance> Instances;
    };

    TArray<FPendingApplyCell> ApplyQueues[3];
    TArray<FCubeMapCell> PendingDeactivation[3];

    TSet<FCubeMapCell> CurrentVisibleCells[3];

    UPROPERTY()
    TMap<FCosmicHISMKey, FCosmicHISMPoolList> FreeHISMPool;

    FRandomStream RandomStream;
    TSet<FCubeMapCell> PendingCells[3];
    TArray<FAsyncTask<FFoliageGenerationTask>*> ActiveTasks[3];

    float GetLayerRadius(ECosmicFoliageLayer Layer) const;
    FColor GetLayerColor(ECosmicFoliageLayer Layer) const;
    /** Aplica las instancias generadas al mundo */
    void ApplyGeneratedInstances(const FCubeMapCell& Cell, ECosmicFoliageLayer Layer, const TArray<FCosmicFoliageInstance>& Instances);

    UHierarchicalInstancedStaticMeshComponent* AcquireHISM(const FCosmicHISMKey& Key);
    void ReleaseHISM(const FCosmicHISMKey& Key, UHierarchicalInstancedStaticMeshComponent* Comp);

};
