#include "CosmicFoliageSpawner.h"
#include "Engine/World.h"
#include "CosmicNoiseSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"



// COMPONENTE

UCosmicFoliageSpawner::UCosmicFoliageSpawner()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCosmicFoliageSpawner::BeginPlay()
{
    Super::BeginPlay();
    RandomStream.Initialize(FMath::Rand());
}

void UCosmicFoliageSpawner::UpdateFoliageGeneration(float DeltaTime, const FVector& PlanetCenter, const float PlanetRadius, UCosmicNoiseSettings* NoiseSettings)
{
    ElapsedTime += DeltaTime;
    if (ElapsedTime < UpdateInterval) return;
    ElapsedTime = 0.0f;

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC || !PC->PlayerCameraManager) return;

    FVector ViewerLocation = PC->PlayerCameraManager->GetCameraLocation();

    CleanupFarInstances(ViewerLocation, ViewDistanceKm * 1.5f);
    RequestAreaGeneration(ViewerLocation, ViewDistanceKm);

    UE_LOG(LogTemp, Warning, TEXT("Numero de tareas activas: %d"), ActiveTasks.Num());

    // Procesar tareas completadas
    for (int32 i = ActiveTasks.Num() - 1; i >= 0; i--)
    {
        FAsyncTask<FFoliageGenerationTask>* Task = ActiveTasks[i];

        if (Task->IsDone())
        {
            //UE_LOG(LogTemp, Warning, TEXT("Tarea %d terminada: "), i);
            // GetTask() es del contenedor FAsyncTask
            FFoliageGenerationTask& CompletedTask = Task->GetTask();

            ApplyGeneratedInstances(CompletedTask.Cell, CompletedTask.ResultInstances);

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

    /*FAsyncTask<FFoliageGenerationTask>* AsyncTask =
        new FAsyncTask<FFoliageGenerationTask>(
            CellBox,
            FoliageCollection,
            RandomStream.FRand(),
            1.0f,
            Cell
        );

    AsyncTask->StartBackgroundTask();

    ActiveTasks.Add(AsyncTask);*/

    // TODO: Asociar la celda con la tarea
    // Crear un struct que guarde (FAsyncTask*, FIntVector)
}

void UCosmicFoliageSpawner::ApplyGeneratedInstances(
    const FIntVector& Cell,
    const TArray<FCosmicFoliageInstance>& Instances)
{
    FCosmicFoliageCellData& CellData = CellDataMap.FindOrAdd(Cell);

    TMap<UStaticMesh*, TArray<FTransform>> Batch;

    for (const FCosmicFoliageInstance& Inst : Instances)
    {
        if (!Inst.Mesh)
            continue;

        Batch.FindOrAdd(Inst.Mesh).Add(Inst.Transform);
    }

    for (auto& Pair : Batch)
    {
        UStaticMesh* Mesh = Pair.Key;
        TArray<FTransform>& Transforms = Pair.Value;

        UHierarchicalInstancedStaticMeshComponent* Comp =
            GetOrCreateCellComponent(CellData, Mesh);

        if (!Comp)
            continue;

        Comp->AddInstances(Transforms, false);
    }

    CellData.LastUpdateTime = GetWorld()->GetTimeSeconds();
}

UHierarchicalInstancedStaticMeshComponent* UCosmicFoliageSpawner::GetOrCreateComponent(UStaticMesh* Mesh)
{
    if (!Mesh) return nullptr;

    if (MeshComponents.Contains(Mesh))
        return MeshComponents[Mesh];

    UHierarchicalInstancedStaticMeshComponent* Comp =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(GetOwner());

    Comp->SetStaticMesh(Mesh);
    Comp->SetupAttachment(GetOwner()->GetRootComponent());
    Comp->RegisterComponent();

    MeshComponents.Add(Mesh, Comp);

    return Comp;
}

UHierarchicalInstancedStaticMeshComponent* UCosmicFoliageSpawner::GetOrCreateCellComponent(FCosmicFoliageCellData& CellData, UStaticMesh* Mesh)
{
    if (!Mesh)
        return nullptr;

    if (CellData.MeshComponents.Contains(Mesh))
        return CellData.MeshComponents[Mesh];

    UHierarchicalInstancedStaticMeshComponent* Comp =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(GetOwner());

    Comp->SetStaticMesh(Mesh);
    Comp->SetupAttachment(GetOwner()->GetRootComponent());
    Comp->RegisterComponent();

    CellData.MeshComponents.Add(Mesh, Comp);

    return Comp;
}

void UCosmicFoliageSpawner::CleanupFarInstances(
    const FVector& ViewerLocation,
    float MaxDistanceKm)
{
    float MaxDistanceSq = FMath::Square(MaxDistanceKm * 100000.0f);

    TArray<FIntVector> CellsToRemove;

    for (auto& Pair : CellDataMap)
    {
        const FIntVector& Cell = Pair.Key;
        FCosmicFoliageCellData& Data = Pair.Value;

        FVector CellCenter(
            Cell.X * 100000.0f + 50000.0f,
            Cell.Y * 100000.0f + 50000.0f,
            Cell.Z * 100000.0f + 50000.0f
        );

        if (FVector::DistSquared(CellCenter, ViewerLocation) > MaxDistanceSq)
        {
            for (auto& MeshPair : Data.MeshComponents)
            {
                if (MeshPair.Value)
                {
                    MeshPair.Value->DestroyComponent();
                }
            }

            CellsToRemove.Add(Cell);
        }
    }

    for (const FIntVector& Cell : CellsToRemove)
    {
        CellDataMap.Remove(Cell);
    }
}