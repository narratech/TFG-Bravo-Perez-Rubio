// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CosmicOctree.h"
#include "CosmicFoliageGenerationTask.h"
#include "CosmicFoliageSpawner.generated.h"

class UCosmicFoliageCollection;
class UCosmicNoiseSettings;

USTRUCT()
struct FCosmicFoliageCellData
{
    GENERATED_BODY()

    UPROPERTY()
    TMap<UStaticMesh*, UHierarchicalInstancedStaticMeshComponent*> MeshComponents;

    UPROPERTY()
    float LastUpdateTime = 0.0f;
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

    void InitFoliageSpawner(float PlanetRadius, UCosmicNoiseSettings* NoiseSettings);
    void UpdateFoliageSpawner(float DeltaTime, const FVector& ViewerLocation, const FVector& PlanetCenter, float PlanetRadius, float DistanceToSurface, UCosmicNoiseSettings* NoiseSettings);
    void CancelAsyncWork();

    // Configuración principal
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    UCosmicFoliageCollection* FoliageCollection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    float ViewDistanceKm = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    float UpdateInterval = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage", meta = (ClampMin = "0.05"))
    float CellSizeKm = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    int32 MaxInstancesPerFrame = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Debug")
    bool bDrawDebugCells = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Debug")
    FColor DebugCellColor = FColor::Green;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage|Debug")
    float DebugCellThickness = 20.0f;

protected:
    virtual void BeginPlay() override; 
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Octree manager
    FCosmicOctree Octree;

    // Celdas activas con sus instancias
    UPROPERTY()
    TMap<FCubeMapCell, FCosmicFoliageCellData> ActiveCells;

    // Debug: Dibujar celdas activas
    void DrawDebugCells(const FVector& PlanetCenter, float PlanetRadius);

    // Actualizar octree y generar celdas
    void UpdateOctreeAndGenerate(const FVector& ViewerLocation, float DistanceToSurface, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings);
    void UpdateFoliageGeneration(float DeltaTime, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings);
    void GenerateCellFoliage(const FCubeMapCell& Cell, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings);

private:
    float PlanetRadiusCm = 100000.f;
    float ElapsedTime = 0.0f;
    FRandomStream RandomStream;
    TSet<FCubeMapCell> PendingCells;
    TArray<FAsyncTask<FFoliageGenerationTask>*> ActiveTasks;
    UCosmicNoiseSettings* CurrentNoiseSettings;


    /** Aplica las instancias generadas al mundo */
    void ApplyGeneratedInstances(const FCubeMapCell& Cell, const TArray<FCosmicFoliageInstance>& Instances);

    UHierarchicalInstancedStaticMeshComponent* GetOrCreateCellComponent(FCosmicFoliageCellData& CellData,
        UStaticMesh* Mesh);

};
