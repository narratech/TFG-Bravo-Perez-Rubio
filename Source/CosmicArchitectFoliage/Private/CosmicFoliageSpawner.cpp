#include "CosmicFoliageSpawner.h"
#include "Engine/World.h"
#include "ICosmicNoiseStrategy.h"
#include "CosmicFoliageCollection.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"


FORCEINLINE int32 GetIndexFromLayer(ECosmicFoliageLayer Layer)
{
    switch (Layer)
    {
    case ECosmicFoliageLayer::Near:   return 0;
    case ECosmicFoliageLayer::Medium: return 1;
    case ECosmicFoliageLayer::Far:    return 2;
    default: return 0;
    }
}

FORCEINLINE ECosmicFoliageLayer GetLayerFromIndex(int32 Index)
{
    switch (Index)
    {
    case 0:   return ECosmicFoliageLayer::Near;
    case 1:   return ECosmicFoliageLayer::Medium;
    case 2:   return ECosmicFoliageLayer::Far;
    default: return ECosmicFoliageLayer::Near;
    }
}

// COMPONENTE

UCosmicFoliageSpawner::UCosmicFoliageSpawner()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCosmicFoliageSpawner::InitFoliageSpawner(float RadiusKm)
{
    RandomStream.Initialize(0);
    Octree.Initialize(RadiusKm * 100000, 16); // 16 niveles de profundidad

    if (FoliageCollection)
    {
        FoliageCollection->OnFoliageCollectionChanged.RemoveAll(this);
        FoliageCollection->OnFoliageCollectionChanged.AddUObject(
            this,
            &UCosmicFoliageSpawner::ClearFoliage
        );
    }
}

void UCosmicFoliageSpawner::UpdateFoliageSpawner(float DeltaTime, const FVector& ViewerLocation, const FVector& PlanetCenter, double PlanetRadius, double DistanceToSurface, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy)
{
    ElapsedTime += DeltaTime;
    if (ElapsedTime < UpdateInterval) return;
    ElapsedTime = 0.0f;

    //double TotalStartTime = FPlatformTime::Seconds();

    // --- OCTREE + GENERACIÓN ---
    //double OctreeStartTime = FPlatformTime::Seconds();

    UpdateOctreeAndGenerate(ViewerLocation, DistanceToSurface, PlanetCenter, PlanetRadius, NoiseGenerationStrategy);

    //double OctreeEndTime = FPlatformTime::Seconds();


    // --- COMPLETAR TASKS ---
    //double TasksStartTime = FPlatformTime::Seconds();

    UpdateFoliageGeneration();

    //double TasksEndTime = FPlatformTime::Seconds();


    // --- APLICAR INSTANCIAS ---
    //double ApplyStartTime = FPlatformTime::Seconds();

    ProcessApplyQueue();

    //double ApplyEndTime = FPlatformTime::Seconds();


    //double DeleteStartTime = FPlatformTime::Seconds();

    ProcessDeactivationQueue();

    //double DeleteEndTime = FPlatformTime::Seconds();

    // --- TOTAL ---
    //double TotalEndTime = FPlatformTime::Seconds();

    /*if ((TotalEndTime - TotalStartTime) * 1000.0 > 1.0f) {
        UE_LOG(LogTemp, Warning, TEXT("FOLIAGE TIMING: Total=%.4f ms | Octree=%.4f | Tasks=%.4f | Apply=%.4f | Deactivate=%.4f"),
            (TotalEndTime - TotalStartTime) * 1000.0,
            (OctreeEndTime - OctreeStartTime) * 1000.0,
            (TasksEndTime - TasksStartTime) * 1000.0,
            (ApplyEndTime - ApplyStartTime) * 1000.0,
            (DeleteStartTime - DeleteEndTime) * 1000.0
        );
    }*/
    // LOG
   
    

    //UE_LOG(LogTemp, Warning, TEXT("Tareas Activas: %d, Pendientes: %d"), ActiveCells.Num(), PendingCells.Num());

    // Debug: Dibujar celdas
    DrawDebugCells(PlanetCenter, PlanetRadius);
}

void UCosmicFoliageSpawner::CancelAsyncWork()
{
    for (int32 Layer = 0; Layer < 3; Layer++)
    {
        for (FAsyncTask<FFoliageGenerationTask>* Task : ActiveTasks[Layer])
        {
            if (!Task) continue;

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

        ActiveTasks[Layer].Empty();
    }
}

void UCosmicFoliageSpawner::ClearFoliage()
{
    CancelAsyncWork();

    // 1. Destruir componentes activos
    for (int32 i = 0; i < 3; i++)
    {
        for (auto& CellPair : LayerCells[i].ActiveCells)
        {
            for (auto& MeshPair : CellPair.Value.MeshComponents)
            {
                if (MeshPair.Value) MeshPair.Value->DestroyComponent();
            }
        }
        LayerCells[i].ActiveCells.Empty();
        PendingCells[i].Empty();
        ApplyQueues[i].Empty();
        PendingDeactivation[i].Empty();
        CurrentVisibleCells[i].Empty();
    }

    // 2. Destruir componentes libres del Pool
    for (auto& Pair : FreeHISMPool)
    {
        for (UHierarchicalInstancedStaticMeshComponent* Comp : Pair.Value.Components)
        {
            if (Comp) Comp->DestroyComponent();
        }
    }
    FreeHISMPool.Empty();
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

void UCosmicFoliageSpawner::DrawDebugCells(const FVector& PlanetCenter, double PlanetRadius)
{
    if (!bDrawDebugCells)
        return;

    // Dibujar todas las celdas activas
    //for (const auto& Pair : ActiveCells)
    //{
    //    const FCubeMapCell& Cell = Pair.Key;

    //    // Obtener vértices de la celda
    //    TArray<FVector> Vertices = Octree.GetDebugVertices(Cell);

    //    // Dibujar líneas
    //    for (int32 i = 0; i < Vertices.Num(); i += 2)
    //    {
    //        DrawDebugLine(
    //            GetWorld(),
    //            PlanetCenter + Vertices[i],
    //            PlanetCenter + Vertices[i + 1],
    //            FColor::Green,
    //            false,
    //            UpdateInterval,
    //            0,
    //            DebugCellThickness
    //        );
    //    }

    //    //// Dibujar un punto en el centro de la celda
    //    //FVector Center = PlanetCenter + Octree.GetNodeCenter(Cell) * PlanetRadius;
    //    //DrawDebugPoint(
    //    //    GetWorld(),
    //    //    Center,
    //    //    10.0f,
    //    //    FColor::Red,
    //    //    false,
    //    //    -1
    //    //);

    //    //// Dibujar texto con información de la celda
    //    //DrawDebugString(
    //    //    GetWorld(),
    //    //    Center,
    //    //    Cell.ToString(),
    //    //    nullptr,
    //    //    FColor::White,
    //    //    -1
    //    //);
    //}
}

void UCosmicFoliageSpawner::UpdateOctreeAndGenerate(const FVector& ViewerLocation, double DistanceToSurface, const FVector& PlanetCenter, double PlanetRadius, TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy)
{
    double TotalStart = FPlatformTime::Seconds();

    if (!FoliageCollection)
        return;

    // ------------------ UNIQUE LAYERS ------------------
    double UniqueStart = FPlatformTime::Seconds();

    TSet<ECosmicFoliageLayer> UniqueLayers;

    for (const auto& FoliageEntry : FoliageCollection->FoliageEntries)
    {
        for (const auto& FoliageInstance : FoliageEntry.Foliage)
        {
            UniqueLayers.Add(FoliageInstance.FoliageLayer);
        }
    }

    double UniqueEnd = FPlatformTime::Seconds();


    // ------------------ POR LAYER ------------------
    double LayersTotalTime = 0.0;

    for (size_t i = 0; i < 3; i++)
    {
        //double LayerStart = FPlatformTime::Seconds();

        ECosmicFoliageLayer CurrentLayer = GetLayerFromIndex(i);

        if (!UniqueLayers.Find(CurrentLayer)) continue;

        // -------- OCTREE QUERY --------
        //double OctreeStart = FPlatformTime::Seconds();

        TArray<FCubeMapCell> VisibleNodes;
        if (DistanceToSurface < GetLayerRadius(CurrentLayer) * 100000)
        {
            Octree.GetNodesInRadius(ViewerLocation, PlanetCenter, GetLayerRadius(CurrentLayer), VisibleNodes);
        }

        //double OctreeEnd = FPlatformTime::Seconds();

        // -------- SET BUILD --------
        //double SetStart = FPlatformTime::Seconds();

        TSet<FCubeMapCell> VisibleSet(VisibleNodes);
        CurrentVisibleCells[i] = VisibleSet;

        //double SetEnd = FPlatformTime::Seconds();

        // -------- ACTIVATE --------
        //double ActivateStart = FPlatformTime::Seconds();

        for (const FCubeMapCell& Node : VisibleNodes)
        {
            if (!LayerCells[i].ActiveCells.Contains(Node) && !PendingCells[i].Contains(Node))
            {
                PendingCells[i].Add(Node);
                GenerateCellFoliage(Node, PlanetCenter, PlanetRadius, CurrentLayer, NoiseGenerationStrategy);
            }
        }

        //double ActivateEnd = FPlatformTime::Seconds();

        // -------- DEACTIVATE --------
        //double DeactivateStart = FPlatformTime::Seconds();

        TArray<FCubeMapCell> ToRemove;

        for (const auto& Pair : LayerCells[i].ActiveCells)
        {
            if (!VisibleSet.Contains(Pair.Key))
            {
                PendingDeactivation[i].Add(Pair.Key);
                ToRemove.Add(Pair.Key);
            }
        }

        for (const FCubeMapCell& Node : ToRemove)
        {
            //LayerCells[i].ActiveCells.Remove(Node);
            PendingCells[i].Remove(Node);
        }

        //double DeactivateEnd = FPlatformTime::Seconds();

        //double LayerEnd = FPlatformTime::Seconds();

        //LayersTotalTime += (LayerEnd - LayerStart);

        /*UE_LOG(LogTemp, Warning, TEXT(
            "Layer %d | Total=%.3f ms | Octree=%.3f | Set=%.3f | Activate=%.3f | Deactivate=%.3f | Visible=%d"),
            i,
            (LayerEnd - LayerStart) * 1000.0,
            (OctreeEnd - OctreeStart) * 1000.0,
            (SetEnd - SetStart) * 1000.0,
            (ActivateEnd - ActivateStart) * 1000.0,
            (DeactivateEnd - DeactivateStart) * 1000.0,
            VisibleNodes.Num()
        );*/
    }

    // ------------------ TOTAL ------------------
    //double TotalEnd = FPlatformTime::Seconds();

    /*UE_LOG(LogTemp, Warning, TEXT(
        "UpdateOctree TOTAL=%.3f ms | UniqueLayers=%.3f | Layers=%.3f"),
        (TotalEnd - TotalStart) * 1000.0,
        (UniqueEnd - UniqueStart) * 1000.0,
        LayersTotalTime * 1000.0
    );*/
}

void UCosmicFoliageSpawner::GenerateCellFoliage(
    const FCubeMapCell& Cell,
    const FVector& PlanetCenter,
    double PlanetRadius,
    ECosmicFoliageLayer Layer,
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy)
{
    if (!FoliageCollection)
        return;

    float CellAreaKm2 = Octree.GetNodeAreaKm2(Cell);

    FAsyncTask<FFoliageGenerationTask>* Task =
        new FAsyncTask<FFoliageGenerationTask>(
            Cell,
            Layer,
            FoliageCollection,
            PlanetCenter,
            PlanetRadius,
            NoiseGenerationStrategy,
            CellAreaKm2
        );

    Task->StartBackgroundTask();

    ActiveTasks[GetIndexFromLayer(Layer)].Add(MoveTemp(Task));

    UE_LOG(LogTemp, Verbose, TEXT("Generando foliage para celda: %s"), *Cell.ToString());
}

void UCosmicFoliageSpawner::ClearDelegates()
{
    if (FoliageCollection)
    {
        FoliageCollection->OnFoliageCollectionChanged.RemoveAll(this);
    }
}

void UCosmicFoliageSpawner::UpdateFoliageGeneration()
{
    for (int32 Layer = 0; Layer < 3; Layer++)
    {
        for (int32 i = ActiveTasks[Layer].Num() - 1; i >= 0; i--)
        {
            FAsyncTask<FFoliageGenerationTask>* Task = ActiveTasks[Layer][i];

            if (Task->IsDone())
            {
                FFoliageGenerationTask& CompletedTask = Task->GetTask();
                const FCubeMapCell& Cell = CompletedTask.Cell;

                if (PendingCells[Layer].Contains(Cell))
                {
                    FPendingApplyCell Pending;
                    Pending.Cell = Cell;
                    Pending.Layer = static_cast<ECosmicFoliageLayer>(Layer);
                    Pending.Instances = MoveTemp(CompletedTask.ResultInstances);

                    ApplyQueues[Layer].Add(MoveTemp(Pending));
                }

                delete Task;
                ActiveTasks[Layer].RemoveAt(i);
            }
        }
    }
}

void UCosmicFoliageSpawner::ProcessApplyQueue()
{
    int32 RemainingBudget = MaxInstancesPerFrame;

    // PRIORIDAD: Near Medium Far
    for (int32 Layer = 0; Layer < 3; Layer++)
    {
        auto& Queue = ApplyQueues[Layer];

        for (int32 i = 0; i < Queue.Num(); )
        {
            FPendingApplyCell& Pending = Queue[i];

            // Si ya no es necesaria descartar
            if (!CurrentVisibleCells[Layer].Contains(Pending.Cell))
            {
                PendingCells[Layer].Remove(Pending.Cell); // limpieza extra
                Queue.RemoveAt(i);
                continue;
            }

            int32 NumInstances = Pending.Instances.Num();

            // Si no hay presupuesto y no hemos empezado salir
            if (RemainingBudget <= 0)
            {
                return;
            }

            ApplyGeneratedInstances(Pending.Cell, Pending.Layer, Pending.Instances);

            PendingCells[Layer].Remove(Pending.Cell);

            RemainingBudget -= NumInstances;

            Queue.RemoveAt(i);
        }
    }
}

void UCosmicFoliageSpawner::ProcessDeactivationQueue()
{
    int32 Budget = MaxInstancesPerFrame;

    for (int32 Layer = 0; Layer < 3; Layer++)
    {
        auto& Queue = PendingDeactivation[Layer];

        for (int32 i = 0; i < Queue.Num(); )
        {
            const FCubeMapCell& Cell = Queue[i];

            if (!LayerCells[Layer].ActiveCells.Contains(Cell))
            {
                Queue.RemoveAt(i);
                continue;
            }

            FCosmicFoliageCellData& CellData = LayerCells[Layer].ActiveCells[Cell];

            int32 InstanceCount = 0;

            for (auto& MeshPair : CellData.MeshComponents)
            {
                if (MeshPair.Value)
                {
                    InstanceCount += MeshPair.Value->GetInstanceCount();

                    // ¡En lugar de destruirlo, lo reciclamos devolviéndolo al pool!
                    ReleaseHISM(MeshPair.Key, MeshPair.Value);
                }
            }

            LayerCells[Layer].ActiveCells.Remove(Cell);
            Budget -= InstanceCount;
            Queue.RemoveAt(i);

            if (Budget <= 0)
            {
                return;
            }
        }
    }
}


float UCosmicFoliageSpawner::GetLayerRadius(ECosmicFoliageLayer Layer) const
{
    switch (Layer)
    {
    case ECosmicFoliageLayer::Near:   return NearLayerRadiusKm;
    case ECosmicFoliageLayer::Medium: return MediumLayerRadiusKm;
    case ECosmicFoliageLayer::Far:    return FarLayerRadiusKm;
    default: return 0.0f;
    }
}

FColor UCosmicFoliageSpawner::GetLayerColor(ECosmicFoliageLayer Layer) const
{
    switch (Layer)
    {
    case ECosmicFoliageLayer::Near:   return FColor::Red;
    case ECosmicFoliageLayer::Medium: return FColor::Yellow;
    case ECosmicFoliageLayer::Far:    return FColor::Green;
    default: return FColor::White;
    }
}

void UCosmicFoliageSpawner::ApplyGeneratedInstances(
    const FCubeMapCell& Cell,
    ECosmicFoliageLayer Layer,
    const TArray<FCosmicFoliageInstance>& Instances)
{
    if (Instances.Num() == 0) return;

    FCosmicFoliageCellData& CellData = LayerCells[GetIndexFromLayer(Layer)].ActiveCells.FindOrAdd(Cell);

    TMap<UStaticMesh*, TArray<FTransform>> Batch;

    for (const FCosmicFoliageInstance& Inst : Instances)
    {
        if (!Inst.Mesh) continue;
        Batch.FindOrAdd(Inst.Mesh).Add(Inst.Transform);
    }

    for (auto& Pair : Batch)
    {
        UStaticMesh* Mesh = Pair.Key;
        TArray<FTransform>& Transforms = Pair.Value;

        // Adquirimos un componente exclusivo para esta celda
        UHierarchicalInstancedStaticMeshComponent* Comp = AcquireHISM(Mesh);

        if (!Comp) continue;

        Comp->SetVisibility(true);
        Comp->AddInstances(Transforms, false);

        // Guardamos la referencia en el CellData
        CellData.MeshComponents.Add(Mesh, Comp);
    }
}

UHierarchicalInstancedStaticMeshComponent* UCosmicFoliageSpawner::AcquireHISM(UStaticMesh* Mesh)
{
    if (!Mesh) return nullptr;

    FCosmicHISMPoolList& PoolList = FreeHISMPool.FindOrAdd(Mesh);

    // Si hay un componente libre en el pool, lo sacamos y lo reutilizamos
    if (PoolList.Components.Num() > 0)
    {
        UHierarchicalInstancedStaticMeshComponent* Comp = PoolList.Components.Pop();
        if (Comp)
        {
            return Comp;
        }
    }

    // Si no hay componentes libres, creamos uno nuevo
    UHierarchicalInstancedStaticMeshComponent* NewComp = NewObject<UHierarchicalInstancedStaticMeshComponent>(GetOwner());
    NewComp->SetStaticMesh(Mesh);
    NewComp->SetupAttachment(GetOwner()->GetRootComponent());
    NewComp->RegisterComponent();

    return NewComp;
}

void UCosmicFoliageSpawner::ReleaseHISM(UStaticMesh* Mesh, UHierarchicalInstancedStaticMeshComponent* Comp)
{
    if (!Comp || !Mesh) return;

    // Vaciamos las instancias de la celda y lo ocultamos
    Comp->ClearInstances();
    Comp->SetVisibility(false);

    // Lo devolvemos al pool para que otra celda lo use en el futuro
    FreeHISMPool.FindOrAdd(Mesh).Components.Add(Comp);
}

