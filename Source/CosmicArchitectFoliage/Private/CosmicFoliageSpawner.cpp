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

void UCosmicFoliageSpawner::InitFoliageSpawner(float RadiusKm, UCosmicNoiseSettings* NoiseSettings)
{
    PlanetRadiusCm = RadiusKm * 100000;
    CurrentNoiseSettings = NoiseSettings;
}

void UCosmicFoliageSpawner::BeginPlay()
{
    Super::BeginPlay();
    RandomStream.Initialize(0);

    Octree.Initialize(PlanetRadiusCm, 8); // 8 niveles de profundidad
}

void UCosmicFoliageSpawner::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ElapsedTime += DeltaTime;
    if (ElapsedTime < UpdateInterval) return;
    ElapsedTime = 0.0f;

    // Obtener posición del jugador
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC || !PC->PlayerCameraManager)
        return;

    AActor* PlanetActor = GetOwner();
    if (!PlanetActor)
        return;

    FVector ViewerLocation = PC->PlayerCameraManager->GetCameraLocation();
    FVector PlanetCenter = PlanetActor->GetActorLocation();

    // Actualizar octree y generar foliage
    UpdateOctreeAndGenerate(ViewerLocation, PlanetCenter, PlanetRadiusCm, CurrentNoiseSettings);
    UpdateFoliageGeneration(DeltaTime, PlanetCenter, PlanetRadiusCm, CurrentNoiseSettings);

    // Debug: Dibujar celdas
    DrawDebugCells(PlanetCenter, PlanetRadiusCm);
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

void UCosmicFoliageSpawner::UpdateOctreeAndGenerate(const FVector& ViewerLocation, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings)
{
    if (!FoliageCollection)
        return;

    // Obtener nodos dentro del radio de visión
    TArray<FCubeMapCell> VisibleNodes;
    Octree.GetNodesInRadius(ViewerLocation, PlanetCenter, ViewDistanceKm, FVector::Dist(ViewerLocation, PlanetCenter) - PlanetRadiusCm, VisibleNodes);

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

void UCosmicFoliageSpawner::GenerateCellFoliage(const FCubeMapCell& Cell, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings)
{
    if (!FoliageCollection)
        return;

    float CellAreaKm2 = Octree.GetNodeAreaKm2(Cell);

    FAsyncTask<FFoliageGenerationTask>* Task =
        new FAsyncTask<FFoliageGenerationTask>(
            Cell,
            FoliageCollection,
            PlanetCenter,
            PlanetRadius,
            NoiseSettings,
            CellAreaKm2
        );

    Task->StartBackgroundTask();
    ActiveTasks.Add(Task);

    UE_LOG(LogTemp, Verbose, TEXT("Generando foliage para celda: %s"), *Cell.ToString());
}

void UCosmicFoliageSpawner::UpdateFoliageGeneration(float DeltaTime, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings)
{
    
    // Procesar tareas completadas
    for (int32 i = ActiveTasks.Num() - 1; i >= 0; i--)
    {
        FAsyncTask<FFoliageGenerationTask>* Task = ActiveTasks[i];

        if (Task->IsDone())
        {
            FFoliageGenerationTask& CompletedTask = Task->GetTask();
            const FCubeMapCell& Cell = CompletedTask.Cell;

            // Eevitar aplicar si ya no interesa
            if (PendingCells.Contains(Cell))
            {
                ApplyGeneratedInstances(Cell, CompletedTask.ResultInstances);
                PendingCells.Remove(Cell);
            }

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

