// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "CosmicMeshComponent.generated.h"

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

    // Malla base (deformada a la esfera, sin alturas adicionales)
    TArray<FVector> BaseVertices;
    TArray<FVector> BaseNormals;
    TArray<FProcMeshTangent> BaseTangents;

    // Alturas adicionales (para modificar en tiempo de ejecución)
    //TArray<float> HeightOffsets;

    // Malla final (base + alturas)
    TArray<FVector> CurrentVertices;
    TArray<int32> Triangles;
    TArray<FVector> CurrentNormals;
    TArray<FProcMeshTangent> CurrentTangents;
    TArray<FVector2D> UVs;

    void BuildBaseMesh();
    void BuildSphereMesh();
    void UpdateMesh();
    void SetMeshActive(bool active);
    void UpdateHeights(const FVector2D& Origin);
};

