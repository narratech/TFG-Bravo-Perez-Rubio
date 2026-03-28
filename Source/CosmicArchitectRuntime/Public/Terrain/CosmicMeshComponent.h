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
    bool bMeshCreated = false;
    bool bActiveMesh;
    FVector ShiftOffset;

    EClipmapQuadrant CurrentQuadrant = EClipmapQuadrant::BottomRight;

    UCosmicNoiseSettings* NoiseSettings = nullptr;
    // Malla base (deformada a la esfera, sin alturas adicionales)
    TArray<FVector> BaseVertices;
    TArray<FVector> BaseNormals;
    TArray<FProcMeshTangent> BaseTangents;

    // Malla final (base + alturas)
    TArray<FVector> CurrentVertices;
    TArray<FLinearColor> CurrentColors;
    TArray<int32> Triangles;
    TArray<FVector> CurrentNormals;
    TArray<FProcMeshTangent> CurrentTangents;
    TArray<FVector2D> UVs;

    void BuildBaseMesh();
    void BuildSphereMesh();
    void ReScaleLevel(int64 GridSpacing, const FVector ActorPosition);
    void SetMeshActive(bool active);
    // Lanza la tarea de ruido
    void RequestMeshUpdate();
    // Comprueba si la tarea termino y aplica la malla
    bool CheckAndApplyMeshUpdate();
    void ShiftLevel(FIntPoint Shift);     
    int32 GetQuadrantIndex(EClipmapQuadrant Q) const;
    void SetHoleQuadrant(EClipmapQuadrant NewQuadrant);

    FAsyncTask<FCosmicArchitectNoiseGenerator>* NoiseTask = nullptr;
    bool bIsGeneratingNoise = false;
};

