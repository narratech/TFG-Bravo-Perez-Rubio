// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CosmicCubeMapCell.generated.h"

/**
 * Represents a cell within a subdivided cubemap.
 *
 * Each cell identifies a specific region of one of the
 * six cube faces used for planetary projection.
 */
USTRUCT()
struct COSMICARCHITECTCOMMON_API FCubeMapCell
{
    GENERATED_BODY()
     
    /**
     * Cube face to which the cell belongs.
     *
     * Valid values: 0-5.
     */
    int32 Face = 0;

    /**
     * X coordinate of the cell within the face.
     */
    int32 X = 0;

    /**
     * Y coordinate of the cell within the face.
     */
    int32 Y = 0;

    /**
     * Subdivision level of the cell.
     *
     * A value of 0 represents the full face.
     */
    int32 Depth = 0;

    /**
     * Comparison operator used for hash structures and TMap.
     */
    bool operator==(const FCubeMapCell& Other) const
    {
        return Face == Other.Face && X == Other.X && Y == Other.Y && Depth == Other.Depth;
    }

    /**
     * Generates a unique hash for the cell.
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
     * Returns a human-readable identifier for the cell.
     */
    FString ToString() const
    {
        return FString::Printf(TEXT("F%d_X%d_Y%d_D%d"), Face, X, Y, Depth);
    }
};

/**
 * Represents the spatial bounds of a cubemap node.
 *
 * Contains information about the corners and center of a region
 * both in cube space and projected onto a sphere.
 */
struct FNodeBounds
{
    /**
     * Minimum corner of the node in cube space.
     */
    FVector MinCorner;

    /**
     * Maximum corner of the node in cube space.
     */
    FVector MaxCorner;

    /**
     * Center of the node in cube space.
     */
    FVector Center;

    /**
     * Returns the 8 corners of the node in cube coordinates.
     *
     * Used primarily for debugging and visualization.
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
     * Converts the cube corners to spherical coordinates.
     *
     * @param Radius Radius used to project onto the sphere.
     * @return Array with the 8 corners projected onto the sphere.
     */
    TArray<FVector> GetSphereCorners(float Radius) const
    {
        TArray<FVector> Corners;

        // Project the 8 corners onto the sphere
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