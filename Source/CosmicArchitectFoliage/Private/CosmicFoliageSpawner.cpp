// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
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
    // El clipmap llama explicitamente a UpdateFoliageSpawner; registrar otro
    // tick de componente no aporta trabajo y si tiene coste de scheduling.
    PrimaryComponentTick.bCanEverTick = false;
    FoliageLayerPriority = {
        ECosmicFoliageLayer::Far,
        ECosmicFoliageLayer::Medium,
        ECosmicFoliageLayer::Near
    };
}

void UCosmicFoliageSpawner::InitFoliageSpawner(float RadiusKm)
{
    Octree.Initialize(RadiusKm * 100000, 16); // 16 niveles de profundidad
    ClearFoliage();

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
    UpdateFoliageGeneration();
    UpdateOctreeAndGenerate(ViewerLocation, DistanceToSurface, PlanetCenter);

    const FVector ViewerDir = (ViewerLocation - PlanetCenter).GetSafeNormal();
    StartQueuedGenerationTasks(ViewerDir, PlanetRadius, NoiseGenerationStrategy);

    int32 RemainingDeactivationBudget = FMath::Max(1, MaxDeactivationInstancesPerFrame);
    int32 RemainingApplyBudget = FMath::Max(1, MaxInstancesPerFrame);

    ProcessDeactivationQueue(RemainingDeactivationBudget);
    ProcessApplyQueue(ViewerDir, RemainingApplyBudget);
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

    for (int32 i = 0; i < 3; i++)
    {
        LayerCells[i].ActiveCells.Empty();
        PendingCells[i].Empty();
        ApplyQueues[i].Empty();
        PendingDeactivation[i].Empty();
        QueuedCells[i].Empty();
        PendingDeactivationCells[i].Empty();
        CellsBeingDeactivated[i].Empty();
        CurrentVisibleCells[i].Empty();
        bVisibilityQueryValid[i] = false;
        bLayerWasEnabled[i] = false;
        LastVisibilityRadiusKm[i] = 0.0f;
    }

    // Cada malla/estado de colision tiene un unico HISM compartido.
    for (auto& Pair : SharedHISMs)
    {
        if (Pair.Value.Component)
        {
            Pair.Value.Component->DestroyComponent();
        }
    }
    SharedHISMs.Empty();
    FoliageEntriesSnapshot.Reset();
    ConfiguredLayerMask = 0;
    bLayerMaskDirty = true;
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


#if WITH_EDITOR
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
#endif

void UCosmicFoliageSpawner::RefreshConfiguredLayerMask()
{
    ConfiguredLayerMask = 0;
    FoliageEntriesSnapshot.Reset();
    if (FoliageCollection)
    {
        FoliageEntriesSnapshot =
            MakeShared<TArray<FCosmicFoliageCollectionEntry>, ESPMode::ThreadSafe>(
                FoliageCollection->FoliageEntries);

        for (const FCosmicFoliageCollectionEntry& Entry : *FoliageEntriesSnapshot)
        {
            for (const FCosmicFoliageMesh& Mesh : Entry.Foliage)
            {
                if (Mesh.Mesh)
                {
                    ConfiguredLayerMask |= 1 << GetIndexFromLayer(Mesh.FoliageLayer);
                }
            }
        }
    }
    bLayerMaskDirty = false;
}

int32 UCosmicFoliageSpawner::GetActiveTaskCount() const
{
    return ActiveTasks[0].Num() + ActiveTasks[1].Num() + ActiveTasks[2].Num();
}

void UCosmicFoliageSpawner::GetLayerPriorityIndices(int32 OutLayerIndices[3]) const
{
    int32 NumLayers = 0;

    auto AddUniqueLayer = [&OutLayerIndices, &NumLayers](ECosmicFoliageLayer Layer)
    {
        int32 LayerIndex = INDEX_NONE;
        switch (Layer)
        {
        case ECosmicFoliageLayer::Near:   LayerIndex = 0; break;
        case ECosmicFoliageLayer::Medium: LayerIndex = 1; break;
        case ECosmicFoliageLayer::Far:    LayerIndex = 2; break;
        default: return;
        }

        for (int32 Index = 0; Index < NumLayers; ++Index)
        {
            if (OutLayerIndices[Index] == LayerIndex)
            {
                return;
            }
        }

        if (NumLayers < 3)
        {
            OutLayerIndices[NumLayers++] = LayerIndex;
        }
    };

    for (const ECosmicFoliageLayer Layer : FoliageLayerPriority)
    {
        AddUniqueLayer(Layer);
    }

    // Fallback por defecto: Far -> Medium -> Near
    AddUniqueLayer(ECosmicFoliageLayer::Far);
    AddUniqueLayer(ECosmicFoliageLayer::Medium);
    AddUniqueLayer(ECosmicFoliageLayer::Near);
}

void UCosmicFoliageSpawner::UpdateOctreeAndGenerate(const FVector& ViewerLocation, double DistanceToSurface, const FVector& PlanetCenter)
{
    if (bLayerMaskDirty)
    {
        RefreshConfiguredLayerMask();
    }

    const FVector ViewerRelativeToPlanet = ViewerLocation - PlanetCenter;

    for (int32 i = 0; i < 3; ++i)
    {
        const ECosmicFoliageLayer CurrentLayer = GetLayerFromIndex(i);
        const float LayerRadiusKm = GetLayerRadius(CurrentLayer);
        const bool bLayerConfigured = (ConfiguredLayerMask & (1 << i)) != 0;
        const bool bLayerEnabled = bLayerConfigured &&
            DistanceToSurface < static_cast<double>(LayerRadiusKm) * 100000.0;
        const double QueryMovementThreshold = FMath::Clamp(
            static_cast<double>(LayerRadiusKm) * 100000.0 * VisibilityUpdateDistanceRatio,
            50.0,
            2000.0);
        const bool bViewerMovedEnough = !LastVisibilityQueryLocation[i].Equals(
            ViewerRelativeToPlanet,
            QueryMovementThreshold);
        const bool bShouldRefreshVisibility =
            !bVisibilityQueryValid[i] ||
            bLayerWasEnabled[i] != bLayerEnabled ||
            !FMath::IsNearlyEqual(LastVisibilityRadiusKm[i], LayerRadiusKm) ||
            bViewerMovedEnough;

        if (!bShouldRefreshVisibility)
        {
            continue;
        }

        TArray<FCubeMapCell> VisibleNodes;
        if (bLayerEnabled)
        {
            Octree.GetNodesInRadius(ViewerLocation, PlanetCenter, LayerRadiusKm, VisibleNodes);
        }

        TSet<FCubeMapCell> VisibleSet(VisibleNodes);

        for (const FCubeMapCell& Node : VisibleNodes)
        {
            if (!LayerCells[i].ActiveCells.Contains(Node) && !PendingCells[i].Contains(Node))
            {
                PendingCells[i].Add(Node);
                QueuedCells[i].Add(FPendingQueuedCell{ Node, Octree.GetNodeCenterDirection(Node) });
            }
        }

        for (const auto& Pair : LayerCells[i].ActiveCells)
        {
            if (!VisibleSet.Contains(Pair.Key) &&
                !PendingDeactivationCells[i].Contains(Pair.Key))
            {
                PendingDeactivation[i].Add(Pair.Key);
                PendingDeactivationCells[i].Add(Pair.Key);
            }
        }

        // Las celdas aun no iniciadas se pueden cancelar sin tocar el thread pool.
        for (int32 QueueIndex = QueuedCells[i].Num() - 1; QueueIndex >= 0; --QueueIndex)
        {
            if (!VisibleSet.Contains(QueuedCells[i][QueueIndex].Cell))
            {
                PendingCells[i].Remove(QueuedCells[i][QueueIndex].Cell);
                QueuedCells[i].RemoveAtSwap(QueueIndex, 1, EAllowShrinking::No);
            }
        }

        CurrentVisibleCells[i] = MoveTemp(VisibleSet);
        LastVisibilityQueryLocation[i] = ViewerRelativeToPlanet;
        LastVisibilityRadiusKm[i] = LayerRadiusKm;
        bVisibilityQueryValid[i] = true;
        bLayerWasEnabled[i] = bLayerEnabled;
    }
}

void UCosmicFoliageSpawner::GenerateCellFoliage(
    const FCubeMapCell& Cell,
    double PlanetRadius,
    ECosmicFoliageLayer Layer,
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy)
{
    if (!FoliageEntriesSnapshot.IsValid())
        return;

    FAsyncTask<FFoliageGenerationTask>* Task =
        new FAsyncTask<FFoliageGenerationTask>(
            Cell,
            Layer,
            FoliageEntriesSnapshot,
            PlanetRadius,
            NoiseGenerationStrategy,
            MaxInstancesPerCell,
            NormalSampleDistanceCm
        );

    Task->StartBackgroundTask();

    ActiveTasks[GetIndexFromLayer(Layer)].Add(MoveTemp(Task));

    UE_LOG(LogTemp, Verbose, TEXT("Generando foliage para celda: %s"), *Cell.ToString());
}

void UCosmicFoliageSpawner::StartQueuedGenerationTasks(
    const FVector& ViewerDir,
    double PlanetRadius,
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy)
{
    if (!FoliageEntriesSnapshot.IsValid() || !NoiseGenerationStrategy.IsValid())
    {
        return;
    }

    int32 AvailableSlots = FMath::Max(1, MaxConcurrentGenerationTasks) - GetActiveTaskCount();
    int32 LayerPriorityIndices[3];
    GetLayerPriorityIndices(LayerPriorityIndices);

    for (const int32 LayerIndex : LayerPriorityIndices)
    {
        if (AvailableSlots <= 0)
        {
            break;
        }

        TArray<FPendingQueuedCell>& Queue = QueuedCells[LayerIndex];
        while (!Queue.IsEmpty() && AvailableSlots > 0)
        {
            int32 BestIndex = INDEX_NONE;
            double BestScore = -MAX_dbl;

            for (int32 Index = 0; Index < Queue.Num(); ++Index)
            {
                const double Score = FVector::DotProduct(ViewerDir, Queue[Index].UnitDirection);
                if (Score > BestScore)
                {
                    BestScore = Score;
                    BestIndex = Index;
                }
            }

            if (BestIndex == INDEX_NONE)
            {
                break;
            }

            const FPendingQueuedCell Selected = Queue[BestIndex];
            Queue.RemoveAtSwap(BestIndex, 1, EAllowShrinking::No);

            if (!PendingCells[LayerIndex].Contains(Selected.Cell) ||
                !CurrentVisibleCells[LayerIndex].Contains(Selected.Cell))
            {
                PendingCells[LayerIndex].Remove(Selected.Cell);
                continue;
            }

            GenerateCellFoliage(
                Selected.Cell,
                PlanetRadius,
                GetLayerFromIndex(LayerIndex),
                NoiseGenerationStrategy);
            --AvailableSlots;
        }
    }
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

                if (PendingCells[Layer].Contains(Cell) &&
                    CurrentVisibleCells[Layer].Contains(Cell))
                {
                    FPendingApplyCell Pending;
                    Pending.Cell = Cell;
                    Pending.Layer = GetLayerFromIndex(Layer);
                    Pending.UnitDirection = Octree.GetNodeCenterDirection(Cell);
                    Pending.Instances = MoveTemp(CompletedTask.ResultInstances);

                    ApplyQueues[Layer].Add(MoveTemp(Pending));
                }
                else
                {
                    PendingCells[Layer].Remove(Cell);
                }

                delete Task;
                ActiveTasks[Layer].RemoveAtSwap(i, 1, EAllowShrinking::No);
            }
        }
    }
}

void UCosmicFoliageSpawner::ProcessApplyQueue(const FVector& ViewerDir, int32& RemainingBudget)
{
    int32 LayerPriorityIndices[3];
    GetLayerPriorityIndices(LayerPriorityIndices);

    for (const int32 Layer : LayerPriorityIndices)
    {
        if (RemainingBudget <= 0)
        {
            return;
        }

        auto& Queue = ApplyQueues[Layer];

        while (!Queue.IsEmpty() && RemainingBudget > 0)
        {
            // Si hay multiples celdas completadas esperando y la primera aun no ha comenzado
            // (NextInstanceIndex == 0), priorizamos la celda mas cercana al jugador.
            if (Queue.Num() > 1 && Queue[0].NextInstanceIndex == 0)
            {
                int32 BestIndex = 0;
                double BestScore = FVector::DotProduct(ViewerDir, Queue[0].UnitDirection);
                for (int32 Index = 1; Index < Queue.Num(); ++Index)
                {
                    if (Queue[Index].NextInstanceIndex == 0)
                    {
                        const double Score = FVector::DotProduct(ViewerDir, Queue[Index].UnitDirection);
                        if (Score > BestScore)
                        {
                            BestScore = Score;
                            BestIndex = Index;
                        }
                    }
                }
                if (BestIndex != 0)
                {
                    Queue.Swap(0, BestIndex);
                }
            }

            FPendingApplyCell& Pending = Queue[0];

            // Si ya no es necesaria descartar
            if (!CurrentVisibleCells[Layer].Contains(Pending.Cell))
            {
                PendingCells[Layer].Remove(Pending.Cell);
                if (LayerCells[Layer].ActiveCells.Contains(Pending.Cell) &&
                    !PendingDeactivationCells[Layer].Contains(Pending.Cell))
                {
                    PendingDeactivation[Layer].Add(Pending.Cell);
                    PendingDeactivationCells[Layer].Add(Pending.Cell);
                }
                Queue.RemoveAtSwap(0, 1, EAllowShrinking::No);
                continue;
            }

            const int32 InstancesLeft = Pending.Instances.Num() - Pending.NextInstanceIndex;
            if (InstancesLeft <= 0)
            {
                // Las celdas vacias tambien se consideran generadas para no
                // relanzar una tarea en cada actualizacion.
                LayerCells[Layer].ActiveCells.FindOrAdd(Pending.Cell);
                PendingCells[Layer].Remove(Pending.Cell);
                Queue.RemoveAtSwap(0, 1, EAllowShrinking::No);
                continue;
            }

            const int32 NumToApply = FMath::Min(InstancesLeft, RemainingBudget);
            ApplyGeneratedInstances(Pending.Cell, Pending.Layer,
                TArrayView<const FCosmicFoliageInstance>(
                    Pending.Instances.GetData() + Pending.NextInstanceIndex,
                    NumToApply));
            Pending.NextInstanceIndex += NumToApply;
            RemainingBudget -= NumToApply;

            if (Pending.NextInstanceIndex >= Pending.Instances.Num())
            {
                PendingCells[Layer].Remove(Pending.Cell);
                Queue.RemoveAtSwap(0, 1, EAllowShrinking::No);
            }
            else
            {
                return;
            }
        }
    }
}

void UCosmicFoliageSpawner::ProcessDeactivationQueue(int32& RemainingBudget)
{
    int32 LayerPriorityIndices[3];
    GetLayerPriorityIndices(LayerPriorityIndices);

    for (const int32 Layer : LayerPriorityIndices)
    {
        if (RemainingBudget <= 0)
        {
            return;
        }

        auto& Queue = PendingDeactivation[Layer];

        for (int32 i = 0; i < Queue.Num(); )
        {
            const FCubeMapCell Cell = Queue[i];

            if (CurrentVisibleCells[Layer].Contains(Cell) &&
                !CellsBeingDeactivated[Layer].Contains(Cell))
            {
                PendingDeactivationCells[Layer].Remove(Cell);
                Queue.RemoveAtSwap(i, 1, EAllowShrinking::No);
                continue;
            }

            if (!LayerCells[Layer].ActiveCells.Contains(Cell))
            {
                CellsBeingDeactivated[Layer].Remove(Cell);
                PendingDeactivationCells[Layer].Remove(Cell);
                Queue.RemoveAtSwap(i, 1, EAllowShrinking::No);
                continue;
            }

            if (RemainingBudget <= 0)
            {
                return;
            }

            const int32 RemovedCount = RemoveCellInstances(Layer, Cell, RemainingBudget);
            RemainingBudget -= RemovedCount;
            if (RemovedCount > 0)
            {
                CellsBeingDeactivated[Layer].Add(Cell);
            }

            if (!LayerCells[Layer].ActiveCells.Contains(Cell))
            {
                CellsBeingDeactivated[Layer].Remove(Cell);
                PendingDeactivationCells[Layer].Remove(Cell);
                Queue.RemoveAtSwap(i, 1, EAllowShrinking::No);

                // Si volvio a entrar durante una retirada parcial, se genera
                // de nuevo una vez concluida para no dejar una celda incompleta.
                if (CurrentVisibleCells[Layer].Contains(Cell) &&
                    !PendingCells[Layer].Contains(Cell))
                {
                    PendingCells[Layer].Add(Cell);
                    QueuedCells[Layer].Add(FPendingQueuedCell{ Cell, Octree.GetNodeCenterDirection(Cell) });
                }
                continue;
            }

            if (RemovedCount == 0)
            {
                // Estado inconsistente: evita bloquear permanentemente la cola.
                UE_LOG(LogTemp, Warning,
                    TEXT("No se pudieron retirar instancias de foliage para %s"),
                    *Cell.ToString());
                Queue.RemoveAtSwap(i, 1, EAllowShrinking::No);
                PendingDeactivationCells[Layer].Remove(Cell);
                CellsBeingDeactivated[Layer].Remove(Cell);
                continue;
            }

            if (RemainingBudget <= 0)
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

void UCosmicFoliageSpawner::ApplyGeneratedInstances(
    const FCubeMapCell& Cell,
    ECosmicFoliageLayer Layer,
    TArrayView<const FCosmicFoliageInstance> Instances)
{
    if (Instances.IsEmpty()) return;

    const int32 LayerIndex = GetIndexFromLayer(Layer);
    FCosmicFoliageCellData& CellData = LayerCells[LayerIndex].ActiveCells.FindOrAdd(Cell);

    TMap<FCosmicHISMKey, TArray<FTransform>> Batch;

    for (const FCosmicFoliageInstance& Inst : Instances)
    {
        if (Inst.HISMKey.Mesh)
        {
            Batch.FindOrAdd(Inst.HISMKey).Add(Inst.Transform);
        }
    }

    for (auto& Pair : Batch)
    {
        const FCosmicHISMKey& Key = Pair.Key;
        TArray<FTransform>& Transforms = Pair.Value;

        FCosmicSharedHISMData* SharedData = GetOrCreateSharedHISM(Key);
        if (!SharedData || !SharedData->Component) continue;

        UHierarchicalInstancedStaticMeshComponent* Component = SharedData->Component;
        if (Component->GetInstanceCount() != SharedData->InstanceOwners.Num() ||
            SharedData->ActiveInstanceCount + SharedData->FreeInstanceIndices.Num() !=
                SharedData->InstanceOwners.Num())
        {
            UE_LOG(LogTemp, Error,
                TEXT("No se pueden anadir instancias: HISM %s desincronizado"),
                *GetNameSafe(Key.Mesh));
            continue;
        }

        Component->SetVisibility(true);
        TArray<int32>& CellIndices = CellData.InstanceIndices.FindOrAdd(Key);
        CellIndices.Reserve(CellIndices.Num() + Transforms.Num());

        int32 TransformIndex = 0;
        while (TransformIndex < Transforms.Num() &&
            !SharedData->FreeInstanceIndices.IsEmpty())
        {
            const int32 InstanceIndex =
                SharedData->FreeInstanceIndices.Pop(EAllowShrinking::No);
            if (!SharedData->InstanceOwners.IsValidIndex(InstanceIndex) ||
                SharedData->InstanceOwners[InstanceIndex].LayerIndex != INDEX_NONE)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("Slot libre invalido en HISM %s"),
                    *GetNameSafe(Key.Mesh));
                continue;
            }

            if (!Component->UpdateInstanceTransform(
                InstanceIndex,
                Transforms[TransformIndex],
                false,
                true,
                true))
            {
                UE_LOG(LogTemp, Error,
                    TEXT("No se pudo reutilizar el slot %d de HISM %s"),
                    InstanceIndex,
                    *GetNameSafe(Key.Mesh));
                SharedData->FreeInstanceIndices.Add(InstanceIndex);
                break;
            }

            FCosmicFoliageInstanceOwner& Owner =
                SharedData->InstanceOwners[InstanceIndex];
            Owner.Cell = Cell;
            Owner.LayerIndex = LayerIndex;
            Owner.CellSlot = CellIndices.Add(InstanceIndex);
            ++SharedData->ActiveInstanceCount;
            ++TransformIndex;
        }

        const int32 NewInstanceCount = Transforms.Num() - TransformIndex;
        if (NewInstanceCount <= 0)
        {
            continue;
        }

        TArray<FTransform> NewTransforms;
        NewTransforms.Append(
            Transforms.GetData() + TransformIndex,
            NewInstanceCount);
        Component->PreAllocateInstancesMemory(NewInstanceCount);
        const int32 FirstExpectedIndex = SharedData->InstanceOwners.Num();
        const TArray<int32> AddedIndices = Component->AddInstances(
            NewTransforms,
            true,
            false,
            false);

        const int32 AddedCount = FMath::Min(AddedIndices.Num(), NewInstanceCount);
        SharedData->InstanceOwners.Reserve(FirstExpectedIndex + AddedCount);
        for (int32 Index = 0; Index < AddedCount; ++Index)
        {
            const int32 InstanceIndex = AddedIndices[Index];
            if (InstanceIndex != FirstExpectedIndex + Index)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("El HISM %s devolvio un indice de alta inesperado"),
                    *GetNameSafe(Key.Mesh));
                break;
            }

            FCosmicFoliageInstanceOwner Owner;
            Owner.Cell = Cell;
            Owner.LayerIndex = LayerIndex;
            Owner.CellSlot = CellIndices.Add(InstanceIndex);
            SharedData->InstanceOwners.Add(Owner);
            ++SharedData->ActiveInstanceCount;
        }
    }
}

FCosmicSharedHISMData* UCosmicFoliageSpawner::GetOrCreateSharedHISM(const FCosmicHISMKey& Key)
{
    if (!Key.Mesh) return nullptr;

    FCosmicSharedHISMData& SharedData = SharedHISMs.FindOrAdd(Key);
    if (IsValid(SharedData.Component))
    {
        return &SharedData;
    }

    UHierarchicalInstancedStaticMeshComponent* NewComp = NewObject<UHierarchicalInstancedStaticMeshComponent>(
        GetOwner(),
        NAME_None,
        RF_Transient | RF_DuplicateTransient  // Marcar como transitorio
    );
    if (!NewComp)
    {
        SharedHISMs.Remove(Key);
        return nullptr;
    }

    NewComp->SetupAttachment(GetOwner()->GetRootComponent());
    NewComp->SetStaticMesh(Key.Mesh);
    NewComp->SetCollisionEnabled(Key.bHasCollision
        ? ECollisionEnabled::QueryAndPhysics
        : ECollisionEnabled::NoCollision);
    NewComp->SetGenerateOverlapEvents(false);
    NewComp->SetCanEverAffectNavigation(false);
    NewComp->SetMobility(EComponentMobility::Movable);
    // Las instancias estan a un radio planetario del origen local. El espacio
    // trasladado mantiene precision de float en culling y en el cluster tree.
    NewComp->bUseTranslatedInstanceSpace = true;
    NewComp->RegisterComponent();
    SharedData.Component = NewComp;
    return &SharedData;
}

int32 UCosmicFoliageSpawner::RemoveCellInstances(
    int32 LayerIndex,
    const FCubeMapCell& Cell,
    int32 InstanceBudget)
{
    FCosmicFoliageCellData* CellData = LayerCells[LayerIndex].ActiveCells.Find(Cell);
    if (!CellData || InstanceBudget <= 0)
    {
        return 0;
    }

    int32 RemovedTotal = 0;

    for (auto MeshIterator = CellData->InstanceIndices.CreateIterator(); MeshIterator; ++MeshIterator)
    {
        if (RemovedTotal >= InstanceBudget)
        {
            break;
        }

        const FCosmicHISMKey Key = MeshIterator.Key();
        TArray<int32>& CellIndices = MeshIterator.Value();
        FCosmicSharedHISMData* SharedData = SharedHISMs.Find(Key);
        if (!SharedData || !IsValid(SharedData->Component))
        {
            const int32 DiscardCount = FMath::Min(
                CellIndices.Num(),
                InstanceBudget - RemovedTotal);
            CellIndices.SetNum(CellIndices.Num() - DiscardCount, EAllowShrinking::No);
            RemovedTotal += DiscardCount;
            if (CellIndices.IsEmpty())
            {
                MeshIterator.RemoveCurrent();
            }
            continue;
        }

        const int32 HideCount = FMath::Min(
            CellIndices.Num(),
            InstanceBudget - RemovedTotal);
        bool bIndicesValid =
            SharedData->Component->GetInstanceCount() == SharedData->InstanceOwners.Num() &&
            SharedData->ActiveInstanceCount + SharedData->FreeInstanceIndices.Num() ==
                SharedData->InstanceOwners.Num() &&
            SharedData->ActiveInstanceCount >= HideCount;
        for (int32 Index = 0; Index < HideCount; ++Index)
        {
            const int32 CellSlot = CellIndices.Num() - 1 - Index;
            const int32 InstanceIndex = CellIndices[CellSlot];

            if (!SharedData->InstanceOwners.IsValidIndex(InstanceIndex))
            {
                bIndicesValid = false;
                continue;
            }

            const FCosmicFoliageInstanceOwner& Owner =
                SharedData->InstanceOwners[InstanceIndex];
            bIndicesValid &=
                Owner.LayerIndex == LayerIndex &&
                Owner.Cell == Cell &&
                Owner.CellSlot == CellSlot;
        }

        if (!bIndicesValid)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Se perdio la correspondencia de indices del HISM compartido %s"),
                *GetNameSafe(Key.Mesh));
            break;
        }

        int32 HiddenCount = 0;
        for (int32 Index = 0; Index < HideCount; ++Index)
        {
            const int32 CellSlot = CellIndices.Num() - 1 - Index;
            const int32 InstanceIndex = CellIndices[CellSlot];
            FTransform HiddenTransform;
            if (!SharedData->Component->GetInstanceTransform(
                InstanceIndex,
                HiddenTransform,
                false))
            {
                break;
            }

            // Mantener la posicion hace que HISM pueda actualizar escala in-place;
            // escala cero tambien elimina inmediatamente el body de colision.
            HiddenTransform.SetScale3D(FVector::ZeroVector);
            if (!SharedData->Component->UpdateInstanceTransform(
                InstanceIndex,
                HiddenTransform,
                false,
                true,
                true))
            {
                break;
            }

            SharedData->InstanceOwners[InstanceIndex] =
                FCosmicFoliageInstanceOwner();
            SharedData->FreeInstanceIndices.Add(InstanceIndex);
            --SharedData->ActiveInstanceCount;
            ++HiddenCount;
        }

        CellIndices.SetNum(CellIndices.Num() - HiddenCount, EAllowShrinking::No);
        RemovedTotal += HiddenCount;

        if (CellIndices.IsEmpty())
        {
            MeshIterator.RemoveCurrent();
        }
        if (SharedData->ActiveInstanceCount == 0)
        {
            SharedData->Component->SetVisibility(false);
        }

        if (HiddenCount < HideCount)
        {
            UE_LOG(LogTemp, Error,
                TEXT("No se pudieron ocultar todos los slots del HISM %s"),
                *GetNameSafe(Key.Mesh));
            break;
        }
    }

    if (CellData->InstanceIndices.IsEmpty())
    {
        LayerCells[LayerIndex].ActiveCells.Remove(Cell);
    }

    return RemovedTotal;
}

