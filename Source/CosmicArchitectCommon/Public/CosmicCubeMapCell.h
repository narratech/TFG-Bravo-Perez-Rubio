// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicCubeMapCell.generated.h"

/**
 *
 */
USTRUCT()
struct COSMICARCHITECTCOMMON_API FCubeMapCell
{
    GENERATED_BODY()

    int32 Face = 0;          // 0-5 (las 6 caras del cubo)
    int32 X = 0;             // Coordenada X en la cara (0 a 2^Depth - 1)
    int32 Y = 0;             // Coordenada Y en la cara
    int32 Depth = 0;         // Nivel de subdivisión (0 = cara completa)

    // Operadores para usar como clave en TMap
    bool operator==(const FCubeMapCell& Other) const
    {
        return Face == Other.Face && X == Other.X && Y == Other.Y && Depth == Other.Depth;
    }

    friend uint32 GetTypeHash(const FCubeMapCell& Cell)
    {
        uint32 Hash = 0;
        Hash = HashCombine(Hash, GetTypeHash(Cell.Face));
        Hash = HashCombine(Hash, GetTypeHash(Cell.X));
        Hash = HashCombine(Hash, GetTypeHash(Cell.Y));
        Hash = HashCombine(Hash, GetTypeHash(Cell.Depth));
        return Hash;
    }

    // Método para obtener el ID como string 
    FString ToString() const
    {
        return FString::Printf(TEXT("F%d_X%d_Y%d_D%d"), Face, X, Y, Depth);
    }
};

// Estructura para los limites del nodo
struct FNodeBounds
{
    FVector MinCorner;      // Esquina minima en el cubo
    FVector MaxCorner;      // Esquina maxima en el cubo
    FVector Center;         // Centro en el cubo

    // Para debugging
    TArray<FVector> GetCubeCorners() const
    {
        TArray<FVector> Corners;
        Corners.Add(MinCorner);
        Corners.Add(FVector(MaxCorner.X, MinCorner.Y, MinCorner.Z));
        Corners.Add(FVector(MaxCorner.X, MaxCorner.Y, MinCorner.Z));
        Corners.Add(FVector(MinCorner.X, MaxCorner.Y, MinCorner.Z));
        Corners.Add(FVector(MinCorner.X, MinCorner.Y, MaxCorner.Z));
        Corners.Add(FVector(MaxCorner.X, MinCorner.Y, MaxCorner.Z));
        Corners.Add(MaxCorner);
        Corners.Add(FVector(MinCorner.X, MaxCorner.Y, MaxCorner.Z));
        return Corners;
    }

    // Obtener las 8 esquinas en coordenadas de esfera
    TArray<FVector> GetSphereCorners(float Radius) const
    {
        TArray<FVector> Corners;

        // 8 esquinas de la celda en el cubo
        Corners.Add(FVector(MinCorner.X, MinCorner.Y, MinCorner.Z).GetSafeNormal() * Radius);
        Corners.Add(FVector(MaxCorner.X, MinCorner.Y, MinCorner.Z).GetSafeNormal() * Radius);
        Corners.Add(FVector(MaxCorner.X, MaxCorner.Y, MinCorner.Z).GetSafeNormal() * Radius);
        Corners.Add(FVector(MinCorner.X, MaxCorner.Y, MinCorner.Z).GetSafeNormal() * Radius);
        Corners.Add(FVector(MinCorner.X, MinCorner.Y, MaxCorner.Z).GetSafeNormal() * Radius);
        Corners.Add(FVector(MaxCorner.X, MinCorner.Y, MaxCorner.Z).GetSafeNormal() * Radius);
        Corners.Add(FVector(MaxCorner.X, MaxCorner.Y, MaxCorner.Z).GetSafeNormal() * Radius);
        Corners.Add(FVector(MinCorner.X, MaxCorner.Y, MaxCorner.Z).GetSafeNormal() * Radius);

        return Corners;
    }
};