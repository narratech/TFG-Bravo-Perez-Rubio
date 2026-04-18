// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicFoliageGenerationTask.h"

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
    FCosmicNoiseEvaluator Evaluator(NoiseSettings);

    // Generar puntos de semilla en la esfera
    GenerateSeedPoints(LocalRandom);
    
    // Evaluar condiciones ambientales para cada punto
    EvaluateEnvironmentalConditions(LocalRandom, Evaluator);

    // Seleccionar y crear instancias basadas en las condiciones
    CreateFoliageInstances(LocalRandom, Evaluator);
}

void FFoliageGenerationTask::GenerateSeedPoints(FRandomStream& Random)
{
    SeedPoints.Empty();

    // Filtrar meshes de esta capa y calcular densidad maxima
    int32 MaxInstancesPerKm2 = 0;
    TArray<const FCosmicFoliageMesh*> LayerMeshes;

    for (const FCosmicFoliageCollectionEntry& Entry : Collection->FoliageEntries)
    {
        for (const FCosmicFoliageMesh& Mesh : Entry.Foliage)
        {
            if (Mesh.FoliageLayer == Layer)
            {
                LayerMeshes.Add(&Mesh);
                MaxInstancesPerKm2 = FMath::Max(MaxInstancesPerKm2, Mesh.InstancesPerKm2);
            }
        }
    }

    if (LayerMeshes.Num() == 0 || MaxInstancesPerKm2 == 0)
        return;

    int32 NumSeeds = FMath::Max(1, FMath::RoundToInt(CellAreaKm2 * MaxInstancesPerKm2));
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

void FFoliageGenerationTask::EvaluateEnvironmentalConditions(FRandomStream& Random, FCosmicNoiseEvaluator& NoiseEvaluator)
{
    uint32 i = 0;

    for (FSeedPoint& Point : SeedPoints)
    {
        FLinearColor BiomeData;
        NoiseEvaluator.EvaluatePoint(Point.Direction, Point.Height, BiomeData);

        Point.Temperature = BiomeData.G;
        Point.Humidity = BiomeData.B;
        Point.Slope = CalculateSlope(Point.Direction, i, NoiseEvaluator);
        Point.WorldPosition = Point.Direction * (PlanetRadius + Point.Height);
        i++;
    }
}


float FFoliageGenerationTask::CalculateSlope(
    const FVector& Direction,
    int32 PointIndex,
    FCosmicNoiseEvaluator& NoiseEvaluator)
{
    const float SampleDistance = 500.0f;

    FVector Tangent1, Tangent2;
    Direction.FindBestAxisVectors(Tangent1, Tangent2);

    FVector SampleDirs[4] = {
        (Direction + Tangent1 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction - Tangent1 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction + Tangent2 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction - Tangent2 * (SampleDistance / PlanetRadius)).GetSafeNormal()
    };

    float CenterHeight = 0.0f;
    FLinearColor Dummy;

    NoiseEvaluator.EvaluatePoint(Direction, CenterHeight, Dummy);

    float MaxSlope = 0.0f;

    for (int32 i = 0; i < 4; i++)
    {
        float SampleHeight = 0.0f;
        NoiseEvaluator.EvaluatePoint(SampleDirs[i], SampleHeight, Dummy);

        float HeightDiff = FMath::Abs(SampleHeight - CenterHeight);
        float SlopeAngle = FMath::Atan(HeightDiff / SampleDistance) * (180.0f / PI);

        MaxSlope = FMath::Max(MaxSlope, SlopeAngle);
    }

    return MaxSlope;
}

void FFoliageGenerationTask::CreateFoliageInstances(FRandomStream& Random, FCosmicNoiseEvaluator& NoiseEvaluator)
{
    ResultInstances.Empty();

    // Agrupar meshes por capa y entrada
    TMap<const FCosmicFoliageCollectionEntry*, TArray<const FCosmicFoliageMesh*>> MeshesByEntry;
    for (const FCosmicFoliageCollectionEntry& Entry : Collection->FoliageEntries)
    {
        TArray<const FCosmicFoliageMesh*> ValidMeshes;
        for (const FCosmicFoliageMesh& Mesh : Entry.Foliage)
        {
            if (Mesh.FoliageLayer == Layer && Mesh.Mesh)
            {
                ValidMeshes.Add(&Mesh);
            }
        }
        if (ValidMeshes.Num() > 0)
        {
            MeshesByEntry.Add(&Entry, ValidMeshes);
        }
    }

    for (const FSeedPoint& Point : SeedPoints)
    {
        const FCosmicFoliageCollectionEntry* Entry = FindBestMatchingEntry(
            Point.Temperature, Point.Humidity, Point.Slope, Point.Height);

        if (!Entry || !MeshesByEntry.Contains(Entry))
            continue;

        const TArray<const FCosmicFoliageMesh*>& ValidMeshes = MeshesByEntry[Entry];
        if (ValidMeshes.Num() == 0)
            continue;

        // Seleccion por peso de entrada 
        const FCosmicFoliageMesh* SelectedMesh = ValidMeshes[Random.RandRange(0, ValidMeshes.Num() - 1)];

        // Densidad basada EXCLUSIVAMENTE en InstancesPerKm2
        float SpawnProbability = static_cast<float>(SelectedMesh->InstancesPerKm2) / 1000.0f; // Normalizado
        if (Random.FRand() > SpawnProbability)
            continue;

        // Calcular transformación
        float Yaw = Random.FRandRange(SelectedMesh->RandomRotationMin, SelectedMesh->RandomRotationMax);
        float Scale = Random.FRandRange(SelectedMesh->ScaleMin, SelectedMesh->ScaleMax);

        FQuat Rotation;
        FVector OffsetNormal = FVector::UpVector;

        if (SelectedMesh->bAlignToGround)
        {
            FVector TerrainNormal = GetTerrainNormal(Point.Direction, NoiseEvaluator);
            OffsetNormal = TerrainNormal;
            FQuat AlignRotation = FQuat::FindBetweenNormals(FVector::UpVector, TerrainNormal);
            FQuat RandomYawRotation = FQuat(TerrainNormal, FMath::DegreesToRadians(Yaw));
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
        Instance.Mesh = SelectedMesh->Mesh;
        Instance.Transform = Transform;
        ResultInstances.Add(Instance);
    }
}


const FCosmicFoliageCollectionEntry* FFoliageGenerationTask::FindBestMatchingEntry(float Temperature, float Humidity, float Slope, float Height)
{
    for (const FCosmicFoliageCollectionEntry& Entry : Collection->FoliageEntries)
    {
        // Verificar si todas las condiciones están dentro de los rangos
        bool bValid = true;

        bValid &= (Slope >= Entry.SlopeMin && Slope <= Entry.SlopeMax);
        bValid &= (Temperature >= Entry.TemperatureMin && Temperature <= Entry.TemperatureMax);
        bValid &= (Humidity >= Entry.HumidityMin && Humidity <= Entry.HumidityMax);
        bValid &= (Height >= Entry.ElevationMinKm * 100000 && Height <= Entry.ElevationMaxKm * 100000);

        if (bValid)
        {
            
            return &Entry;
        }
    }

    return nullptr;
}

const FCosmicFoliageCollectionEntry* FFoliageGenerationTask::FindClosestMatchingEntry(float Temperature, float Humidity, float Slope, float Height)
{
    const FCosmicFoliageCollectionEntry* BestEntry = nullptr;
    float BestDistance = FLT_MAX;

    for (const FCosmicFoliageCollectionEntry& Entry : Collection->FoliageEntries)
    {
        // Calcular distancia normalizada a los rangos
        float TempDist = 0.0f;
        if (Temperature < Entry.TemperatureMin)
            TempDist = Entry.TemperatureMin - Temperature;
        else if (Temperature > Entry.TemperatureMax)
            TempDist = Temperature - Entry.TemperatureMax;

        float HumDist = 0.0f;
        if (Humidity < Entry.HumidityMin)
            HumDist = Entry.HumidityMin - Humidity;
        else if (Humidity > Entry.HumidityMax)
            HumDist = Humidity - Entry.HumidityMax;

        float SlopeDist = 0.0f;
        if (Slope < Entry.SlopeMin)
            SlopeDist = Entry.SlopeMin - Slope;
        else if (Slope > Entry.SlopeMax)
            SlopeDist = Slope - Entry.SlopeMax;

        float HeightDist = 0.0f;
        if (Height < Entry.ElevationMinKm * 100000)
            HeightDist = Entry.ElevationMinKm * 100000 - Height;
        else if (Height > Entry.ElevationMaxKm * 100000)
            HeightDist = Height - Entry.ElevationMaxKm * 100000;

        // Distancia euclidiana ponderada
        float Distance = FMath::Sqrt(
            TempDist * TempDist * 1.0f +
            HumDist * HumDist * 1.0f +
            SlopeDist * SlopeDist * 0.5f +      // Menor peso para pendiente
            HeightDist * HeightDist * 0.3f       // Menor peso para altura
        );

        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            BestEntry = &Entry;
        }
    }

    return BestEntry;
}

FVector FFoliageGenerationTask::GetTerrainNormal(
    const FVector& Direction,
    FCosmicNoiseEvaluator& NoiseEvaluator)
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

    // height central desde evaluator
    NoiseEvaluator.EvaluatePoint(Direction, CenterHeight, Dummy);

    FVector Positions[4];

    for (int32 i = 0; i < 4; i++)
    {
        float SampleHeight = 0.0f;

        // height consistente con el sistema global
        NoiseEvaluator.EvaluatePoint(SampleDirs[i], SampleHeight, Dummy);

        Positions[i] = PlanetCenter + SampleDirs[i] * (PlanetRadius + SampleHeight);
    }

    FVector V1 = Positions[0] - Positions[1];
    FVector V2 = Positions[2] - Positions[3];

    FVector Normal = FVector::CrossProduct(V1, V2).GetSafeNormal();

    // asegurar orientación hacia fuera del planeta
    if (FVector::DotProduct(Normal, Direction) < 0)
    {
        Normal = -Normal;
    }

    return Normal;
}
