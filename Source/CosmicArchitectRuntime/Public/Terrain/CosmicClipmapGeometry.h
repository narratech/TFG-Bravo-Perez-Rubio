// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/**
 * Stateless geometry utilities for planetary clipmap and ocean generation.
 *
 * Provides shared routines for:
 * - Planar-to-spherical grid projection with exact radial normal and tangent derivation.
 * - Clipmap level triangle index generation with 2:1 edge stitching and ring cutouts.
 *
 * All mathematical functions are FORCEINLINE to ensure zero overhead and identical assembly.
 */
struct COSMICARCHITECTRUNTIME_API FCosmicClipmapGeometry
{
    /**
     * Projects a 2D coordinate on a planar tangent grid onto a sphere of the given radius.
     * Computes the base position on the spherical surface and the outward radial normal.
     *
     * @param WorldX Planar X distance from the grid center in centimeters.
     * @param WorldY Planar Y distance from the grid center in centimeters.
     * @param Radius Planet or ocean radius in centimeters.
     * @param OutPosition Resulting local spherical position.
     * @param OutNormal Resulting outward normal vector.
     */
    FORCEINLINE static void ProjectPlanarGridPointToSphere(
        double WorldX,
        double WorldY,
        double Radius,
        FVector& OutPosition,
        FVector& OutNormal)
    {
        const FVector SphereCenter(0.0, 0.0, -Radius);
        const double Distance2D = FMath::Sqrt(WorldX * WorldX + WorldY * WorldY);

        if (Distance2D <= Radius && Distance2D > 0.001)
        {
            const double ZOffset = FMath::Sqrt(Radius * Radius - Distance2D * Distance2D);
            OutPosition = FVector(WorldX, WorldY, -Radius + ZOffset);
        }
        else if (Distance2D <= 0.001)
        {
            OutPosition = FVector::ZeroVector;
        }
        else
        {
            const double Scale = Radius / Distance2D;
            OutPosition = FVector(WorldX * Scale, WorldY * Scale, -Radius);
        }

        OutNormal = (OutPosition - SphereCenter);
        if (OutNormal.SizeSquared() > 0.001)
        {
            OutNormal.Normalize();
        }
        else
        {
            OutNormal = FVector::UpVector;
        }
    }

    /**
     * Projects a 2D coordinate on a planar tangent grid onto a sphere of the given radius.
     * Computes the base position, outward radial normal, and tangent direction vector.
     *
     * @param WorldX Planar X distance from the grid center in centimeters.
     * @param WorldY Planar Y distance from the grid center in centimeters.
     * @param Radius Planet or ocean radius in centimeters.
     * @param OutPosition Resulting local spherical position.
     * @param OutNormal Resulting outward normal vector.
     * @param OutTangent Resulting tangent vector orthogonal to the normal.
     */
    FORCEINLINE static void ProjectPlanarGridPointToSphere(
        double WorldX,
        double WorldY,
        double Radius,
        FVector& OutPosition,
        FVector& OutNormal,
        FVector& OutTangent)
    {
        ProjectPlanarGridPointToSphere(WorldX, WorldY, Radius, OutPosition, OutNormal);

        OutTangent = FVector(-OutNormal.Y, OutNormal.X, 0.0);
        if (OutTangent.SizeSquared() > 0.001)
        {
            OutTangent.Normalize();
        }
        else
        {
            OutTangent = FVector(1.0, 0.0, 0.0);
        }
    }

    /**
     * Generates triangle index buffers for a single clipmap level, including:
     * - Optional ring hole culling (when bIsRing is true)
     * - 2:1 edge stitching to prevent T-junction cracks with coarser neighboring levels
     * - Standard 2-triangle quad subdivision for interior cells
     *
     * @param Resolution Grid resolution along one edge (must be even, e.g. 64, 128)
     * @param bIsRing Whether this level has a coarser inner level hole
     * @param VertexBaseOffset Base index offset in the combined vertex buffer (0 for single-level meshes)
     * @param OutTriangles Triangle index buffer to append to
     */
    static void GenerateClipmapLevelTriangles(
        int32 Resolution,
        bool bIsRing,
        int32 VertexBaseOffset,
        TArray<int32>& OutTriangles)
    {
        const int32 VertRes = Resolution + 1;
        const int32 VertsPerLevel = VertRes * VertRes;
        const int32 LevelMaxVertex = VertexBaseOffset + VertsPerLevel;
        const int32 HalfRes = Resolution / 2;

        for (int32 y = 0; y < Resolution; ++y)
        {
            for (int32 x = 0; x < Resolution; ++x)
            {
                const int32 i0 = VertexBaseOffset + y * VertRes + x;
                const int32 i1 = i0 + 1;
                const int32 i2 = i0 + VertRes;
                const int32 i3 = i2 + 1;

                if (bIsRing)
                {
                    const bool bInsideInner =
                        x > HalfRes / 2 &&
                        x < Resolution - HalfRes / 2 &&
                        y > HalfRes / 2 &&
                        y < Resolution - HalfRes / 2;

                    if (bInsideInner)
                    {
                        continue;
                    }
                }

                if (i0 >= LevelMaxVertex || i1 >= LevelMaxVertex ||
                    i2 >= LevelMaxVertex || i3 >= LevelMaxVertex)
                {
                    continue;
                }

                const bool bBorder =
                    (x == 0) ||
                    (x == Resolution - 1) ||
                    (y == 0) ||
                    (y == Resolution - 1);

                // LEVEL BORDER STITCHING (2:1 quad transition)
                if (bBorder)
                {
                    // Horizontal borders
                    if ((y == 0 || y == Resolution - 1) && (x % 2 == 0) && x < Resolution - 1)
                    {
                        const int32 i4 = i1 + 1;
                        const int32 i5 = i3 + 1;

                        if (i4 < LevelMaxVertex && i5 < LevelMaxVertex)
                        {
                            if (y == Resolution - 1) // Bottom border
                            {
                                if (x != Resolution - 2)
                                {
                                    OutTriangles.Add(i1);
                                    OutTriangles.Add(i5);
                                    OutTriangles.Add(i4);
                                }

                                if (x != 0)
                                {
                                    OutTriangles.Add(i1);
                                    OutTriangles.Add(i0);
                                    OutTriangles.Add(i2);
                                }

                                OutTriangles.Add(i2);
                                OutTriangles.Add(i5);
                                OutTriangles.Add(i1);
                            }
                            else // Top border
                            {
                                if (x != 0)
                                {
                                    OutTriangles.Add(i0);
                                    OutTriangles.Add(i2);
                                    OutTriangles.Add(i3);
                                }

                                if (x != Resolution - 2)
                                {
                                    OutTriangles.Add(i3);
                                    OutTriangles.Add(i5);
                                    OutTriangles.Add(i4);
                                }

                                OutTriangles.Add(i0);
                                OutTriangles.Add(i3);
                                OutTriangles.Add(i4);
                            }
                        }
                    }
                    // Vertical borders
                    else if ((x == 0 || x == Resolution - 1) && (y % 2 == 0) && y < Resolution - 1)
                    {
                        const int32 i4 = i2 + VertRes;
                        const int32 i5 = i3 + VertRes;

                        if (i4 < LevelMaxVertex && i5 < LevelMaxVertex)
                        {
                            if (x == Resolution - 1) // Right border
                            {
                                OutTriangles.Add(i1);
                                OutTriangles.Add(i2);
                                OutTriangles.Add(i5);

                                if (y != 0)
                                {
                                    OutTriangles.Add(i2);
                                    OutTriangles.Add(i1);
                                    OutTriangles.Add(i0);
                                }

                                if (y != Resolution - 2)
                                {
                                    OutTriangles.Add(i2);
                                    OutTriangles.Add(i4);
                                    OutTriangles.Add(i5);
                                }
                            }
                            else // Left border
                            {
                                if (y != 0)
                                {
                                    OutTriangles.Add(i0);
                                    OutTriangles.Add(i3);
                                    OutTriangles.Add(i1);
                                }

                                if (y != Resolution - 2)
                                {
                                    OutTriangles.Add(i3);
                                    OutTriangles.Add(i4);
                                    OutTriangles.Add(i5);
                                }

                                OutTriangles.Add(i0);
                                OutTriangles.Add(i4);
                                OutTriangles.Add(i3);
                            }
                        }
                    }
                }
                else
                {
                    // NORMAL INTERIOR (Standard winding)
                    OutTriangles.Add(i0);
                    OutTriangles.Add(i2);
                    OutTriangles.Add(i1);

                    OutTriangles.Add(i1);
                    OutTriangles.Add(i2);
                    OutTriangles.Add(i3);
                }
            }
        }
    }
};
