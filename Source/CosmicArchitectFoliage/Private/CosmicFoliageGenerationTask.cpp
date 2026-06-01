// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicFoliageGenerationTask.h"
#include "ICosmicNoiseStrategy.h"

// TAREA ASINCRONA 
void FFoliageGenerationTask::DoWork()
{
    if (!Collection) return;

    //Crear siempre la misma semilla para la misma celda
    uint32 Hash = 2166136261u;

    Hash = (Hash ^ Cell.Face) * 16777619u;
    Hash = (Hash ^ Cell.X) * 16777619u;
    Hash = (Hash ^ Cell.Y) * 16777619u;
    Hash = (Hash ^ Cell.Depth) * 16777619u;
    Hash = (Hash ^ static_cast<uint32>(Layer)) * 16777619u;

    FRandomStream LocalRandom(Hash);

    // Generar puntos de semilla en la esfera
    GenerateSeedPoints(LocalRandom);
    
    // Evaluar condiciones ambientales para cada punto
    EvaluateEnvironmentalConditions(LocalRandom);

    // Seleccionar y crear instancias basadas en las condiciones
    CreateFoliageInstances(LocalRandom);
}

void FFoliageGenerationTask::GenerateSeedPoints(FRandomStream& Random)
{
    SeedPoints.Empty();

    int32 NumSeeds = 0;
    for (const FCosmicFoliageCollectionEntry& Entry : Collection->FoliageEntries)
        for (const FCosmicFoliageMesh& Mesh : Entry.Foliage)
            if (Mesh.FoliageLayer == Layer)
                NumSeeds += FMath::Max(1, FMath::RoundToInt(CellAreaKm2 * Mesh.InstancesPerKm2));

    if (NumSeeds == 0)
        return;

    SeedPoints.Reserve(NumSeeds);

    // Datos de la celda para mapeo UV
    int32 CellsPerSide = 1 << Cell.Depth;
    float CellSizeUV = 1.0f / CellsPerSide;
    float MinU = Cell.X * CellSizeUV;
    float MaxU = (Cell.X + 1) * CellSizeUV;
    float MinV = Cell.Y * CellSizeUV;
    float MaxV = (Cell.Y + 1) * CellSizeUV;

    for (int32 i = 0; i < NumSeeds; i++)
    {
        float U = Random.FRandRange(MinU, MaxU);
        float V = Random.FRandRange(MinV, MaxV);
        float X = U * 2.0f - 1.0f;
        float Y = V * 2.0f - 1.0f;

        FVector CubePoint;
        switch (Cell.Face)
        {
        case 0: CubePoint = FVector(1.0f, X, Y); break;
        case 1: CubePoint = FVector(-1.0f, X, Y); break;
        case 2: CubePoint = FVector(X, 1.0f, Y); break;
        case 3: CubePoint = FVector(X, -1.0f, Y); break;
        case 4: CubePoint = FVector(X, Y, 1.0f); break;
        case 5: CubePoint = FVector(X, Y, -1.0f); break;
        default: CubePoint = FVector::ZeroVector; break;
        }

        FSeedPoint Point;
        Point.Direction = CubePoint.GetSafeNormal();
        SeedPoints.Add(Point);
    }
}

void FFoliageGenerationTask::EvaluateEnvironmentalConditions(FRandomStream& Random)
{
    for (FSeedPoint& Point : SeedPoints)
    {
        FLinearColor BiomeData;
        NoiseGenerationStrategy->EvaluatePoint(Point.Direction, Point.Height, BiomeData);

        Point.Temperature = BiomeData.G;
        Point.Humidity = BiomeData.B;
        Point.WorldPosition = Point.Direction * (PlanetRadius + Point.Height);

        // Una sola llamada para pendiente y normal
        CalculateSlopeAndNormal(Point.Direction, Point.Slope, Point.CachedNormal);
    }
}

void FFoliageGenerationTask::CreateFoliageInstances(FRandomStream& Random)
{
    ResultInstances.Empty();

    // Recopilar todos los meshes válidos de esta capa con su cuota de instancias
    struct FMeshAllocation
    {
        const FCosmicFoliageCollectionEntry* Entry;
        const FCosmicFoliageMesh* Mesh;
        int32 TargetCount;   // instancias que le corresponden
        int32 Assigned;      // cuántas ya se asignaron
    };

    TArray<FMeshAllocation> Allocations;
    int32 TotalTarget = 0;

    for (const FCosmicFoliageCollectionEntry& Entry : Collection->FoliageEntries)
    {
        for (const FCosmicFoliageMesh& Mesh : Entry.Foliage)
        {
            if (Mesh.FoliageLayer != Layer || !Mesh.Mesh)
                continue;

            int32 Target = FMath::Max(1, FMath::RoundToInt(CellAreaKm2 * Mesh.InstancesPerKm2));
            Allocations.Add({ &Entry, &Mesh, Target, 0 });
            TotalTarget += Target;
        }
    }

    if (Allocations.Num() == 0 || TotalTarget == 0)
        return;

    // Barajar los seed points para evitar sesgos espaciales en la asignación
    TArray<int32> PointIndices;
    PointIndices.Reserve(SeedPoints.Num());
    for (int32 i = 0; i < SeedPoints.Num(); i++)
        PointIndices.Add(i);

    // Fisher-Yates shuffle
    for (int32 i = PointIndices.Num() - 1; i > 0; i--)
    {
        int32 j = Random.RandRange(0, i);
        PointIndices.Swap(i, j);
    }

    // Distribuir puntos entre allocations proporcionalmente sin desperdiciar ninguno.
    // Iteramos en orden aleatorio y asignamos cada punto al mesh que más instancias
    // le faltan en proporción a su cuota (largest remainder / round-robin ponderado).
    ResultInstances.Reserve(FMath::Min(SeedPoints.Num(), TotalTarget));

    int32 TotalAssigned = 0;

    for (int32 Idx : PointIndices)
    {
        if (TotalAssigned >= TotalTarget)
            break;

        const FSeedPoint& Point = SeedPoints[Idx];

        // Buscar la allocation con mayor "deuda" proporcional restante
        // deuda = (TargetCount - Assigned) — elegimos la de mayor cuota pendiente
        int32 BestAlloc = -1;
        int32 BestRemaining = 0;

        for (int32 a = 0; a < Allocations.Num(); a++)
        {
            const FMeshAllocation& Alloc = Allocations[a];
            int32 Remaining = Alloc.TargetCount - Alloc.Assigned;
            if (Remaining > BestRemaining)
            {
                // Verificar que las condiciones ambientales del punto encajan con esta entry
                const FCosmicFoliageCollectionEntry* E = Alloc.Entry;
                bool bValid =
                    Point.Slope >= E->SlopeMin && Point.Slope <= E->SlopeMax &&
                    Point.Temperature >= E->TemperatureMin && Point.Temperature <= E->TemperatureMax &&
                    Point.Humidity >= E->HumidityMin && Point.Humidity <= E->HumidityMax &&
                    Point.Height >= E->ElevationMinKm * 100000.f && Point.Height <= E->ElevationMaxKm * 100000.f;

                if (bValid)
                {
                    BestRemaining = Remaining;
                    BestAlloc = a;
                }
            }
        }

        if (BestAlloc == -1)
            continue; // ningún mesh acepta las condiciones de este punto

        FMeshAllocation& Alloc = Allocations[BestAlloc];
        const FCosmicFoliageMesh* SelectedMesh = Alloc.Mesh;

        // Calcular transformación 
        float Yaw = Random.FRandRange(SelectedMesh->RandomRotationMin, SelectedMesh->RandomRotationMax);
        float Scale = Random.FRandRange(SelectedMesh->ScaleMin, SelectedMesh->ScaleMax);

        FQuat   Rotation;
        FVector OffsetNormal = FVector::UpVector;

        if (SelectedMesh->bAlignToGround)
        {
            OffsetNormal = Point.CachedNormal;  
            FQuat AlignRotation = FQuat::FindBetweenNormals(FVector::UpVector, Point.CachedNormal);
            FQuat RandomYawRotation = FQuat(Point.CachedNormal, FMath::DegreesToRadians(Yaw));
            Rotation = RandomYawRotation * AlignRotation;
        }
        else if (SelectedMesh->bAlignToPlanetNormal)
        {
            OffsetNormal = Point.Direction;
            FQuat AlignRotation = FQuat::FindBetweenNormals(FVector::UpVector, Point.Direction);
            FQuat RandomYawRotation = FQuat(Point.Direction, FMath::DegreesToRadians(Yaw));
            Rotation = RandomYawRotation * AlignRotation;
        }
        else
        {
            Rotation = FRotator(0, Yaw, 0).Quaternion();
        }

        FTransform Transform;
        FVector FinalPosition = Point.WorldPosition + OffsetNormal * SelectedMesh->HeightOffset;
        Transform.SetLocation(FinalPosition);
        Transform.SetRotation(Rotation);
        Transform.SetScale3D(FVector(Scale));

        FCosmicFoliageInstance Instance;
        Instance.MeshDef = SelectedMesh;
        Instance.Transform = Transform;
        ResultInstances.Add(Instance);

        Alloc.Assigned++;
        TotalAssigned++;
    }
}

void FFoliageGenerationTask::CalculateSlopeAndNormal(
    const FVector& Direction,
    float& OutSlope,
    FVector& OutNormal)
{
    const float SampleDistance = 500.0f;

    FVector Tangent1, Tangent2;
    Direction.FindBestAxisVectors(Tangent1, Tangent2);

    FVector SampleDirs[4] =
    {
        (Direction + Tangent1 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction - Tangent1 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction + Tangent2 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction - Tangent2 * (SampleDistance / PlanetRadius)).GetSafeNormal()
    };

    float CenterHeight = 0.0f;
    FLinearColor Dummy;
    NoiseGenerationStrategy->EvaluatePoint(Direction, CenterHeight, Dummy); 

    FVector Positions[4];
    float   MaxSlope = 0.0f;

    for (int32 i = 0; i < 4; i++)
    {
        float SampleHeight = 0.0f;
        NoiseGenerationStrategy->EvaluatePoint(SampleDirs[i], SampleHeight, Dummy);

        Positions[i] = PlanetCenter + SampleDirs[i] * (PlanetRadius + SampleHeight);

        float HeightDiff = FMath::Abs(SampleHeight - CenterHeight);
        float SlopeAngle = FMath::Atan(HeightDiff / SampleDistance) * (180.0f / PI);
        MaxSlope = FMath::Max(MaxSlope, SlopeAngle);
    }

    // Normal
    FVector V1 = Positions[0] - Positions[1];
    FVector V2 = Positions[2] - Positions[3];
    FVector Normal = FVector::CrossProduct(V1, V2).GetSafeNormal();

    if (FVector::DotProduct(Normal, Direction) < 0)
        Normal = -Normal;

    OutSlope = MaxSlope;
    OutNormal = Normal;
}
