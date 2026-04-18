#include "CosmicFoliageSpawner.h"
#include "Engine/World.h"
#include "CosmicNoiseSettings.h"
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

void UCosmicFoliageSpawner::UpdateFoliageSpawner(float DeltaTime, const FVector& ViewerLocation, const FVector& PlanetCenter, float PlanetRadius, float DistanceToSurface, UCosmicNoiseSettings* NoiseSettings)
{
    ElapsedTime += DeltaTime;
    if (ElapsedTime < UpdateInterval) return;
    ElapsedTime = 0.0f;

    // Actualizar octree y generar foliage
    UpdateOctreeAndGenerate(ViewerLocation, DistanceToSurface, PlanetCenter, PlanetRadius, NoiseSettings);
    UpdateFoliageGeneration(DeltaTime, PlanetCenter, PlanetRadius, NoiseSettings);

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

    for (int32 i = 0; i < 3; i++)
    {
        // Destroy components
        for (auto& Pair : LayerCells[i].ActiveCells)
        {
            FCosmicFoliageCellData& CellData = Pair.Value;

            for (auto& MeshPair : CellData.MeshComponents)
            {
                if (MeshPair.Value)
                {
                    MeshPair.Value->DestroyComponent();
                }
            }

            CellData.MeshComponents.Empty();
        }

        LayerCells[i].ActiveCells.Empty();
        PendingCells[i].Empty();
    }
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

void UCosmicFoliageSpawner::UpdateOctreeAndGenerate(const FVector& ViewerLocation, float DistanceToSurface, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings)
{
    if (!FoliageCollection)
        return;

    double CreateStartTime = FPlatformTime::Seconds();

    // Obtener nodos dentro del radio de visión
    for (size_t i = 0; i < 3; i++)
    {
        ECosmicFoliageLayer CurrentLayer = GetLayerFromIndex(i);

        TArray<FCubeMapCell> VisibleNodes;
        if (DistanceToSurface < GetLayerRadius(CurrentLayer) * 100000) {         
            Octree.GetNodesInRadius(ViewerLocation, PlanetCenter, GetLayerRadius(CurrentLayer), VisibleNodes);
        }

        TSet<FCubeMapCell> VisibleSet(VisibleNodes);

        // Activar nuevas
        for (const FCubeMapCell& Node : VisibleNodes)
        {
            if (!LayerCells[i].ActiveCells.Contains(Node) && !PendingCells[i].Contains(Node))
            {
                PendingCells[i].Add(Node);
                GenerateCellFoliage(Node, PlanetCenter, PlanetRadius, CurrentLayer, NoiseSettings);
            }
        }

        // Desactivar antiguas
        TArray<FCubeMapCell> ToRemove;

        for (const auto& Pair : LayerCells[i].ActiveCells)
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
            LayerCells[i].ActiveCells.Remove(Node);
        }
    } 

    double CreateEndTime = FPlatformTime::Seconds();
    if (bDrawDebugCells) {
        UE_LOG(LogTemp, Warning, TEXT("Actualizar octree y generar celdas tomo: %.4f ms"), (CreateEndTime - CreateStartTime) * 1000.0);
    }
}

void UCosmicFoliageSpawner::GenerateCellFoliage(
    const FCubeMapCell& Cell,
    const FVector& PlanetCenter,
    float PlanetRadius,
    ECosmicFoliageLayer Layer,
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
            Layer,
            FoliageCollection,
            PlanetCenter,
            PlanetRadius,
            Params,
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

void UCosmicFoliageSpawner::UpdateFoliageGeneration(
    float DeltaTime,
    const FVector& PlanetCenter,
    float PlanetRadius,
    UCosmicNoiseSettings* NoiseSettings)
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
                    ApplyGeneratedInstances(Cell, static_cast<ECosmicFoliageLayer>(Layer), CompletedTask.ResultInstances);
                    PendingCells[Layer].Remove(Cell);
                }

                delete Task;
                ActiveTasks[Layer].RemoveAt(i);
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
    if (Instances.Num() == 0)
        return;

    FCosmicFoliageCellData& CellData =
        LayerCells[GetIndexFromLayer(Layer)].ActiveCells.FindOrAdd(Cell);

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

        UHierarchicalInstancedStaticMeshComponent* Comp =
            GetOrCreateCellComponent(CellData, Mesh);

        if (!Comp) continue;

        Comp->AddInstances(Transforms, false);
    }
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

