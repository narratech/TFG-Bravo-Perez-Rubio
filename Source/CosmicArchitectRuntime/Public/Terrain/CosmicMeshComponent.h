// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "CosmicArchitectNoiseGenerator.h"
#include "CosmicMeshComponent.generated.h"

class UCosmicNoiseSettings;

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
    float PlanetRadius;
    bool bIsRing;
    bool bIsPlanet;
    bool bMeshCreated = false;
    bool bIsSphereMesh = false;
    bool bActiveMesh;

    FTransform PatchTransform;
    EClipmapQuadrant CurrentQuadrant = EClipmapQuadrant::BottomRight;

    UCosmicNoiseSettings* NoiseSettings = nullptr;
    // Malla base (deformada a la esfera, sin alturas adicionales)
    TArray<FVector> BaseVertices;
    TArray<FVector> BaseNormals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> BaseTangents;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy);

    void BuildBaseMesh();
    void BuildBaseProjectedMesh();
    void BuildBasePlainMesh();
    void BuildSphereMesh();
    void ReScaleLevel(int64 GridSpacing);

    void SetPositionAndRotation(const FVector& SurfacePos, const FRotator& PatchRotation);

    FVector ProjectToPlanet(const FVector& WorldPos, const FVector& PlanetCenter) const;
    void SetMeshActive(bool active);
    // Lanza la tarea de ruido
    void RequestMeshUpdate();
    // Comprueba si la tarea termino y aplica la malla
    bool CheckAndApplyMeshUpdate();
    bool IsTaskActive();
    void CancelAsyncWork();
    void ShiftLevel(FIntPoint Shift);
    int32 GetQuadrantIndex(EClipmapQuadrant Q) const;
    void SetHoleQuadrant(EClipmapQuadrant NewQuadrant);

    FAsyncTask<FCosmicArchitectNoiseGenerator>* NoiseTask = nullptr;
    bool bIsGeneratingNoise = false;
};

