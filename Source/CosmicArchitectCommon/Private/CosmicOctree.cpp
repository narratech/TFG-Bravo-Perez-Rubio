// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicOctree.h"

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
}

FVector FCosmicOctree::UVToCubePoint(int32 Face, float U, float V) const
{
    // U y V estan en [0, 1]
    // Convertir a coordenadas en el cubo [-1, 1]
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
    // Proyectar el punto del cubo a la esfera
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
    // Area aproximada: la celda en el cubo tiene area (CellSize^2)
    // Proyectada a la esfera, el area se escala por el radio al cuadrado
    int32 CellsPerSide = 1 << Cell.Depth;
    float CellSizeCube = 2.0f / CellsPerSide;
    float AreaOnCube = CellSizeCube * CellSizeCube;

    // Area en la esfera = area en cubo * radio^2
    float AreaCm2 = AreaOnCube * SphereRadius * SphereRadius;

    // Convertir a km2 (1 km2 = 10,000,000,000 cm2)
    return AreaCm2 / 10000000000.0f;
}

void FCosmicOctree::GetChildren(const FCubeMapCell& Parent, TArray<FCubeMapCell>& OutChildren) const
{
    if (Parent.Depth >= MaxDepth)
        return;

    int32 NextDepth = Parent.Depth + 1;
    int32 ChildSize = 1 << (NextDepth - Parent.Depth); // 2^(nextDepth - currentDepth)

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
    // Obtener los límites reales de la celda en el cubo
    FNodeBounds Bounds = GetNodeBounds(Cell);

    // Calcular los 4 vectores de las esquinas en la esfera
    TArray<FVector> Corners = Bounds.GetSphereCorners(1.0f); // Radio unitario para ángulos

    // Encontrar la distancia angular máxima entre cualquier par de esquinas
    float MaxAngularDist = 0.0f;
    for (int32 i = 0; i < Corners.Num(); i++)
    {
        for (int32 j = i + 1; j < Corners.Num(); j++)
        {
            float Dot = FVector::DotProduct(Corners[i], Corners[j]);
            Dot = FMath::Clamp(Dot, -1.0f, 1.0f);
            float AngularDist = FMath::Acos(Dot);
            MaxAngularDist = FMath::Max(MaxAngularDist, AngularDist);
        }
    }

    return MaxAngularDist; // Diámetro angular máximo real de la celda en radianes
}

float FCosmicOctree::GetCellRadius(const FCubeMapCell& Cell) const
{
    // Método mejorado: calcular el radio real del parche en la esfera
    FNodeBounds Bounds = GetNodeBounds(Cell);
    TArray<FVector> SphereCorners = Bounds.GetSphereCorners(SphereRadius);

    // Centro de la celda en la esfera
    FVector Center = GetNodeCenterWorld(Cell, FVector::ZeroVector, SphereRadius);

    // Encontrar la distancia máxima al centro
    float MaxRadius = 0.0f;
    for (const FVector& Corner : SphereCorners)
    {
        float Dist = FVector::Dist(Center, Corner);
        MaxRadius = FMath::Max(MaxRadius, Dist);
    }

    return MaxRadius;
}

//int32 FCosmicOctree::GetDesiredDepth(float DistanceKm) const
//{
//    // Convertir distancia a angulo
//    float DistanceCm = DistanceKm * 100000.0f;
//    float AngleRad = DistanceCm / SphereRadius;
//
//    // Queremos celdas cuyo tamano angular sea aproximadamente la mitad del angulo de visión
//    float TargetAngularSize = AngleRad * 0.5f;
//
//    // Encontrar el depth que da un tamano angular cercano al objetivo
//    for (int32 Depth = 0; Depth <= MaxDepth; Depth++)
//    {
//        FCubeMapCell TestCell;
//        TestCell.Face = 0;
//        TestCell.X = 0;
//        TestCell.Y = 0;
//        TestCell.Depth = Depth;
//
//        float CellAngle = GetCellAngularSize(TestCell);
//        if (CellAngle <= TargetAngularSize)
//        {
//            return FMath::Max(0, Depth - 1);
//        }
//    }
//
//    return MaxDepth;
//}

//int32 FCosmicOctree::GetMaxDepthFromDistance(float DistanceToSurfaceCm) const
//{
//    // A mayor distancia a la superficie, menor profundidad
//    if (DistanceToSurfaceCm > 1000000.0f) return 0;      // > 10km -> solo raíz
//    if (DistanceToSurfaceCm > 100000.0f) return 1;       // > 1km
//    if (DistanceToSurfaceCm > 10000.0f) return 2;        // > 100m
//    if (DistanceToSurfaceCm > 1000.0f) return 3;         // > 10m
//    if (DistanceToSurfaceCm > 100.0f) return 4;          // > 1m
//    return 5;                                             // muy cerca
//}

void FCosmicOctree::TraverseCell(
    const FCubeMapCell& Cell,
    const FVector& PlayerPos,
    const FVector& PlanetCenter,
    float ViewDistanceCm,
    float ViewAngleRad,
    TArray<FCubeMapCell>& OutNodes) const
{
    // Calcular los límites del parche esférico de esta celda
    FNodeBounds Bounds = GetNodeBounds(Cell);

    // Obtener las esquinas de la celda proyectadas a la esfera
    TArray<FVector> SphereCorners = Bounds.GetSphereCorners(SphereRadius);

    // Verificar si algún punto de la celda está dentro del radio de visión 3D
    bool bAnyPointInRange = false;

    for (const FVector& Corner : SphereCorners)
    {
        float DistToPoint = FVector::Dist(Corner, PlayerPos);
        if (DistToPoint <= ViewDistanceCm)
        {
            bAnyPointInRange = true;
            break;
        }
    }

    // Si ningún punto está en rango, verificar si la celda podría intersecar
    if (!bAnyPointInRange)
    {
        FVector CellCenter = GetNodeCenterWorld(Cell, PlanetCenter, SphereRadius);
        float DistToCellCenter = FVector::Dist(PlayerPos, CellCenter);
        float CellRadius = GetCellRadius(Cell);

        // Si la distancia es mayor que (radio_vision + radio_celda), no hay intersección posible
        if (DistToCellCenter > ViewDistanceCm + CellRadius)
        {
            return;
        }

        // Si ya es un nodo hoja y está cerca, incluirlo por seguridad
        if (Cell.Depth >= MaxDepth)
        {
            OutNodes.Add(Cell);
            return;
        }
    }

    // Calcular el tamaño angular real de la celda
    float CellAngularSize = GetCellAngularSize(Cell);

    // CONDICIÓN DE SUBDIVISIÓN: 
    // Subdividir si la celda es más grande que la mitad del ángulo de visión
    if (Cell.Depth < MaxDepth && CellAngularSize > ViewAngleRad * 0.5f)
    {
        // Subdividir la celda
        TArray<FCubeMapCell> Children;
        GetChildren(Cell, Children);

        for (const FCubeMapCell& Child : Children)
        {
            TraverseCell(Child, PlayerPos, PlanetCenter, ViewDistanceCm, ViewAngleRad, OutNodes);
        }
        return;
    }

    // Si llegamos aquí, la celda es suficientemente pequeña o es hoja
    OutNodes.Add(Cell);
}

void FCosmicOctree::GetNodesInRadius(
    const FVector& ViewerLocation,
    const FVector& PlanetCenter,
    float ViewDistanceKm,
    TArray<FCubeMapCell>& OutNodes) const
{
    OutNodes.Reset();

    float ViewDistanceCm = ViewDistanceKm * 100000.0f;

    // Ángulo equivalente a la distancia de visión sobre la esfera
    float ViewAngleRad = ViewDistanceCm / SphereRadius;

    // Para cada cara, verificar rápidamente si puede ser visible
    for (int32 Face = 0; Face < 6; Face++)
    {
        FCubeMapCell Root;
        Root.Face = Face;
        Root.X = 0;
        Root.Y = 0;
        Root.Depth = 0;

        // Verificar si esta cara podría ser visible
        FVector FaceCenter = GetNodeCenterWorld(Root, PlanetCenter, SphereRadius);
        float DistToFaceCenter = FVector::Dist(ViewerLocation, FaceCenter);
        float FaceRadius = PI * SphereRadius / 2.0f; // 90 grados en cm (cara completa)

        if (DistToFaceCenter > ViewDistanceCm + FaceRadius)
        {
            continue; // Esta cara está definitivamente fuera de rango
        }

        TraverseCell(Root, ViewerLocation, PlanetCenter, ViewDistanceCm, ViewAngleRad, OutNodes);
    }
}




FCubeMapCell FCosmicOctree::FindCellAtLocation(const FVector& WorldPosition, const FVector& PlanetCenter, int32 TargetDepth) const
{
    // Obtener direccion desde el centro
    FVector Dir = (WorldPosition - PlanetCenter).GetSafeNormal();

    // Encontrar que cara del cubo
    float AbsX = FMath::Abs(Dir.X);
    float AbsY = FMath::Abs(Dir.Y);
    float AbsZ = FMath::Abs(Dir.Z);

    int32 Face;
    float U, V;

    if (AbsX >= AbsY && AbsX >= AbsZ)
    {
        // Cara X
        Face = Dir.X > 0 ? 0 : 1;
        U = (Dir.Y / AbsX + 1.0f) * 0.5f;
        V = (Dir.Z / AbsX + 1.0f) * 0.5f;
    }
    else if (AbsY >= AbsZ)
    {
        // Cara Y
        Face = Dir.Y > 0 ? 2 : 3;
        U = (Dir.X / AbsY + 1.0f) * 0.5f;
        V = (Dir.Z / AbsY + 1.0f) * 0.5f;
    }
    else
    {
        // Cara Z
        Face = Dir.Z > 0 ? 4 : 5;
        U = (Dir.X / AbsZ + 1.0f) * 0.5f;
        V = (Dir.Y / AbsZ + 1.0f) * 0.5f;
    }

    // Clamp para evitar errores numericos
    U = FMath::Clamp(U, 0.0f, 0.999999f);
    V = FMath::Clamp(V, 0.0f, 0.999999f);

    // Si no se especifica depth, usar el maximo
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

    // Crear lineas conectando los vértices
    TArray<FVector> Vertices;

    // Indices de las aristas del cubo
    int32 Edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // Cara inferior
        {4,5}, {5,6}, {6,7}, {7,4}, // Cara superior
        {0,4}, {1,5}, {2,6}, {3,7}  // Aristas verticales
    };

    for (const auto& Edge : Edges)
    {
        Vertices.Add(SphereCorners[Edge[0]]);
        Vertices.Add(SphereCorners[Edge[1]]);
    }

    return Vertices;
}