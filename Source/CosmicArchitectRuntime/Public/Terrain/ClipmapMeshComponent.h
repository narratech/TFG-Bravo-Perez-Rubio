// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "ClipmapMeshComponent.generated.h"

/**
 * 
 */
UCLASS()
class UClipmapMeshComponent : public UProceduralMeshComponent
{
    GENERATED_BODY()

public:
    int32 LevelIndex;
    int32 Resolution;
    float GridSpacing;
    float PlanetRadius;
    bool bIsRing;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;

    void BuildMesh();
    void UpdateHeights(const FVector2D& Origin);
};

