// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicOctree.h"

namespace
{
    double SphericalTriangleSolidAngle(
        const FVector& A,
        const FVector& B,
        const FVector& C)
    {
        const double Numerator = FMath::Abs(FVector::DotProduct(A, FVector::CrossProduct(B, C)));
        const double Denominator = 1.0
            + FVector::DotProduct(A, B)
            + FVector::DotProduct(B, C)
            + FVector::DotProduct(C, A);
        return 2.0 * FMath::Atan2(Numerator, FMath::Max(Denominator, UE_DOUBLE_SMALL_NUMBER));
    }
}

FCosmicOctree::FCosmicOctree()
{
}

FCosmicOctree::~FCosmicOctree()
{
}

void FCosmicOctree::Initialize(float InPlanetRadius, int32 InMaxDepth)
{
    SphereRadius = InPlanetRadius;
    MaxDepth = InMaxDepth;
    RequiredDepthCache.Reset();
}

FVector FCosmicOctree::UVToCubePoint(int32 Face, float U, float V) const
{
    // U and V are in [0, 1]
    // Convert to coordinates in the cube [-1, 1] 
    float X = U * 2.0f - 1.0f;
    float Y = V * 2.0f - 1.0f;

    switch (Face)
    {
    case 0: return FVector(1.0f, X, Y); // +X
    case 1: return FVector(-1.0f, X, Y); // -X
    case 2: return FVector(X, 1.0f, Y); // +Y
    case 3: return FVector(X, -1.0f, Y); // -Y
    case 4: return FVector(X, Y, 1.0f); // +Z
    case 5: return FVector(X, Y, -1.0f); // -Z
    default: return FVector::ZeroVector;
    }

   
}

FVector FCosmicOctree::CellToCubePoint(const FCubeMapCell& Cell) const
{
    int32 CellsPerSide = 1 << Cell.Depth;
    float CellUVSize = 1.0f / CellsPerSide;

    float U = (Cell.X + 0.5f) * CellUVSize;
    float V = (Cell.Y + 0.5f) * CellUVSize;

    return UVToCubePoint(Cell.Face, U, V);
}

FVector FCosmicOctree::CubePointToDirection(const FVector& CubePoint) const
{
    // Project cube point to the sphere
    return CubePoint.GetSafeNormal();
}

FNodeBounds FCosmicOctree::GetNodeBounds(const FCubeMapCell& Cell) const
{
    FNodeBounds Bounds;

    int32 CellsPerSide = 1 << Cell.Depth;
    float CellUVSize = 1.0f / CellsPerSide;

    float MinU = Cell.X * CellUVSize;
    float MaxU = (Cell.X + 1) * CellUVSize;
    float MinV = Cell.Y * CellUVSize;
    float MaxV = (Cell.Y + 1) * CellUVSize;

    float MinX = MinU * 2.0f - 1.0f;
    float MaxX = MaxU * 2.0f - 1.0f;
    float MinY = MinV * 2.0f - 1.0f;
    float MaxY = MaxV * 2.0f - 1.0f;

    switch (Cell.Face)
    {
    case 0: Bounds.MinCorner = FVector(1.0f, MinX, MinY); Bounds.MaxCorner = FVector(1.0f, MaxX, MaxY); break;
    case 1: Bounds.MinCorner = FVector(-1.0f, MinX, MinY); Bounds.MaxCorner = FVector(-1.0f, MaxX, MaxY); break;
    case 2: Bounds.MinCorner = FVector(MinX, 1.0f, MinY); Bounds.MaxCorner = FVector(MaxX, 1.0f, MaxY); break;
    case 3: Bounds.MinCorner = FVector(MinX, -1.0f, MinY); Bounds.MaxCorner = FVector(MaxX, -1.0f, MaxY); break;
    case 4: Bounds.MinCorner = FVector(MinX, MinY, 1.0f); Bounds.MaxCorner = FVector(MaxX, MaxY, 1.0f); break;
    case 5: Bounds.MinCorner = FVector(MinX, MinY, -1.0f); Bounds.MaxCorner = FVector(MaxX, MaxY, -1.0f); break;
    }

    Bounds.Center = (Bounds.MinCorner + Bounds.MaxCorner) * 0.5f;
    return Bounds;
}

void FCosmicOctree::GetCellCornerDirections(
    const FCubeMapCell& Cell,
    FVector OutCorners[4]) const
{
    const int32 CellsPerSide = 1 << Cell.Depth;
    const float CellUVSize = 1.0f / CellsPerSide;
    const float MinU = Cell.X * CellUVSize;
    const float MaxU = (Cell.X + 1) * CellUVSize;
    const float MinV = Cell.Y * CellUVSize;
    const float MaxV = (Cell.Y + 1) * CellUVSize;

    OutCorners[0] = CubePointToDirection(UVToCubePoint(Cell.Face, MinU, MinV));
    OutCorners[1] = CubePointToDirection(UVToCubePoint(Cell.Face, MaxU, MinV));
    OutCorners[2] = CubePointToDirection(UVToCubePoint(Cell.Face, MaxU, MaxV));
    OutCorners[3] = CubePointToDirection(UVToCubePoint(Cell.Face, MinU, MaxV));
}

FVector FCosmicOctree::GetNodeCenter(const FCubeMapCell& Cell) const
{
    FVector CubePoint = CellToCubePoint(Cell);
    return CubePointToDirection(CubePoint);
}

FVector FCosmicOctree::GetNodeCenterWorld(
    const FCubeMapCell& Cell,
    const FVector& InPlanetCenter,
    float InPlanetRadius) const
{
    FVector Direction = GetNodeCenterDirection(Cell);
    return InPlanetCenter + Direction * InPlanetRadius;
}

float FCosmicOctree::GetNodeAreaKm2(const FCubeMapCell& Cell) const
{
    FVector Corners[4];
    GetCellCornerDirections(Cell, Corners);

    const double SolidAngle =
        SphericalTriangleSolidAngle(Corners[0], Corners[1], Corners[2]) +
        SphericalTriangleSolidAngle(Corners[0], Corners[2], Corners[3]);
    const double AreaCm2 = SolidAngle * SphereRadius * SphereRadius;
    return static_cast<float>(AreaCm2 / 10000000000.0);
}

void FCosmicOctree::GetChildren(const FCubeMapCell& Parent, TArray<FCubeMapCell>& OutChildren) const
{
    if (Parent.Depth >= MaxDepth)
        return;

    int32 NextDepth = Parent.Depth + 1;
    for (int32 dx = 0; dx < 2; dx++)
    {
        for (int32 dy = 0; dy < 2; dy++)
        {
            FCubeMapCell Child;
            Child.Face = Parent.Face;
            Child.X = Parent.X * 2 + dx;
            Child.Y = Parent.Y * 2 + dy;
            Child.Depth = NextDepth;
            OutChildren.Add(Child);
        }
    }
}

FCubeMapCell FCosmicOctree::GetParent(const FCubeMapCell& Child) const
{
    if (Child.Depth == 0)
        return Child; // Root node has no parent

    FCubeMapCell Parent;
    Parent.Face = Child.Face;
    Parent.X = Child.X / 2;
    Parent.Y = Child.Y / 2;
    Parent.Depth = Child.Depth - 1;

    return Parent;
}



float FCosmicOctree::GetCellAngularSize(const FCubeMapCell& Cell) const
{
    FVector Corners[4];
    GetCellCornerDirections(Cell, Corners);

    // Find maximum angular distance between any pair of corners
    float MaxAngularDist = 0.0f;
    for (int32 i = 0; i < 4; i++)
    {
        for (int32 j = i + 1; j < 4; j++)
        {
            float Dot = FVector::DotProduct(Corners[i], Corners[j]);
            Dot = FMath::Clamp(Dot, -1.0f, 1.0f);
            float AngularDist = FMath::Acos(Dot);
            MaxAngularDist = FMath::Max(MaxAngularDist, AngularDist);
        }
    }

    return MaxAngularDist; // Actual maximum angular diameter of the cell in radians
}

float FCosmicOctree::GetCellRadius(const FCubeMapCell& Cell) const
{
    FVector Corners[4];
    GetCellCornerDirections(Cell, Corners);

    // Cell center on the sphere
    FVector Center = GetNodeCenterWorld(Cell, FVector::ZeroVector, SphereRadius);

    // Find maximum distance to center
    float MaxRadius = 0.0f;
    for (const FVector& Corner : Corners)
    {
        const float Dist = FVector::Dist(Center, Corner * SphereRadius);
        MaxRadius = FMath::Max(MaxRadius, Dist);
    }

    return MaxRadius;
}

void FCosmicOctree::TraverseCell(
    const FCubeMapCell& Cell,
    const FVector& PlayerPos,
    const FVector& PlanetCenter,
    float ViewDistanceCm,
    int32 RequiredDepth,
    TArray<FCubeMapCell>& OutNodes) const
{
    // 1. Calculate patch center in the world and its approximate radius
    FVector CellWorldCenter = GetNodeCenterWorld(Cell, PlanetCenter, SphereRadius);
    float CellRadius = GetCellRadius(Cell); // Maximum patch radius in cm

    // 2. Sphere intersection test (view) sphere (cell)
    const double MaximumDistance = ViewDistanceCm + CellRadius;
    if (FVector::DistSquared(PlayerPos, CellWorldCenter) > FMath::Square(MaximumDistance))
    {
        return; // The cell is completely out of view
    }

    // 3. If required depth is reached, add it
    if (Cell.Depth >= RequiredDepth)
    {
        // If it intersects, it is visible (even if its corners are outside)
        OutNodes.Add(Cell);
        return;
    }

    // 4. Subdivide
    for (int32 ChildX = 0; ChildX < 2; ++ChildX)
    {
        for (int32 ChildY = 0; ChildY < 2; ++ChildY)
        {
            FCubeMapCell Child;
            Child.Face = Cell.Face;
            Child.X = Cell.X * 2 + ChildX;
            Child.Y = Cell.Y * 2 + ChildY;
            Child.Depth = Cell.Depth + 1;
            TraverseCell(Child, PlayerPos, PlanetCenter, ViewDistanceCm, RequiredDepth, OutNodes);
        }
    }
}

void FCosmicOctree::GetNodesInRadius(
    const FVector& ViewerLocation,
    const FVector& PlanetCenter,
    float ViewDistanceKm,
    TArray<FCubeMapCell>& OutNodes) const
{
    OutNodes.Reset();

    float ViewDistanceCm = ViewDistanceKm * 100000.0f;
    float ViewAngleRad = ViewDistanceCm / SphereRadius;
    float TargetAngularSize = FMath::Max(ViewAngleRad * 0.5f, 0.08f / SphereRadius);

    const int32 DistanceCacheKey = FMath::RoundToInt(ViewDistanceCm);
    int32 RequiredDepth = MaxDepth;
    if (const int32* CachedDepth = RequiredDepthCache.Find(DistanceCacheKey))
    {
        RequiredDepth = *CachedDepth;
    }
    else
    {
        for (int32 Depth = 0; Depth <= MaxDepth; Depth++)
        {
            FCubeMapCell TestCell;
            TestCell.Face = 0;
            TestCell.Depth = Depth;
            if (Depth == 0)
            {
                TestCell.X = 0;
                TestCell.Y = 0;
            }
            else
            {
                const int32 HalfCells = 1 << (Depth - 1);
                TestCell.X = HalfCells - 1;
                TestCell.Y = HalfCells - 1;
            }

            if (GetCellAngularSize(TestCell) <= TargetAngularSize)
            {
                RequiredDepth = Depth;
                break;
            }
        }
        RequiredDepthCache.Add(DistanceCacheKey, RequiredDepth);
    }

    // Traverse the 6 faces
    for (int32 Face = 0; Face < 6; Face++)
    {
        FCubeMapCell Root;
        Root.Face = Face;
        Root.X = 0;
        Root.Y = 0;
        Root.Depth = 0;

        // Can this face intersect the view sphere?
        FVector FaceCenter = GetNodeCenterWorld(Root, PlanetCenter, SphereRadius);
        float FaceRadius = PI * SphereRadius / 2.0f; // 90 deg in cm
        const double MaximumDistance = ViewDistanceCm + FaceRadius;
        if (FVector::DistSquared(ViewerLocation, FaceCenter) > FMath::Square(MaximumDistance))
        {
            continue;
        }

        TraverseCell(Root, ViewerLocation, PlanetCenter, ViewDistanceCm, RequiredDepth, OutNodes);
    }
}




FCubeMapCell FCosmicOctree::FindCellAtLocation(const FVector& WorldPosition, const FVector& PlanetCenter, int32 TargetDepth) const
{
    // Get direction from center
    FVector Dir = (WorldPosition - PlanetCenter).GetSafeNormal();

    // Find which cube face
    float AbsX = FMath::Abs(Dir.X);
    float AbsY = FMath::Abs(Dir.Y);
    float AbsZ = FMath::Abs(Dir.Z);

    int32 Face;
    float U, V;

    if (AbsX >= AbsY && AbsX >= AbsZ)
    {
        // X Face
        Face = Dir.X > 0 ? 0 : 1;
        U = (Dir.Y / AbsX + 1.0f) * 0.5f;
        V = (Dir.Z / AbsX + 1.0f) * 0.5f;
    }
    else if (AbsY >= AbsZ)
    {
        // Y Face
        Face = Dir.Y > 0 ? 2 : 3;
        U = (Dir.X / AbsY + 1.0f) * 0.5f;
        V = (Dir.Z / AbsY + 1.0f) * 0.5f;
    }
    else
    {
        // Z Face
        Face = Dir.Z > 0 ? 4 : 5;
        U = (Dir.X / AbsZ + 1.0f) * 0.5f;
        V = (Dir.Y / AbsZ + 1.0f) * 0.5f;
    }

    // Clamp to avoid numerical errors
    U = FMath::Clamp(U, 0.0f, 0.999999f);
    V = FMath::Clamp(V, 0.0f, 0.999999f);

    // If depth is not specified, use max
    int32 Depth = TargetDepth >= 0 ? TargetDepth : MaxDepth;
    int32 CellsPerSide = 1 << Depth;

    FCubeMapCell Cell;
    Cell.Face = Face;
    Cell.X = FMath::FloorToInt(U * CellsPerSide);
    Cell.Y = FMath::FloorToInt(V * CellsPerSide);
    Cell.Depth = Depth;

    return Cell;
}

FVector FCosmicOctree::GetNodeCenterDirection(const FCubeMapCell& Cell) const
{
    return CellToCubePoint(Cell).GetSafeNormal();
}

TArray<FVector> FCosmicOctree::GetDebugVertices(const FCubeMapCell& Cell) const
{
    FNodeBounds Bounds = GetNodeBounds(Cell);
    TArray<FVector> SphereCorners = Bounds.GetSphereCorners(SphereRadius);

    // Create lines connecting vertices
    TArray<FVector> Vertices;

    // Indices of cube edges
    int32 Edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // Bottom face
        {4,5}, {5,6}, {6,7}, {7,4}, // Top face
        {0,4}, {1,5}, {2,6}, {3,7}  // Vertical edges
    };

    for (const auto& Edge : Edges)
    {
        Vertices.Add(SphereCorners[Edge[0]]);
        Vertices.Add(SphereCorners[Edge[1]]);
    }

    return Vertices;
}
