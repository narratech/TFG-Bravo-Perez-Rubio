// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicFoliageGenerationTask.h"

// TAREA ASINCRONA 
void FFoliageGenerationTask::DoWork()
{
    if (!Collection) return;

    //Crear siempre la misma semilla para la misma celsa
    uint32 Hash = 2166136261u;

    Hash = (Hash ^ Cell.Face) * 16777619u;
    Hash = (Hash ^ Cell.X) * 16777619u;
    Hash = (Hash ^ Cell.Y) * 16777619u;
    Hash = (Hash ^ Cell.Depth) * 16777619u;

    FRandomStream LocalRandom(Hash);

    // Generar puntos de semilla en la esfera
    GenerateSeedPoints(LocalRandom);

    FCosmicNoiseEvaluator Evaluator(NoiseSettings);
    // Evaluar condiciones ambientales para cada punto
    EvaluateEnvironmentalConditions(LocalRandom, Evaluator);

    // Seleccionar y crear instancias basadas en las condiciones
    CreateFoliageInstances(LocalRandom, Evaluator);
}

void FFoliageGenerationTask::GenerateSeedPoints(FRandomStream& Random)
{
    SeedPoints.Empty();

    float Density = Collection->SeedsPerSquareKm * Collection->GlobalDensity;

    int32 NumSeeds = FMath::Max(1, FMath::RoundToInt(CellAreaKm2 * Density));

    SeedPoints.Reserve(NumSeeds);

    // Datos de la celda
    int32 CellsPerSide = 1 << Cell.Depth;
    float CellSizeUV = 1.0f / CellsPerSide;

    float MinU = Cell.X * CellSizeUV;
    float MaxU = (Cell.X + 1) * CellSizeUV;
    float MinV = Cell.Y * CellSizeUV;
    float MaxV = (Cell.Y + 1) * CellSizeUV;

    for (int32 i = 0; i < NumSeeds; i++)
    {
        // Sample uniforme dentro de la celda
        float U = Random.FRandRange(MinU, MaxU);
        float V = Random.FRandRange(MinV, MaxV);

        // Convertir a espacio [-1,1]
        float X = U * 2.0f - 1.0f;
        float Y = V * 2.0f - 1.0f;

        FVector CubePoint;

        // Mapear segun la cara
        switch (Cell.Face)
        {
        case 0: CubePoint = FVector(1.0f, X, Y); break;   // +X
        case 1: CubePoint = FVector(-1.0f, X, Y); break;  // -X
        case 2: CubePoint = FVector(X, 1.0f, Y); break;   // +Y
        case 3: CubePoint = FVector(X, -1.0f, Y); break;  // -Y
        case 4: CubePoint = FVector(X, Y, 1.0f); break;   // +Z
        case 5: CubePoint = FVector(X, Y, -1.0f); break;  // -Z
        default: CubePoint = FVector::ZeroVector; break;
        }

        // Proyectar a esfera 
        FVector Direction = CubePoint.GetSafeNormal();

        FSeedPoint Point;
        Point.Direction = Direction;

        SeedPoints.Add(Point);
    }
}

void FFoliageGenerationTask::EvaluateEnvironmentalConditions(FRandomStream& Random, FCosmicNoiseEvaluator& NoiseEvaluator)
{
    if (SeedPoints.Num() == 0) return;

    uint32 i = 0;
    
    // Convertir direcciones a puntos de ruido (coordenadas normalizadas)
    for (FSeedPoint& Point : SeedPoints)
    {
        float X = Point.Direction.X;
        float Y = Point.Direction.Y;
        float Z = Point.Direction.Z;

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
    ResultInstances.Reserve(SeedPoints.Num() / 2); // Aproximadamente la mitad serán válidos

    for (const FSeedPoint& Point : SeedPoints)
    {
        // Encontrar la entrada que mejor se adapta a las condiciones
        const FCosmicFoliageCollectionEntry* Entry = FindBestMatchingEntry(
            Point.Temperature,
            Point.Humidity,
            Point.Slope,
            Point.Height
        );

        // Si no encontramos una perfecta, buscar la más cercana
        /*if (!Entry)
        {
            Entry = FindClosestMatchingEntry(
                Point.Temperature,
                Point.Humidity,
                Point.Slope,
                Point.Height
            );
        }*/

        if (!Entry || Entry->Foliage.Num() == 0)
            continue;

        // Seleccionar mesh aleatorio del array
        int32 MeshIndex = Random.RandRange(0, Entry->Foliage.Num() - 1);
        const FCosmicFoliageMesh& SelectedMesh = Entry->Foliage[MeshIndex];

        if (!SelectedMesh.Mesh)
            continue;

        // Ajustar densidad según el multiplicador del mesh
        if (Random.FRand() > SelectedMesh.DensityMultiplier)
            continue;

        

        // Calcular transformación final
        float Yaw = Random.FRandRange(
            SelectedMesh.RandomRotationMin,
            SelectedMesh.RandomRotationMax
        );

        float Scale = Random.FRandRange(
            SelectedMesh.ScaleMin,
            SelectedMesh.ScaleMax
        );

        FVector OffsetNormal = FVector::UpVector;

        FQuat Rotation;
        if (SelectedMesh.bAlignToGround)
        {
            // Obtener normal del terreno (también podría optimizarse con cálculo por lotes)
            FVector TerrainNormal = OffsetNormal = GetTerrainNormal(Point.Direction, NoiseEvaluator);

            // Rotacion que alinea el up vector del mesh con la normal
            FQuat AlignRotation = FQuat::FindBetweenNormals(FVector::UpVector, TerrainNormal);

            // Anadir rotacion aleatoria alrededor de la normal
            FQuat RandomYawRotation = FQuat(TerrainNormal, FMath::DegreesToRadians(Yaw));

            Rotation = RandomYawRotation * AlignRotation;
        }
        else if(SelectedMesh.bAlignToPlanetNormal)
        {
            FVector PlanetNormal = OffsetNormal = Point.Direction;

            // Alinear el up del mesh con la normal del planeta
            FQuat AlignRotation = FQuat::FindBetweenNormals(FVector::UpVector, PlanetNormal);

            // Rotacion aleatoria alrededor de la normal (para variedad)
            FQuat RandomYawRotation = FQuat(PlanetNormal, FMath::DegreesToRadians(Yaw));

            Rotation = RandomYawRotation * AlignRotation;         
        }
        else {
            Rotation = FRotator(0, Yaw, 0).Quaternion();
        }

        FTransform Transform;
        FVector FinalPosition = Point.WorldPosition;

        FinalPosition += OffsetNormal * SelectedMesh.HeightOffset;

        Transform.SetLocation(FinalPosition);
        Transform.SetRotation(Rotation);
        Transform.SetScale3D(FVector(Scale));

        //UE_LOG(LogTemp, Warning, TEXT("Generando en X:%.4f, Y:%.4f, Z:%.4f"), Point.WorldPosition.X, Point.WorldPosition.Y, Point.WorldPosition.Z);

        FCosmicFoliageInstance Instance;
        Instance.Mesh = SelectedMesh.Mesh;
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
