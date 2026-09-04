// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicFoliageGenerationTask.h"
#include "ICosmicNoiseStrategy.h"

namespace
{
    FVector CubePointForFace(int32 Face, double X, double Y)
    {
        switch (Face)
        {
        case 0: return FVector(1.0, X, Y);
        case 1: return FVector(-1.0, X, Y);
        case 2: return FVector(X, 1.0, Y);
        case 3: return FVector(X, -1.0, Y);
        case 4: return FVector(X, Y, 1.0);
        case 5: return FVector(X, Y, -1.0);
        default: return FVector::ZeroVector;
        }
    }

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

    bool IsInsideRange(float Value, float A, float B)
    {
        return Value >= FMath::Min(A, B) && Value <= FMath::Max(A, B);
    }
}

// TAREA ASINCRONA 
void FFoliageGenerationTask::DoWork()
{
    if (!FoliageEntries.IsValid() || FoliageEntries->IsEmpty() ||
        !NoiseGenerationStrategy.IsValid() || PlanetRadius <= 0.0)
    {
        return;
    }

    //Crear siempre la misma semilla para la misma celda
    uint32 Hash = 2166136261u;

    Hash = (Hash ^ Cell.Face) * 16777619u;
    Hash = (Hash ^ Cell.X) * 16777619u;
    Hash = (Hash ^ Cell.Y) * 16777619u;
    Hash = (Hash ^ Cell.Depth) * 16777619u;
    Hash = (Hash ^ static_cast<uint32>(Layer)) * 16777619u;

    FRandomStream LocalRandom(Hash);

    CellAreaKm2 = CalculateCellAreaKm2();
    if (CellAreaKm2 <= 0.0f)
    {
        return;
    }

    // Generar puntos de semilla en la esfera
    GenerateSeedPoints(LocalRandom);
    
    // Evaluar condiciones ambientales para cada punto
    EvaluateEnvironmentalConditions();

    // Seleccionar y crear instancias basadas en las condiciones
    CreateFoliageInstances(LocalRandom);
}

double FFoliageGenerationTask::CalculateCellAreaKm2() const
{
    const int32 CellsPerSide = 1 << Cell.Depth;
    const double CellSize = 2.0 / static_cast<double>(CellsPerSide);
    const double MinX = -1.0 + static_cast<double>(Cell.X) * CellSize;
    const double MaxX = MinX + CellSize;
    const double MinY = -1.0 + static_cast<double>(Cell.Y) * CellSize;
    const double MaxY = MinY + CellSize;

    const FVector D00 = CubePointForFace(Cell.Face, MinX, MinY).GetSafeNormal();
    const FVector D10 = CubePointForFace(Cell.Face, MaxX, MinY).GetSafeNormal();
    const FVector D11 = CubePointForFace(Cell.Face, MaxX, MaxY).GetSafeNormal();
    const FVector D01 = CubePointForFace(Cell.Face, MinX, MaxY).GetSafeNormal();

    const double SolidAngle =
        SphericalTriangleSolidAngle(D00, D10, D11) +
        SphericalTriangleSolidAngle(D00, D11, D01);
    const double AreaCm2 = SolidAngle * PlanetRadius * PlanetRadius;
    return AreaCm2 / 10000000000.0;
}

int32 FFoliageGenerationTask::PrepareAllocations(FRandomStream& Random)
{
    Allocations.Reset();
    int64 TotalTargets = 0;
    const TArray<FCosmicFoliageCollectionEntry>& Entries = *FoliageEntries;

    for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex)
    {
        const FCosmicFoliageCollectionEntry& Entry = Entries[EntryIndex];
        for (int32 MeshIndex = 0; MeshIndex < Entry.Foliage.Num(); ++MeshIndex)
        {
            const FCosmicFoliageMesh& Mesh = Entry.Foliage[MeshIndex];
            if (Mesh.FoliageLayer != Layer || !Mesh.Mesh || Mesh.InstancesPerKm2 <= 0)
            {
                continue;
            }

            const double ExpectedCount = FMath::Clamp(
                static_cast<double>(CellAreaKm2) * Mesh.InstancesPerKm2,
                0.0,
                static_cast<double>(MAX_int32));
            const int32 WholeCount = static_cast<int32>(ExpectedCount);
            const double Fraction = ExpectedCount - WholeCount;
            const int32 TargetCount = WholeCount + (Random.FRand() < Fraction ? 1 : 0);

            if (TargetCount > 0)
            {
                const float MinimumSlope = FMath::Min(Entry.SlopeMin, Entry.SlopeMax);
                const float MaximumSlope = FMath::Max(Entry.SlopeMin, Entry.SlopeMax);
                const bool bSlopeIsUnrestricted =
                    MinimumSlope <= 0.0f && MaximumSlope >= 90.0f;
                Allocations.Add({
                    EntryIndex,
                    MeshIndex,
                    TargetCount,
                    Mesh.bAlignToGround || !bSlopeIsUnrestricted
                });
                TotalTargets += TargetCount;
            }
        }
    }

    const int32 SafeMaximum = FMath::Clamp(MaxInstancesPerCell, 1, 1000000);
    if (TotalTargets > SafeMaximum)
    {
        const double Scale = static_cast<double>(SafeMaximum) / TotalTargets;
        TArray<TPair<double, int32>> Remainders;
        Remainders.Reserve(Allocations.Num());

        int32 AssignedTargets = 0;
        for (int32 Index = 0; Index < Allocations.Num(); ++Index)
        {
            const double ScaledTarget = Allocations[Index].TargetCount * Scale;
            const int32 WholeTarget = static_cast<int32>(ScaledTarget);
            Allocations[Index].TargetCount = WholeTarget;
            AssignedTargets += WholeTarget;
            // El ruido solo desempata cuotas idénticas sin alterar la proporción.
            Remainders.Emplace(
                ScaledTarget - WholeTarget + Random.FRand() * UE_DOUBLE_SMALL_NUMBER,
                Index);
        }

        Remainders.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B)
        {
            return A.Key > B.Key;
        });

        for (int32 Index = 0;
            AssignedTargets < SafeMaximum && Index < Remainders.Num();
            ++Index, ++AssignedTargets)
        {
            ++Allocations[Remainders[Index].Value].TargetCount;
        }

        TotalTargets = SafeMaximum;
    }

    return static_cast<int32>(FMath::Min<int64>(TotalTargets, MAX_int32));
}

void FFoliageGenerationTask::GenerateSeedPoints(FRandomStream& Random)
{
    SeedPoints.Reset();

    const int32 NumSeeds = PrepareAllocations(Random);

    if (NumSeeds == 0)
        return;

    SeedPoints.Reserve(NumSeeds);

    // Datos de la celda para mapeo UV
    const int32 CellsPerSide = 1 << Cell.Depth;
    const double CellSizeUV = 1.0 / CellsPerSide;
    const double MinU = Cell.X * CellSizeUV;
    const double MaxU = (Cell.X + 1) * CellSizeUV;
    const double MinV = Cell.Y * CellSizeUV;
    const double MaxV = (Cell.Y + 1) * CellSizeUV;
    const double MinX = MinU * 2.0 - 1.0;
    const double MaxX = MaxU * 2.0 - 1.0;
    const double MinY = MinV * 2.0 - 1.0;
    const double MaxY = MaxV * 2.0 - 1.0;

    // La normalizacion cubo->esfera no conserva area. Su Jacobiano es
    // (1+x^2+y^2)^(-3/2); el rechazo evita concentrar vegetacion en unas
    // zonas de la cara y perderla en otras.
    const double ClosestX = FMath::Clamp(0.0, MinX, MaxX);
    const double ClosestY = FMath::Clamp(0.0, MinY, MaxY);
    const double MaximumJacobian = FMath::Pow(
        1.0 + ClosestX * ClosestX + ClosestY * ClosestY,
        -1.5);
    for (int32 AllocationIndex = 0; AllocationIndex < Allocations.Num(); ++AllocationIndex)
    {
        const int32 TargetCount = Allocations[AllocationIndex].TargetCount;
        const int32 MaximumAttempts = FMath::Max(TargetCount * 16, 64);
        int32 GeneratedCount = 0;

        for (int32 Attempt = 0;
            Attempt < MaximumAttempts && GeneratedCount < TargetCount;
            ++Attempt)
        {
            const double X = FMath::Lerp(MinX, MaxX, static_cast<double>(Random.FRand()));
            const double Y = FMath::Lerp(MinY, MaxY, static_cast<double>(Random.FRand()));
            const double Jacobian = FMath::Pow(1.0 + X * X + Y * Y, -1.5);
            if (Random.FRand() * MaximumJacobian > Jacobian)
            {
                continue;
            }

            FSeedPoint Point;
            Point.Direction = CubePointForFace(Cell.Face, X, Y).GetSafeNormal();
            Point.AllocationIndex = AllocationIndex;
            SeedPoints.Add(Point);
            ++GeneratedCount;
        }
    }
}

void FFoliageGenerationTask::EvaluateEnvironmentalConditions()
{
    for (FSeedPoint& Point : SeedPoints)
    {
        FLinearColor BiomeData;
        NoiseGenerationStrategy->EvaluatePoint(Point.Direction, Point.Height, BiomeData);

        Point.Temperature = BiomeData.G;
        Point.Humidity = BiomeData.B;
        Point.WorldPosition = Point.Direction * (PlanetRadius + Point.Height);

        if (Allocations.IsValidIndex(Point.AllocationIndex) &&
            Allocations[Point.AllocationIndex].bNeedsSurfaceNormal)
        {
            // La altura central ya evaluada se reutiliza para la pendiente y la normal.
            CalculateSlopeAndNormal(
                Point.Direction,
                Point.Height,
                Point.Slope,
                Point.CachedNormal);
        }
        else
        {
            Point.Slope = 0.0f;
            Point.CachedNormal = Point.Direction;
        }
    }
}

void FFoliageGenerationTask::CreateFoliageInstances(FRandomStream& Random)
{
    ResultInstances.Empty();
    const TArray<FCosmicFoliageCollectionEntry>& Entries = *FoliageEntries;

    int32 TotalTarget = 0;
    for (const FMeshAllocation& Allocation : Allocations)
    {
        TotalTarget += Allocation.TargetCount;
    }

    if (Allocations.Num() == 0 || TotalTarget == 0)
        return;

    ResultInstances.Reserve(FMath::Min(SeedPoints.Num(), TotalTarget));

    // Cada punto conserva la malla cuya densidad lo generó. Así una regla de
    // bioma no puede apropiarse de los candidatos de otra y falsear densidades.
    for (const FSeedPoint& Point : SeedPoints)
    {
        if (!Allocations.IsValidIndex(Point.AllocationIndex))
        {
            continue;
        }

        const FMeshAllocation& Alloc = Allocations[Point.AllocationIndex];
        const FCosmicFoliageCollectionEntry& Entry = Entries[Alloc.EntryIndex];
        const bool bValid =
            IsInsideRange(Point.Slope, Entry.SlopeMin, Entry.SlopeMax) &&
            IsInsideRange(Point.Temperature, Entry.TemperatureMin, Entry.TemperatureMax) &&
            IsInsideRange(Point.Humidity, Entry.HumidityMin, Entry.HumidityMax) &&
            IsInsideRange(Point.Height, Entry.ElevationMinKm * 100000.f, Entry.ElevationMaxKm * 100000.f);
        if (!bValid)
        {
            continue;
        }

        const FCosmicFoliageMesh& SelectedMesh =
            Entry.Foliage[Alloc.MeshIndex];

        // Calcular transformación 
        const float Yaw = Random.FRandRange(
            FMath::Min(SelectedMesh.RandomRotationMin, SelectedMesh.RandomRotationMax),
            FMath::Max(SelectedMesh.RandomRotationMin, SelectedMesh.RandomRotationMax));
        const float MinimumScale = FMath::Max(
            UE_KINDA_SMALL_NUMBER,
            FMath::Min(SelectedMesh.ScaleMin, SelectedMesh.ScaleMax));
        const float MaximumScale = FMath::Max(
            MinimumScale,
            FMath::Max(SelectedMesh.ScaleMin, SelectedMesh.ScaleMax));
        const float Scale = Random.FRandRange(MinimumScale, MaximumScale);

        FQuat   Rotation;

        if (SelectedMesh.bAlignToGround)
        {
            FQuat AlignRotation = FQuat::FindBetweenNormals(FVector::UpVector, Point.CachedNormal);
            FQuat RandomYawRotation = FQuat(Point.CachedNormal, FMath::DegreesToRadians(Yaw));
            Rotation = RandomYawRotation * AlignRotation;
        }
        else if (SelectedMesh.bAlignToPlanetNormal)
        {
            FQuat AlignRotation = FQuat::FindBetweenNormals(FVector::UpVector, Point.Direction);
            FQuat RandomYawRotation = FQuat(Point.Direction, FMath::DegreesToRadians(Yaw));
            Rotation = RandomYawRotation * AlignRotation;
        }
        else
        {
            Rotation = FRotator(0, Yaw, 0).Quaternion();
        }

        FTransform Transform;
        const FVector FinalPosition =
            Point.WorldPosition + Rotation.RotateVector(SelectedMesh.HeightOffset);
        Transform.SetLocation(FinalPosition);
        Transform.SetRotation(Rotation);
        Transform.SetScale3D(FVector(Scale));

        FCosmicFoliageInstance Instance;
        Instance.HISMKey.Mesh = SelectedMesh.Mesh;
        Instance.HISMKey.bHasCollision = SelectedMesh.bHasCollision;
        Instance.Transform = Transform;
        ResultInstances.Add(Instance);

    }
}

void FFoliageGenerationTask::CalculateSlopeAndNormal(
    const FVector& Direction,
    float CenterHeight,
    float& OutSlope,
    FVector& OutNormal)
{
    const double SampleDistance = FMath::Max(1.0, static_cast<double>(NormalSampleDistanceCm));

    FVector Tangent1, Tangent2;
    Direction.FindBestAxisVectors(Tangent1, Tangent2);

    FVector SampleDirs[2] =
    {
        (Direction + Tangent1 * (SampleDistance / PlanetRadius)).GetSafeNormal(),
        (Direction + Tangent2 * (SampleDistance / PlanetRadius)).GetSafeNormal()
    };

    FLinearColor Dummy;

    FVector Positions[2];

    for (int32 i = 0; i < 2; i++)
    {
        float SampleHeight = 0.0f;
        NoiseGenerationStrategy->EvaluatePoint(SampleDirs[i], SampleHeight, Dummy);

        Positions[i] = SampleDirs[i] * (PlanetRadius + SampleHeight);
    }

    const FVector CenterPosition = Direction * (PlanetRadius + CenterHeight);
    const FVector V1 = Positions[0] - CenterPosition;
    const FVector V2 = Positions[1] - CenterPosition;
    FVector Normal = FVector::CrossProduct(V1, V2).GetSafeNormal();

    if (FVector::DotProduct(Normal, Direction) < 0)
        Normal = -Normal;

    OutNormal = Normal.IsNearlyZero() ? Direction : Normal;
    OutSlope = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
        FVector::DotProduct(OutNormal, Direction),
        -1.0,
        1.0)));
}
