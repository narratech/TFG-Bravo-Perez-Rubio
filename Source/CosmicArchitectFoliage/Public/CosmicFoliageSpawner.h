// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CosmicOctree.h"
#include "CosmicFoliageTypes.h"
#include "CosmicFoliageGenerationTask.h"
#include "CosmicFoliageCollection.h"
#include "CosmicFoliageSpawner.generated.h"

class UCosmicNoiseSettings;

USTRUCT()
struct FCosmicMeshIndices
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<int32> Indices;
};

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

    void InitFoliageSpawner(float PlanetRadius);

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

    UPROPERTY()
    TMap<UStaticMesh*, UHierarchicalInstancedStaticMeshComponent*> MeshComponents;

    // Mapa de instancias activas por celda
    UPROPERTY()
    TMap<FIntVector, FCosmicFoliageCellData> CellDataMap;

    void UpdateFoliageGeneration(float DeltaTime, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings);

    /** Solicita generación para un área */
    void RequestAreaGeneration(const FVector& Center, float RadiusKm, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings);

    /** Limpia instancias fuera de rango */
    void CleanupFarInstances(const FVector& ViewerLocation, float MaxDistanceKm);

protected:
    virtual void BeginPlay() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);

    // Octree manager
    FCosmicOctree Octree;

    // Celdas activas con sus instancias
    UPROPERTY()
    TMap<FCubeMapCell, FCosmicFoliageCellData> ActiveCells;

    // Debug: Dibujar celdas activas
    void DrawDebugCells(const FVector& PlanetCenter, float PlanetRadius);

    // Actualizar octree y generar celdas
    void UpdateOctreeAndGenerate(const FVector& ViewerLocation, const FVector& PlanetCenter, float PlanetRadius);

    void GenerateCellFoliage(const FCubeMapCell& Cell, const FVector& PlanetCenter, float PlanetRadius);

private:
    float PlanetRadiusCm = 100000.f;
    float ElapsedTime = 0.0f;
    FRandomStream RandomStream;
    TSet<FIntVector> PendingGenerationCells;
    TQueue<FIntVector> PendingCells;
    TArray<FAsyncTask<FFoliageGenerationTask>*> ActiveTasks;

    /** Convierte coordenadas mundo a celda de grid */
    FIntVector WorldToCell(const FVector& WorldPos) const;

    /** Genera instancias para una celda específica (puede ser llamado desde hilo) */
    void GenerateCell(const FIntVector& Cell, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings);

    /** Aplica las instancias generadas al mundo */
    void ApplyGeneratedInstances(const FIntVector& Cell, const TArray<FCosmicFoliageInstance>& Instances);

    UHierarchicalInstancedStaticMeshComponent* GetOrCreateComponent(UStaticMesh* Mesh); 

    UHierarchicalInstancedStaticMeshComponent* GetOrCreateCellComponent(FCosmicFoliageCellData& CellData,
        UStaticMesh* Mesh);

};
