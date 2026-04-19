// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CosmicOctree.h"
#include "CosmicFoliageGenerationTask.h"
#include "CosmicFoliageSpawner.generated.h"

class UCosmicFoliageCollection;
class ICosmicNoiseStrategy;

USTRUCT()
struct FCosmicFoliageCellData
{
    GENERATED_BODY()

    UPROPERTY()
    TMap<UStaticMesh*, UHierarchicalInstancedStaticMeshComponent*> MeshComponents;

};

// Estructura para almacenar celdas por capa
USTRUCT()
struct FCosmicFoliageLayerCells
{
    GENERATED_BODY()

    // Mapa de celda: datos de componentes para esta capa específica
    UPROPERTY()
    TMap<FCubeMapCell, FCosmicFoliageCellData> ActiveCells;
};

/**
 * Componente que gestiona el spawning de foliage cerca del jugador
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COSMICARCHITECTFOLIAGE_API UCosmicFoliageSpawner : public UActorComponent
{
	GENERATED_BODY()

public:
    UCosmicFoliageSpawner();

    void InitFoliageSpawner(float PlanetRadius);
    void UpdateFoliageSpawner(float DeltaTime, const FVector& ViewerLocation, const FVector& PlanetCenter, float PlanetRadius, float DistanceToSurface, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);
    void CancelAsyncWork();
    void ClearFoliage();

    // Configuración principal
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    UCosmicFoliageCollection* FoliageCollection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage", meta = (ClampMin = "0.02"))
    float NearLayerRadiusKm = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage", meta = (ClampMin = "0.02"))
    float MediumLayerRadiusKm = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage", meta = (ClampMin = "0.02"))
    float FarLayerRadiusKm = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    float UpdateInterval = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    int32 MaxInstancesPerFrame = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Debug")
    bool bDrawDebugCells = false;

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

    // Octree manager
    FCosmicOctree Octree;

    // Celdas activas independientes para cada capa
    UPROPERTY()
    FCosmicFoliageLayerCells LayerCells[3];

    // Debug: Dibujar celdas activas
    void DrawDebugCells(const FVector& PlanetCenter, float PlanetRadius);

    // Actualizar octree y generar celdas
    void UpdateOctreeAndGenerate(const FVector& ViewerLocation, float DistanceToSurface, const FVector& PlanetCenter, float PlanetRadius, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);
    void UpdateFoliageGeneration(float DeltaTime, const FVector& PlanetCenter, float PlanetRadius, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);
    void GenerateCellFoliage(const FCubeMapCell& Cell, const FVector& PlanetCenter, float PlanetRadius, ECosmicFoliageLayer Layer, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);
    void ClearDelegates();

private:
    float ElapsedTime = 0.0f;
    FRandomStream RandomStream;
    TSet<FCubeMapCell> PendingCells[3];
    TArray<FAsyncTask<FFoliageGenerationTask>*> ActiveTasks[3];

    float GetLayerRadius(ECosmicFoliageLayer Layer) const;
    FColor GetLayerColor(ECosmicFoliageLayer Layer) const;
    /** Aplica las instancias generadas al mundo */
    void ApplyGeneratedInstances(const FCubeMapCell& Cell, ECosmicFoliageLayer Layer, const TArray<FCosmicFoliageInstance>& Instances);

    UHierarchicalInstancedStaticMeshComponent* GetOrCreateCellComponent(FCosmicFoliageCellData& CellData,
        UStaticMesh* Mesh);

};
