// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CubeMapCell.generated.h"

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

    // Método para obtener el ID como string (útil para debugging)
    FString ToString() const
    {
        return FString::Printf(TEXT("F%d_X%d_Y%d_D%d"), Face, X, Y, Depth);
    }
};

// Estructura para los límites del nodo
struct FNodeBounds
{
    FVector MinCorner;      // Esquina mínima en el cubo
    FVector MaxCorner;      // Esquina máxima en el cubo
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
    TArray<FVector> GetSphereCorners(float PlanetRadius) const
    {
        TArray<FVector> Corners = GetCubeCorners();
        TArray<FVector> SphereCorners;

        for (const FVector& Corner : Corners)
        {
            // Proyectar a la esfera
            FVector Dir = Corner.GetSafeNormal();
            SphereCorners.Add(Dir * PlanetRadius);
        }

        return SphereCorners;
    }
};