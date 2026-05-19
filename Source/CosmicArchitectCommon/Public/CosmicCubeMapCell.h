// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicCubeMapCell.generated.h"

/**
 * Representa una celda dentro de un cubemap subdividido.
 *
 * Cada celda identifica una region especifica de una de las
 * seis caras del cubo utilizadas para la proyeccion planetaria.
 */
USTRUCT()
struct COSMICARCHITECTCOMMON_API FCubeMapCell
{
    GENERATED_BODY()

    /**
     * Cara del cubo a la que pertenece la celda.
     *
     * Valores validos: 0-5.
     */
    int32 Face = 0;

    /**
     * Coordenada X de la celda dentro de la cara.
     */
    int32 X = 0;

    /**
     * Coordenada Y de la celda dentro de la cara.
     */
    int32 Y = 0;

    /**
     * Nivel de subdivisión de la celda.
     *
     * Un valor de 0 representa la cara completa.
     */
    int32 Depth = 0;

    /**
     * Operador de comparacion utilizado para estructuras hash y TMap.
     */
    bool operator==(const FCubeMapCell& Other) const
    {
        return Face == Other.Face && X == Other.X && Y == Other.Y && Depth == Other.Depth;
    }

    /**
     * Genera un hash unico para la celda.
     */
    friend uint32 GetTypeHash(const FCubeMapCell& Cell)
    {
        uint32 Hash = 0;
        Hash = HashCombine(Hash, GetTypeHash(Cell.Face));
        Hash = HashCombine(Hash, GetTypeHash(Cell.X));
        Hash = HashCombine(Hash, GetTypeHash(Cell.Y));
        Hash = HashCombine(Hash, GetTypeHash(Cell.Depth));
        return Hash;
    }

    /**
     * Devuelve un identificador legible de la celda.
     */
    FString ToString() const
    {
        return FString::Printf(TEXT("F%d_X%d_Y%d_D%d"), Face, X, Y, Depth);
    }
};

/**
 * Representa los limites espaciales de un nodo del cubemap.
 *
 * Contiene informacion de las esquinas y centro de una region
 * tanto en espacio cubico como proyectado a esfera.
 */
struct FNodeBounds
{
    /**
     * Esquina minima del nodo en espacio cubico.
     */
    FVector MinCorner;

    /**
     * Esquina maxima del nodo en espacio cubico.
     */
    FVector MaxCorner;

    /**
     * Centro del nodo en espacio cubico.
     */
    FVector Center;

    /**
     * Devuelve las 8 esquinas del nodo en coordenadas cubicas.
     *
     * Utilizado principalmente para depuracion y visualizacion.
     */
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

    /**
     * Convierte las esquinas del cubo a coordenadas esfericas.
     *
     * @param Radius Radio utilizado para proyectar sobre la esfera.
     * @return Array con las 8 esquinas proyectadas sobre la esfera.
     */
    TArray<FVector> GetSphereCorners(float Radius) const
    {
        TArray<FVector> Corners;

        // Proyectar las 8 esquinas sobre la esfera
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