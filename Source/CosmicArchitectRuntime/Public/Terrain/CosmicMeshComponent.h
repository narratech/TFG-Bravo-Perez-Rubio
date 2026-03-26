// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "CosmicArchitectNoiseGenerator.h"
#include "CosmicMeshComponent.generated.h"

class UCosmicNoiseSettings;

enum class EShiftDirection
{
    None,
    X_Pos,
    X_Neg,
    Y_Pos,
    Y_Neg
};

UENUM(BlueprintType)
enum class EClipmapQuadrant : uint8
{
    TopLeft = 0,     // Posición base inicial
    TopRight = 1,    // Movido a la derecha
    BottomLeft = 2,  // Movido hacia abajo
    BottomRight = 3  // Movido derecha y abajo
};

struct FClipmapHoleState
{
    EClipmapQuadrant CurrentQuadrant = EClipmapQuadrant::BottomRight;
    int32 OffsetX = 0;
    int32 OffsetY = 0;  
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
    float GridSpacing;
    float PlanetRadius;
    bool bIsRing;
    bool bMeshCreated = false;
    bool bActiveMesh;
    FIntPoint PendingShift;

    FClipmapHoleState HoleState;

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
    void RegenerateLevel(float GridSpacing);
    void SetMeshActive(bool active);
    // Lanza la tarea de ruido
    void RequestMeshUpdate();
    // Comprueba si la tarea termino y aplica la malla
    bool CheckAndApplyMeshUpdate();
    
    void ShiftLevel(FIntPoint Shift);
    EShiftDirection GetShiftDirection(FIntPoint Shift);
    bool NeedsToShift(FIntPoint Shift);
    int32 GetQuadrantIndex(EClipmapQuadrant Q) const;
    void RotateLevel(bool FlipX, bool FlipY);
    void SetHoleQuadrant(EClipmapQuadrant NewQuadrant);

    FAsyncTask<FCosmicArchitectNoiseGenerator>* NoiseTask = nullptr;
    bool bIsGeneratingNoise = false;
};

