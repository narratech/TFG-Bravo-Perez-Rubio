#include "CosmicFoliageSpawner.h"
#include "Engine/World.h"
#include "CosmicNoiseSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"



// COMPONENTE

UCosmicFoliageSpawner::UCosmicFoliageSpawner()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCosmicFoliageSpawner::InitFoliageSpawner(float RadiusKm)
{
    PlanetRadiusCm = RadiusKm * 100000;
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
    UpdateOctreeAndGenerate(ViewerLocation, PlanetCenter, PlanetRadiusCm);

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

void UCosmicFoliageSpawner::UpdateOctreeAndGenerate(const FVector& ViewerLocation, const FVector& PlanetCenter, float PlanetRadius)
{
    if (!FoliageCollection)
        return;

    // Obtener nodos dentro del radio de visión
    TArray<FCubeMapCell> VisibleNodes;
    Octree.GetNodesInRadius(ViewerLocation, PlanetCenter, ViewDistanceKm, FVector::Dist(ViewerLocation, PlanetCenter) - PlanetRadiusCm, VisibleNodes);

    // Activar nuevos nodos
    for (const FCubeMapCell& Node : VisibleNodes)
    {
        if (!ActiveCells.Contains(Node))
        {
            // Generar foliage para este nodo
            GenerateCellFoliage(Node, PlanetCenter, PlanetRadius);

            // Crear entrada en ActiveCells
            FCosmicFoliageCellData& CellData = ActiveCells.FindOrAdd(Node);
            CellData.LastUpdateTime = GetWorld()->GetTimeSeconds();

            UE_LOG(LogTemp, Log, TEXT("Activando celda: %s"), *Node.ToString());
        }
    }

    // Desactivar nodos lejanos
    TArray<FCubeMapCell> ToRemove;
    for (const auto& Pair : ActiveCells)
    {
        if (!VisibleNodes.Contains(Pair.Key))
        {
            // Destruir instancias
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

void UCosmicFoliageSpawner::GenerateCellFoliage(const FCubeMapCell& Cell, const FVector& PlanetCenter, float PlanetRadius)
{
    // TODO: Implementar generación real de foliage
    // Por ahora, solo logueamos
    UE_LOG(LogTemp, Verbose, TEXT("Generando foliage para celda: %s"), *Cell.ToString());
}

void UCosmicFoliageSpawner::UpdateFoliageGeneration(float DeltaTime, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings)
{
    ElapsedTime += DeltaTime;
    if (ElapsedTime < UpdateInterval) return;
    ElapsedTime = 0.0f;

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC || !PC->PlayerCameraManager) return;

    FVector ViewerLocation = PC->PlayerCameraManager->GetCameraLocation();

    DrawDebugCells(PlanetCenter, PlanetRadius);

    CleanupFarInstances(ViewerLocation, ViewDistanceKm * 1.5f);
    RequestAreaGeneration(ViewerLocation, ViewDistanceKm, PlanetCenter, PlanetRadius, NoiseSettings);

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

FIntVector UCosmicFoliageSpawner::WorldToCell(const FVector& WorldPos) const
{
    float CellSizeCm = CellSizeKm * 100000.0f;
    return FIntVector(
        FMath::FloorToInt(WorldPos.X / CellSizeCm),
        FMath::FloorToInt(WorldPos.Y / CellSizeCm),
        FMath::FloorToInt(WorldPos.Z / CellSizeCm)
    );
}

void UCosmicFoliageSpawner::RequestAreaGeneration(const FVector& Center, float RadiusKm, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings)
{
    float RadiusCm = RadiusKm * 100000.0f;
    float CellSizeCm = CellSizeKm * 100000.0f;

    int32 CellsPerSide = FMath::CeilToInt((RadiusCm * 2) / CellSizeCm);
    FIntVector CenterCell = WorldToCell(Center);

    for (int32 x = -CellsPerSide / 2; x <= CellsPerSide / 2; x++)
    {
        for (int32 y = -CellsPerSide / 2; y <= CellsPerSide / 2; y++)
        {
            FIntVector Cell = CenterCell + FIntVector(x, y, 0);

            // Verificar si la celda NO está en el mapa Y NO está pendiente de generación
            if (!CellDataMap.Contains(Cell) && !PendingGenerationCells.Contains(Cell))
            {
                PendingGenerationCells.Add(Cell);  // Marcar como pendiente
                PendingCells.Enqueue(Cell);
            }
        }
    }

    int32 ProcessedThisFrame = 0;
    FIntVector NextCell;
    while (ProcessedThisFrame < MaxInstancesPerFrame && PendingCells.Dequeue(NextCell))
    {
        GenerateCell(NextCell, PlanetCenter, PlanetRadius, NoiseSettings);
        ProcessedThisFrame++;
    }
}

void UCosmicFoliageSpawner::GenerateCell(const FIntVector& Cell, const FVector& PlanetCenter, float PlanetRadius, UCosmicNoiseSettings* NoiseSettings)
{
    if (!FoliageCollection || !NoiseSettings) return;

    FAsyncTask<FFoliageGenerationTask>* AsyncTask =
        new FAsyncTask<FFoliageGenerationTask>(
            Cell,
            FoliageCollection,
            RandomStream.RandRange(0, 999999),
            PlanetCenter,
            PlanetRadius,
            NoiseSettings->Params,
            CellSizeKm 
        );

    AsyncTask->StartBackgroundTask();
    ActiveTasks.Add(AsyncTask);
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

    // ¡IMPORTANTE! La celda ya no está pendiente, está generada
    PendingGenerationCells.Remove(Cell);
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
    //Comp->SetupAttachment(GetOwner()->GetRootComponent());
    Comp->SetWorldLocation(FVector::ZeroVector);
    Comp->SetWorldRotation(FQuat::Identity);
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

    UE_LOG(LogTemp, Warning, TEXT("Celdas removidas: %d"), CellsToRemove.Num());

    for (const FIntVector& Cell : CellsToRemove)
    {
        CellDataMap.Remove(Cell);
        // También limpiar de pendientes por si acaso
        PendingGenerationCells.Remove(Cell);
    }
}