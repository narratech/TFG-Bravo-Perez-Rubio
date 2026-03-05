// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicFoliageTypes.h"
#include "CosmicFoliageCollection.h"
#include "CosmicFoliageSpawner.generated.h"

/**
 * Tarea asincrona para calcular posiciones de foliage
 */
class FFoliageGenerationTask : public FNonAbandonableTask
{
public:
    TArray<FTransform> ResultTransforms;
    FBox SpawnArea;
    UCosmicFoliageCollection* Collection;
    int32 Seed;
    float WorldToKmScale;

    FFoliageGenerationTask(const FBox& InArea, UCosmicFoliageCollection* InCollection, int32 InSeed, float InScale)
        : SpawnArea(InArea), Collection(InCollection), Seed(InSeed), WorldToKmScale(InScale) {
    }

    FORCEINLINE TStatId GetStatId() const
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(FFoliageGenerationTask, STATGROUP_ThreadPoolAsyncTasks);
    }

    void DoWork();
};

USTRUCT(BlueprintType)
struct FCosmicFoliageCellData
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FTransform> Instances;

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

    // Configuración principal
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    UCosmicFoliageCollection* FoliageCollection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    float ViewDistanceKm = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    float UpdateInterval = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    int32 MaxInstancesPerFrame = 1000;

    // Mapa de instancias activas por celda
    UPROPERTY()
    TMap<FIntVector, FCosmicFoliageCellData> CellDataMap;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Solicita generación para un área */
    void RequestAreaGeneration(const FVector& Center, float RadiusKm);

    /** Limpia instancias fuera de rango */
    void CleanupFarInstances(const FVector& ViewerLocation, float MaxDistanceKm);

protected:
    virtual void BeginPlay() override;

private:
    float ElapsedTime = 0.0f;
    FRandomStream RandomStream;
    TQueue<FIntVector> PendingCells;
    TArray<FAsyncTask<FFoliageGenerationTask>*> ActiveTasks;

    /** Convierte coordenadas mundo a celda de grid */
    FIntVector WorldToCell(const FVector& WorldPos, float CellSizeKm) const;

    /** Genera instancias para una celda específica (puede ser llamado desde hilo) */
    void GenerateCell(const FIntVector& Cell);

    /** Aplica las instancias generadas al mundo */
    void ApplyGeneratedInstances(const FIntVector& Cell, const TArray<FTransform>& Transforms);

		
};
