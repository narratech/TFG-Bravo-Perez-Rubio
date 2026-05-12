// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "CosmicArchitectNoiseGenerator.h"
#include "CosmicMeshComponent.generated.h"

class ICosmicNoiseStrategy;

UENUM(BlueprintType)
enum class EClipmapQuadrant : uint8
{
    TopLeft = 0,     // Posición base inicial
    TopRight = 1,    // Movido a la derecha
    BottomLeft = 2,  // Movido hacia abajo
    BottomRight = 3  // Movido derecha y abajo
};

/**
 * 
 */
UCLASS()
class UCosmicMeshComponent : public UProceduralMeshComponent
{
    GENERATED_BODY()

public:
    int32 LevelIndex;
    int32 Resolution;
    int64 GridSpacing;
    double PlanetRadius;
    bool bIsRing;
    bool bIsPlanet;
    bool bMeshCreated = false;
    bool bIsSphereMesh = false;
    bool bActiveMesh;

    FTransform PatchTransform;

    // Malla base (deformada a la esfera, sin alturas adicionales)
    TArray<FVector> BaseVertices;
    TArray<FVector> BaseNormals;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy);

    void BuildBaseProjectedMesh();
    void BuildSphereMesh();
    void ReScaleLevel(int64 GridSpacing);
    void SetPositionAndRotation(const FVector& SurfacePos, const FRotator& PatchRotation);
    void SetMeshActive(bool active);
    // Lanza la tarea de ruido
    void RequestMeshUpdate(TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);
    // Comprueba si la tarea termino y aplica la malla
    bool CheckAndApplyMeshUpdate();
    bool IsTaskActive();
    void CancelAsyncWork();

protected:
    FAsyncTask<FCosmicArchitectNoiseGenerator>* NoiseTask = nullptr;
    bool bIsGeneratingNoise = false;
};

