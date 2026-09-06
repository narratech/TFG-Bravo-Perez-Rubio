// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "CosmicCubeMapCell.h"

/**
 * Manager responsible for managing spherical spatial subdivision
 * via a CubeMap projection.
 *
 * This class allows:
 * - Getting visible nodes within a distance.
 * - Calculating hierarchical relationships between cells.
 * - Converting positions between cubic and spherical space.
 * - Determining cells from world positions.
 * - Obtaining geometric and debugging information.
 */
class COSMICARCHITECTCOMMON_API FCosmicOctree
{
public:
     
    /**
     * Default constructor.
     */
    FCosmicOctree();

    /**
     * Default destructor.
     */
    ~FCosmicOctree();

    /**
     * Initializes the spherical octree.
     *
     * @param InPlanetRadius Planet radius in centimeters.
     * @param InMaxDepth Maximum allowed depth for subdivision.
     */
    void Initialize(float InPlanetRadius, int32 InMaxDepth = 8);

    /**
     * Gets all visible nodes within a radius on the surface.
     *
     * @param ViewerLocation Viewer position in the world.
     * @param PlanetCenter Planet center.
     * @param ViewDistanceKm View distance in kilometers.
     * @param OutNodes Output array with visible cells.
     */
    void GetNodesInRadius(
        const FVector& ViewerLocation,
        const FVector& PlanetCenter,
        float ViewDistanceKm,
        TArray<FCubeMapCell>& OutNodes) const;

    /**
     * Gets the geometric bounds of a cell.
     *
     * @param Cell Target cell.
     * @return Cell bounds information.
     */
    FNodeBounds GetNodeBounds(const FCubeMapCell& Cell) const;

    /**
     * Gets the normalized direction of a cell's center.
     *
     * @param Cell Target cell.
     * @return Normalized direction in spherical space.
     */
    FVector GetNodeCenter(const FCubeMapCell& Cell) const;

    /**
     * Gets the normalized direction of a cell's center.
     *
     * @param Cell Target cell.
     * @return Normalized direction vector.
     */
    FVector GetNodeCenterDirection(const FCubeMapCell& Cell) const;

    /**
     * Gets the world position of a cell's center.
     *
     * @param Cell Target cell.
     * @param InPlanetCenter Planet center.
     * @param InPlanetRadius Planet radius.
     * @return World position of the center.
     */
    FVector GetNodeCenterWorld(
        const FCubeMapCell& Cell,
        const FVector& InPlanetCenter,
        float InPlanetRadius) const;

    /**
     * Calculates the approximate area of a cell in square kilometers.
     *
     * @param Cell Target cell.
     * @return Approximate area in km².
     */
    float GetNodeAreaKm2(const FCubeMapCell& Cell) const;

    /**
     * Gets the four child nodes of a cell.
     *
     * @param Parent Parent cell.
     * @param OutChildren Output array with child cells.
     */
    void GetChildren(const FCubeMapCell& Parent, TArray<FCubeMapCell>& OutChildren) const;

    /**
     * Gets the parent cell of a child cell.
     *
     * @param Child Child cell.
     * @return Corresponding parent cell.
     */
    FCubeMapCell GetParent(const FCubeMapCell& Child) const;

    /**
     * Finds the cell containing a world position.
     *
     * @param WorldPosition Position in the world.
     * @param PlanetCenter Planet center.
     * @param TargetDepth Desired search depth.
     * @return Corresponding cell for the indicated position.
     */
    FCubeMapCell FindCellAtLocation(
        const FVector& WorldPosition,
        const FVector& PlanetCenter,
        int32 TargetDepth = -1) const;

    /**
     * Gets debug vertices to visually represent the cell.
     *
     * @param Cell Target cell.
     * @return List of debug vertices.
     */
    TArray<FVector> GetDebugVertices(const FCubeMapCell& Cell) const;

private:

    /**
     * Sphere radius in centimeters.
     */
    float SphereRadius;

    /**
     * Maximum allowed depth for the octree.
     */
    int32 MaxDepth;

    /**
     * Converts a cell to a center point on the projected cube.
     *
     * @param Cell Target cell.
     * @return Corresponding point on the cube.
     */
    FVector CellToCubePoint(const FCubeMapCell& Cell) const;

    /**
     * Converts UV coordinates of a cube face to a Cartesian point.
     *
     * @param Face Cube face.
     * @param U Normalized U coordinate.
     * @param V Normalized V coordinate.
     * @return Point in cubic space.
     */
    FVector UVToCubePoint(int32 Face, float U, float V) const;

    /**
     * Projects a cube point onto the sphere.
     *
     * @param CubePoint Point on the cube.
     * @return Normalized direction on the sphere.
     */
    FVector CubePointToDirection(const FVector& CubePoint) const;

    /**
     * Calculates the approximate maximum radius of a cell on the sphere.
     *
     * @param Cell Target cell.
     * @return Maximum radius in centimeters.
     */
    float GetCellRadius(const FCubeMapCell& Cell) const;

    /**
     * Gets the angular size of a cell in radians.
     *
     * @param Cell Target cell.
     * @return Angular size in radians.
     */
    float GetCellAngularSize(const FCubeMapCell& Cell) const;

    /** Gets the four unique corners of the cell already normalized. */
    void GetCellCornerDirections(const FCubeMapCell& Cell, FVector OutCorners[4]) const;

    /**
     * Recursively traverses a cell and its visible subdivisions.
     *
     * @param Cell Current cell.
     * @param PlayerPos Viewer position.
     * @param PlanetCenter Planet center.
     * @param ViewDistanceCm Maximum view distance in centimeters.
     * @param RequiredDepth Required depth.
     * @param OutNodes Output array with visible nodes.
     */
    void TraverseCell(
        const FCubeMapCell& Cell,
        const FVector& PlayerPos,
        const FVector& PlanetCenter,
        float ViewDistanceCm,
        int32 RequiredDepth,
        TArray<FCubeMapCell>& OutNodes) const;

    /** Depths already calculated for distances used by layers. */
    mutable TMap<int32, int32> RequiredDepthCache;
};
