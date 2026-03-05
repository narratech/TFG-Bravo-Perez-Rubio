#include "CosmicFoliageSpawner.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"

// TAREA ASÍNCRONA 
void FFoliageGenerationTask::DoWork()
{
    if (!Collection) return;

    FRandomStream LocalRandom(Seed);
    float CellSizeKm = 1.0f;
    float CellSizeCm = CellSizeKm * 100000.0f;

    float AreaKm2 = (CellSizeKm) * (CellSizeKm);
    int32 NumSeeds = FMath::RoundToInt(AreaKm2 * Collection->SeedsPerSquareKm * Collection->GlobalDensity);

    ResultTransforms.Empty();
    ResultTransforms.Reserve(NumSeeds);

    for (int32 i = 0; i < NumSeeds; i++)
    {
        FVector LocalPos(
            SpawnArea.Min.X + LocalRandom.FRandRange(0.0f, CellSizeCm),
            SpawnArea.Min.Y + LocalRandom.FRandRange(0.0f, CellSizeCm),
            SpawnArea.Min.Z + LocalRandom.FRandRange(0.0f, CellSizeCm)
        );

        const FCosmicFoliageCollectionEntry* Entry = Collection->GetRandomEntry(LocalRandom);
        if (!Entry) continue;

        FTransform Transform;
        Transform.SetLocation(LocalPos);

        // FRandomStream::FRandRange() para floats
        float RandomYaw = LocalRandom.FRandRange(
            Entry->Foliage.RandomRotationMin,
            Entry->Foliage.RandomRotationMax
        );

        if (Entry->Foliage.bAlignToGround)
        {
            FRotator Rotation(0.0f, RandomYaw, 0.0f);
            Transform.SetRotation(Rotation.Quaternion());
        }
        else
        {
            Transform.SetRotation(FRotator(0.0f, RandomYaw, 0.0f).Quaternion());
        }

        // FRandomStream::FRandRange() para floats
        float Scale = LocalRandom.FRandRange(Entry->Foliage.ScaleMin, Entry->Foliage.ScaleMax);
        Transform.SetScale3D(FVector(Scale));

        ResultTransforms.Add(Transform);
    }
}

// COMPONENTE

UCosmicFoliageSpawner::UCosmicFoliageSpawner()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UCosmicFoliageSpawner::BeginPlay()
{
    Super::BeginPlay();
    RandomStream.Initialize(FMath::Rand());
}

void UCosmicFoliageSpawner::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ElapsedTime += DeltaTime;
    if (ElapsedTime < UpdateInterval) return;
    ElapsedTime = 0.0f;

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC || !PC->PlayerCameraManager) return;

    FVector ViewerLocation = PC->PlayerCameraManager->GetCameraLocation();

    CleanupFarInstances(ViewerLocation, ViewDistanceKm * 1.5f);
    RequestAreaGeneration(ViewerLocation, ViewDistanceKm);

    // Procesar tareas completadas
    for (int32 i = ActiveTasks.Num() - 1; i >= 0; i--)
    {
        FAsyncTask<FFoliageGenerationTask>* Task = ActiveTasks[i];

        if (Task->IsDone())
        {
            // GetTask() es del contenedor FAsyncTask
            FFoliageGenerationTask& CompletedTask = Task->GetTask();

            // Necesitas guardar la celda asociada - mejora: pasar Cell como parámetro
            FIntVector Cell = WorldToCell(ViewerLocation, 1.0f);

            ApplyGeneratedInstances(Cell, CompletedTask.ResultTransforms);

            delete Task;
            ActiveTasks.RemoveAt(i);
        }
    }
}

FIntVector UCosmicFoliageSpawner::WorldToCell(const FVector& WorldPos, float CellSizeKm) const
{
    float CellSizeCm = CellSizeKm * 100000.0f;
    return FIntVector(
        FMath::FloorToInt(WorldPos.X / CellSizeCm),
        FMath::FloorToInt(WorldPos.Y / CellSizeCm),
        FMath::FloorToInt(WorldPos.Z / CellSizeCm)
    );
}

void UCosmicFoliageSpawner::RequestAreaGeneration(const FVector& Center, float RadiusKm)
{
    float RadiusCm = RadiusKm * 100000.0f;
    float CellSizeKm = 1.0f;
    float CellSizeCm = CellSizeKm * 100000.0f;

    int32 CellsPerSide = FMath::CeilToInt((RadiusCm * 2) / CellSizeCm);
    FIntVector CenterCell = WorldToCell(Center, CellSizeKm);

    for (int32 x = -CellsPerSide / 2; x <= CellsPerSide / 2; x++)
    {
        for (int32 y = -CellsPerSide / 2; y <= CellsPerSide / 2; y++)
        {
            FIntVector Cell = CenterCell + FIntVector(x, y, 0);

            if (!CellDataMap.Contains(Cell))
            {
                PendingCells.Enqueue(Cell);
            }
        }
    }

    int32 ProcessedThisFrame = 0;
    FIntVector NextCell;
    while (ProcessedThisFrame < MaxInstancesPerFrame && PendingCells.Dequeue(NextCell))
    {
        GenerateCell(NextCell);
        ProcessedThisFrame++;
    }
}

void UCosmicFoliageSpawner::GenerateCell(const FIntVector& Cell)
{
    if (!FoliageCollection) return;

    float CellSizeKm = 1.0f;
    float CellSizeCm = CellSizeKm * 100000.0f;

    FBox CellBox(
        FVector(Cell.X * CellSizeCm, Cell.Y * CellSizeCm, Cell.Z * CellSizeCm),
        FVector((Cell.X + 1) * CellSizeCm, (Cell.Y + 1) * CellSizeCm, (Cell.Z + 1) * CellSizeCm)
    );

    // Crear y lanzar tarea
    FFoliageGenerationTask* Task = new FFoliageGenerationTask(
        CellBox,
        FoliageCollection,
        RandomStream.FRand(),  // Rand() para int32
        1.0f
    );

    FAsyncTask<FFoliageGenerationTask>* AsyncTask = new FAsyncTask<FFoliageGenerationTask>(*Task);
    AsyncTask->StartBackgroundTask();

    ActiveTasks.Add(AsyncTask);

    // TODO: Asociar la celda con la tarea
    // Crear un struct que guarde (FAsyncTask*, FIntVector)
}

void UCosmicFoliageSpawner::ApplyGeneratedInstances(const FIntVector& Cell, const TArray<FTransform>& Transforms)
{
    FCosmicFoliageCellData& CellData = CellDataMap.FindOrAdd(Cell);

    CellData.Instances = Transforms;
    CellData.LastUpdateTime = GetWorld()->GetTimeSeconds();

    UE_LOG(LogTemp, Verbose, TEXT("Cell %s generated %d instances"),
        *Cell.ToString(), Transforms.Num());
}

void UCosmicFoliageSpawner::CleanupFarInstances(const FVector& ViewerLocation, float MaxDistanceKm)
{
    float MaxDistanceSq = FMath::Square(MaxDistanceKm * 100000.0f);
    TArray<FIntVector> CellsToRemove;

    for (const auto& Pair : CellDataMap)
    {
        FVector CellCenter(
            Pair.Key.X * 100000.0f + 50000.0f,
            Pair.Key.Y * 100000.0f + 50000.0f,
            Pair.Key.Z * 100000.0f + 50000.0f
        );

        if (FVector::DistSquared(CellCenter, ViewerLocation) > MaxDistanceSq)
        {
            CellsToRemove.Add(Pair.Key);
        }
    }

    for (const FIntVector& Cell : CellsToRemove)
    {
        CellDataMap.Remove(Cell);
    }
}