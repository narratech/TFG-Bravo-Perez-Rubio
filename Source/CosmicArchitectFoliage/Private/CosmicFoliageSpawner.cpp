#include "CosmicFoliageSpawner.h"
#include "Engine/World.h"
#include "CosmicNoiseSettings.h"
#include "CosmicFoliageCollection.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"



// COMPONENTE

UCosmicFoliageSpawner::UCosmicFoliageSpawner()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCosmicFoliageSpawner::InitFoliageSpawner(float RadiusKm)
{
    RandomStream.Initialize(0);
    Octree.Initialize(RadiusKm * 100000, 8); // 8 niveles de profundidad
}

void UCosmicFoliageSpawner::UpdateFoliageSpawner(float DeltaTime, const FVector& ViewerLocation, const FVector& PlanetCenter, float PlanetRadius, float DistanceToSurface, UCosmicNoiseSettings* NoiseSettings)
{
    ElapsedTime += DeltaTime;
    if (ElapsedTime < UpdateInterval) return;
    ElapsedTime = 0.0f;

    // Actualizar octree y generar foliage
    UpdateOctreeAndGenerate(ViewerLocation, DistanceToSurface, PlanetCenter, PlanetRadius, NoiseSettings);
    UpdateFoliageGeneration(DeltaTime, PlanetCenter, PlanetRadius, NoiseSettings);

    // Debug: Dibujar celdas
    DrawDebugCells(PlanetCenter, PlanetRadius);
}

void UCosmicFoliageSpawner::CancelAsyncWork()
{
    for (auto& Task : ActiveTasks)
    {
        if (Task->Cancel() || Task->IsDone())
        {
            delete Task;
        }
        else
        {
            Task->EnsureCompletion();
            delete Task;
        }
    }

    //UE_LOG(LogTemp, Warning, TEXT("Eliminando tareas de celda: %d"), ActiveTasks.Num());

    ActiveTasks.Empty();
}

void UCosmicFoliageSpawner::ClearFoliage()
{
    CancelAsyncWork();

    // Destruir todos los componentes instanciados
    for (auto& Pair : ActiveCells)
    {
        FCosmicFoliageCellData& CellData = Pair.Value;

        for (auto& MeshPair : CellData.MeshComponents)
        {
            if (MeshPair.Value)
            {
                MeshPair.Value->ClearInstances();   // opcional pero limpio
                MeshPair.Value->DestroyComponent(); // esto es lo importante
            }
        }

        CellData.MeshComponents.Empty();
    }

    // Limpiar todas las celdas activas
    ActiveCells.Empty();

    // Limpiar pendientes 
    PendingCells.Empty();

    UE_LOG(LogTemp, Warning, TEXT("Foliage completamente limpiado"));
}

void UCosmicFoliageSpawner::BeginDestroy()
{
    CancelAsyncWork();
    ClearDelegates();
    Super::BeginDestroy();
}

void UCosmicFoliageSpawner::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    CancelAsyncWork();
    ClearDelegates();
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UCosmicFoliageSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelAsyncWork();
    ClearDelegates();
    Super::EndPlay(EndPlayReason);
}



void UCosmicFoliageSpawner::PreEditChange(FProperty* PropertyAboutToChange)
{
    Super::PreEditChange(PropertyAboutToChange);

    FName PropertyName = PropertyAboutToChange
        ? PropertyAboutToChange->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicFoliageSpawner, FoliageCollection))
    {
        if (FoliageCollection)
        {
            FoliageCollection->OnFoliageCollectionChanged.RemoveAll(this);
        }
        
        ClearFoliage();
        return;
    }
}

void UCosmicFoliageSpawner::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicFoliageSpawner, FoliageCollection))
    {
        if (FoliageCollection)
        {
            FoliageCollection->OnFoliageCollectionChanged.AddUObject(
                this,
                &UCosmicFoliageSpawner::ClearFoliage
            );
        }
        return;
    }
}

void UCosmicFoliageSpawner::DrawDebugCells(const FVector& PlanetCenter, float PlanetRadius)
{
    if (!bDrawDebugCells)
        return;

    // Dibujar todas las celdas activas
    for (const auto& Pair : ActiveCells)
    {
        const FCubeMapCell& Cell = Pair.Key;

        // Obtener vértices de la celda
        TArray<FVector> Vertices = Octree.GetDebugVertices(Cell);

        // Dibujar líneas
        for (int32 i = 0; i < Vertices.Num(); i += 2)
        {
            DrawDebugLine(
                GetWorld(),
                PlanetCenter + Vertices[i],
                PlanetCenter + Vertices[i + 1],
                DebugCellColor,
                false,
                -1,
                0,
                DebugCellThickness
            );
        }

        // Dibujar un punto en el centro de la celda
        FVector Center = PlanetCenter + Octree.GetNodeCenter(Cell) * PlanetRadius;
        DrawDebugPoint(
            GetWorld(),
            Center,
            10.0f,
            FColor::Red,
            false,
            -1
        );

        // Dibujar texto con información de la celda
        DrawDebugString(
            GetWorld(),
            Center,
            Cell.ToString(),
            nullptr,
            FColor::White,
            -1
        );
    }
}

void UCosmicFoliageSpawner::UpdateOctreeAndGenerate(const FVector& ViewerLocation, float DistanceToSurface, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings)
{
    if (!FoliageCollection)
        return;

    // Obtener nodos dentro del radio de visión
    TArray<FCubeMapCell> VisibleNodes;
    if (DistanceToSurface < ViewDistanceKm * 100000) {
        Octree.GetNodesInRadius(ViewerLocation, PlanetCenter, ViewDistanceKm, VisibleNodes);
    }

    TSet<FCubeMapCell> VisibleSet(VisibleNodes);

    // Activar nuevas
    for (const FCubeMapCell& Node : VisibleNodes)
    {
        if (!ActiveCells.Contains(Node) && !PendingCells.Contains(Node))
        {
            PendingCells.Add(Node);
            GenerateCellFoliage(Node, PlanetCenter, PlanetRadius, NoiseSettings);
        }
    }

    // Desactivar antiguas
    TArray<FCubeMapCell> ToRemove;

    for (const auto& Pair : ActiveCells)
    {
        if (!VisibleSet.Contains(Pair.Key))
        {
            for (auto& MeshPair : Pair.Value.MeshComponents)
            {
                if (MeshPair.Value)
                {
                    MeshPair.Value->DestroyComponent();
                }
            }

            ToRemove.Add(Pair.Key);

            UE_LOG(LogTemp, Log, TEXT("Desactivando celda: %s"), *Pair.Key.ToString());
        }
    }

    for (const FCubeMapCell& Node : ToRemove)
    {
        ActiveCells.Remove(Node);
    }
}

void UCosmicFoliageSpawner::GenerateCellFoliage(
    const FCubeMapCell& Cell,
    const FVector& PlanetCenter,
    float PlanetRadius,
    UCosmicNoiseSettings* NoiseSettings)
{
    if (!FoliageCollection)
        return;

    FCosmicNoiseGenerationParameters Params;
    if (NoiseSettings) {
        Params = NoiseSettings->Params;
    }

    float CellAreaKm2 = Octree.GetNodeAreaKm2(Cell);

    FAsyncTask<FFoliageGenerationTask>* Task =
        new FAsyncTask<FFoliageGenerationTask>(
            Cell,
            FoliageCollection,
            PlanetCenter,
            PlanetRadius,
            Params,
            CellAreaKm2
        );

    Task->StartBackgroundTask();

    ActiveTasks.Add(MoveTemp(Task));

    UE_LOG(LogTemp, Verbose, TEXT("Generando foliage para celda: %s"), *Cell.ToString());
}

void UCosmicFoliageSpawner::ClearDelegates()
{
    if (FoliageCollection)
    {
        FoliageCollection->OnFoliageCollectionChanged.RemoveAll(this);
    }
}

void UCosmicFoliageSpawner::UpdateFoliageGeneration(
    float DeltaTime,
    const FVector& PlanetCenter,
    float PlanetRadius,
    UCosmicNoiseSettings* NoiseSettings)
{
    for (int32 i = ActiveTasks.Num() - 1; i >= 0; i--)
    {
        FAsyncTask<FFoliageGenerationTask>* Task = ActiveTasks[i];

        if (Task->IsDone())
        {
            FFoliageGenerationTask& CompletedTask = Task->GetTask();
            const FCubeMapCell& Cell = CompletedTask.Cell;

            if (PendingCells.Contains(Cell))
            {
                ApplyGeneratedInstances(Cell, CompletedTask.ResultInstances);
                PendingCells.Remove(Cell);
            }

            // remove
            delete Task;
            ActiveTasks.RemoveAt(i);
        }
    }
}


void UCosmicFoliageSpawner::ApplyGeneratedInstances(
    const FCubeMapCell& Cell,
    const TArray<FCosmicFoliageInstance>& Instances)
{
    if (Instances.Num() == 0)
        return;

    FCosmicFoliageCellData& CellData = ActiveCells.FindOrAdd(Cell);

    TMap<UStaticMesh*, TArray<FTransform>> Batch;

    TMap<UStaticMesh*, bool> MeshCollisionCache;

    for (const auto& Entry : FoliageCollection->FoliageEntries)
    {
        for (const auto& FoliageInst : Entry.Foliage)
        {
            MeshCollisionCache.Add(FoliageInst.Mesh, FoliageInst.bHasCollision);
        }
    }

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

        const bool HasCollision = MeshCollisionCache.FindRef(Mesh);

        Comp->SetCollisionEnabled(
            HasCollision
            ? ECollisionEnabled::QueryAndPhysics
            : ECollisionEnabled::NoCollision
        );

        Comp->AddInstances(Transforms, false);
    }

    CellData.LastUpdateTime = GetWorld()->GetTimeSeconds();
}

UHierarchicalInstancedStaticMeshComponent*
UCosmicFoliageSpawner::GetOrCreateCellComponent(
    FCosmicFoliageCellData& CellData,
    UStaticMesh* Mesh)
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

