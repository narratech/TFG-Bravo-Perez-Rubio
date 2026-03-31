// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicFoliageGenerationTask.h"
#include "CosmicArchitectNoise/Public/CosmicNoise.h"
#include "CosmicArchitectNoise/Public/ThirdParty/FastNoiseLite.h"

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

    // Evaluar condiciones ambientales para cada punto
    EvaluateEnvironmentalConditions(LocalRandom);

    // Seleccionar y crear instancias basadas en las condiciones
    CreateFoliageInstances(LocalRandom);
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

void FFoliageGenerationTask::EvaluateEnvironmentalConditions(FRandomStream& Random)
{
    if (SeedPoints.Num() == 0) return;

    TArray<FVector> NoisePoints;
    NoisePoints.Reserve(SeedPoints.Num());

    // Convertir direcciones a puntos de ruido (coordenadas normalizadas)
    for (const FSeedPoint& Point : SeedPoints)
    {
        // Usamos la dirección como coordenada de ruido (esfera unitaria)
        NoisePoints.Add(Point.Direction);
    }

    // Calcular todas las alturas 
    TArray<float> Heights = CosmicNoise::CalculateHeightsDirect(NoisePoints, NoiseSettings);

    // Inicializar ruidos para temperatura y humedad
    FastNoiseLite HumidityNoise;
    FastNoiseLite TempNoise;

    HumidityNoise.SetSeed(NoiseSettings.Seed);
    HumidityNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    HumidityNoise.SetFractalType(FastNoiseLite::FractalType_FBm);
    HumidityNoise.SetFrequency(NoiseSettings.HumidityFrequency * 100.0f);
    HumidityNoise.SetFractalOctaves(NoiseSettings.HumidityOctaves);

    TempNoise.SetSeed(NoiseSettings.Seed);
    TempNoise.SetFrequency(NoiseSettings.TemperatureFrequency * 100.0f);

    float MaxPossibleHeight = 0.0f;

    for (int i = 0; i < NoiseSettings.Biomes.Num(); i++){
        float BiomeMaxHeight = 0.0f;
        const FCosmicBiomeData& BiomeData = NoiseSettings.Biomes[i];

        for (int j = 0; j < BiomeData.NoiseLayers.Num(); j++) {
            const FCosmicNoiseTypes& Layer = BiomeData.NoiseLayers[j];
            BiomeMaxHeight += Layer.Amplitude;
        }
        MaxPossibleHeight = FMath::Max(MaxPossibleHeight, BiomeMaxHeight);
    }
    if (MaxPossibleHeight == 0.0f) MaxPossibleHeight = 1000.0f;


    for (int32 i = 0; i < SeedPoints.Num(); i++)
    {
        FSeedPoint& Point = SeedPoints[i];
        float X = Point.Direction.X;
        float Y = Point.Direction.Y;
        float Z = Point.Direction.Z;

        // Asignar altura calculada
        Point.Height = Heights[i];

        // CALCULAR TEMPERATURA 
        float Latitude = FMath::Abs(Z);
        float BaseTemp = 1.0f - (Latitude * NoiseSettings.LatitudeEffect);

        // Penalización por altitud (a mayor altura, menor temperatura)
        float AltitudePenalty = FMath::Clamp(Point.Height / MaxPossibleHeight, 0.0f, 0.5f);
        BaseTemp -= AltitudePenalty;

        // Variación por ruido
        float TempVariance = TempNoise.GetNoise(X, Y, Z) * 0.2f;
        Point.Temperature = FMath::Clamp(BaseTemp + TempVariance, 0.0f, 1.0f);

        // CALCULAR HUMEDAD
        float RawHum = HumidityNoise.GetNoise(X, Y, Z);
        float BaseHum = (RawHum + 1.0f) * 0.5f + NoiseSettings.HumidityOffset;

        // Las zonas bajas y cerca del ecuador son más húmedas
        float ElevationFactor = 1.0f - FMath::Clamp(Point.Height / 3000.0f, 0.0f, 0.7f);
        float LatitudeHumidity = 1.0f - Latitude * 0.5f;

        Point.Humidity = FMath::Clamp(
            (BaseHum * 0.6f + ElevationFactor * 0.2f + LatitudeHumidity * 0.2f) *
            NoiseSettings.HumidityContrast + (1.0f - NoiseSettings.HumidityContrast) * 0.5f,
            0.0f, 1.0f
        );

        // --- CALCULAR PENDIENTE (necesita alturas de puntos vecinos) ---
        // Para la pendiente necesitamos muestrear puntos cercanos
        // Esto es más complejo de hacer por lotes, lo dejamos como método separado
        Point.Slope = CalculateSlope(Point.Direction, i, Heights, Random);

        // Actualizar posición final con la altura del terreno
        Point.WorldPosition = Point.Direction * (PlanetRadius + Point.Height);
    }
}


float FFoliageGenerationTask::CalculateSlope(const FVector& Direction, int32 PointIndex, const TArray<float>& AllHeights, FRandomStream& Random)
{
    // La pendiente se calcula muestreando la altura en puntos cercanos
    const float SampleDistance = 500.0f; // 5 metros

    // Crear dos vectores perpendiculares a la dirección
    FVector Tangent1, Tangent2;
    Direction.FindBestAxisVectors(Tangent1, Tangent2);

    // Generar puntos de muestra alrededor
    FVector SampleDirs[] = {
        (Direction + Tangent1 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction - Tangent1 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction + Tangent2 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction - Tangent2 * (SampleDistance / PlanetRadius)).GetSafeNormal()
    };

    // Convertir direcciones a puntos de ruido
    TArray<FVector> NoisePoints;
    for (int32 i = 0; i < 4; i++)
    {
        NoisePoints.Add(SampleDirs[i]);
    }

    // Calcular alturas de los puntos de muestra
    TArray<float> SampleHeights = CosmicNoise::CalculateHeightsDirect(NoisePoints, NoiseSettings);

    float CenterHeight = AllHeights[PointIndex];
    float MaxSlope = 0.0f;

    for (int32 i = 0; i < 4; i++)
    {
        float HeightDiff = FMath::Abs(SampleHeights[i] - CenterHeight);
        float SlopeAngle = FMath::Atan(HeightDiff / SampleDistance) * (180.0f / PI);
        MaxSlope = FMath::Max(MaxSlope, SlopeAngle);
    }

    return MaxSlope;
}

void FFoliageGenerationTask::CreateFoliageInstances(FRandomStream& Random)
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
        if (!Entry)
        {
            Entry = FindClosestMatchingEntry(
                Point.Temperature,
                Point.Humidity,
                Point.Slope,
                Point.Height
            );
        }

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
            FVector TerrainNormal = OffsetNormal = GetTerrainNormal(Point.Direction, Random);

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
        bValid &= (Height >= Entry.ElevationMin && Height <= Entry.ElevationMax);

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
        if (Height < Entry.ElevationMin)
            HeightDist = Entry.ElevationMin - Height;
        else if (Height > Entry.ElevationMax)
            HeightDist = Height - Entry.ElevationMax;

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

FVector FFoliageGenerationTask::GetTerrainNormal(const FVector& Direction, FRandomStream& Random)
{
    // Para la normal también podemos usar cálculo por lotes
    const float SampleDistance = 500.0f;

    FVector Tangent1, Tangent2;
    Direction.FindBestAxisVectors(Tangent1, Tangent2);

    // Generar puntos para muestrear
    FVector SampleDirs[4] = {
        (Direction + Tangent1 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction - Tangent1 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction + Tangent2 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction - Tangent2 * (SampleDistance / PlanetRadius)).GetSafeNormal()
    };

    // Preparar puntos de ruido
    TArray<FVector> NoisePoints;
    for (int32 i = 0; i < 4; i++)
    {
        NoisePoints.Add(SampleDirs[i]);
    }

    // Calcular alturas por lotes
    TArray<float> Heights = CosmicNoise::CalculateHeightsDirect(NoisePoints, NoiseSettings);

    // Calcular posiciones en el mundo
    FVector Positions[4];
    for (int32 i = 0; i < 4; i++)
    {
        Positions[i] = PlanetCenter + SampleDirs[i] * (PlanetRadius + Heights[i]);
    }

    // Calcular normal aproximada
    FVector V1 = Positions[0] - Positions[1];
    FVector V2 = Positions[2] - Positions[3];
    FVector Normal = FVector::CrossProduct(V1, V2).GetSafeNormal();

    // Asegurar que la normal apunta hacia afuera
    if (FVector::DotProduct(Normal, Direction) < 0)
    {
        Normal = -Normal;
    }

    return Normal;
}
