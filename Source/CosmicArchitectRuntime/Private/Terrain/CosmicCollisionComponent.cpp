// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Terrain/CosmicCollisionComponent.h"
#include "Terrain/CosmicCollisionGenerationTask.h"
#include "Terrain/CosmicClipmapGeometry.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicsEngine/BodyInstance.h"
#include "ICosmicNoiseStrategy.h"
#include "Terrain/CosmicPlanetCollisionManager.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UCosmicCollisionComponent::UCosmicCollisionComponent()
{
    bTickInEditor = true;
    PrimaryComponentTick.bCanEverTick = true;

    SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    CanCharacterStepUpOn = ECB_Yes;
    bCastDynamicShadow = false;
}

void UCosmicCollisionComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!bIsCompanion)
    {
        EnsureCompanionCreated();
    }
}

void UCosmicCollisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearCollision();

    if (CompanionPatch)
    {
        CompanionPatch->DestroyComponent();
        CompanionPatch = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void UCosmicCollisionComponent::EnsureCompanionCreated()
{
    if (bIsCompanion) return;

    if (!CompanionPatch && GetOwner())
    {
        CompanionPatch = NewObject<UCosmicCollisionComponent>(GetOwner(), NAME_None, RF_Transient);
        CompanionPatch->bIsCompanion = true;
        CompanionPatch->CollisionTriangleSize = CollisionTriangleSize;
        CompanionPatch->CollisionResolution = CollisionResolution;
        CompanionPatch->UpdateCellInterval = UpdateCellInterval;
        CompanionPatch->bUseComplexAsSimpleCollision = bUseComplexAsSimpleCollision;
        CompanionPatch->bUseAsyncCooking = bUseAsyncCooking;
        CompanionPatch->bShowCollisionMesh = bShowCollisionMesh;
        CompanionPatch->DebugColor = DebugColor;
        CompanionPatch->DebugLineWidth = DebugLineWidth;
        CompanionPatch->RegisterComponent();

        USceneComponent* ParentToAttach = GetAttachParent();
        if (!ParentToAttach || ParentToAttach == this)
        {
            ParentToAttach = GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
        }

        if (ParentToAttach && ParentToAttach != this)
        {
            CompanionPatch->AttachToComponent(ParentToAttach, FAttachmentTransformRules::KeepWorldTransform);
        }

        CompanionPatch->DeactivatePhysics();

        CompanionPatch->Tris = Tris;
        CompanionPatch->BaseVertices = BaseVertices;
        CompanionPatch->BaseNormals = BaseNormals;
        CompanionPatch->PlanetRadius = PlanetRadius;
        CompanionPatch->bBaseGeometryGenerated = bBaseGeometryGenerated;
    }
}

void UCosmicCollisionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsCompanion) return;

    // Monitor async noise task
    if (UpdateState == ECollisionUpdateState::NoiseTaskRunning)
    {
        if (NoiseTask && NoiseTask->IsDone())
        {
            TArray<FVector> CalculatedVertices = MoveTemp(NoiseTask->GetTask().CalculatedVertices);
            delete NoiseTask;
            NoiseTask = nullptr;

            EnsureCompanionCreated();

            UCosmicCollisionComponent* StandbyPatch = bPrimaryIsActiveBody ? CompanionPatch : this;
            if (StandbyPatch)
            {
                UpdateState = ECollisionUpdateState::PhysicsCooking;
                const bool bCookAsync = bUseAsyncCooking && bIsActive;
                StandbyPatch->StartPatchCook(
                    MoveTemp(CalculatedVertices),
                    PendingTransform,
                    bCookAsync,
                    [this](bool bSuccess)
                    {
                        OnStandbyCookFinished(bSuccess);
                    }
                );
            }
            else
            {
                UpdateState = ECollisionUpdateState::Idle;
            }
        }
    }

    if (bShowCollisionMesh)
    {
        DrawDebugCollisionMesh();
    }
}

#if WITH_EDITOR
void UCosmicCollisionComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // Geometry breaking changes -> full rebuild
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicCollisionComponent, CollisionTriangleSize) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicCollisionComponent, CollisionResolution))
    {
        ClearCollision();

        if (GetOwner())
        {
            GenerateCollisionMesh(PlanetRadius);
        }
        return;
    }

    // Physics configuration changes
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicCollisionComponent, bUseComplexAsSimpleCollision) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicCollisionComponent, bUseAsyncCooking))
    {
        if (CompanionPatch)
        {
            CompanionPatch->bUseComplexAsSimpleCollision = bUseComplexAsSimpleCollision;
            CompanionPatch->bUseAsyncCooking = bUseAsyncCooking;
        }

        if (IsBuilt())
        {
            RebuildCollision();
        }
        return;
    }

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicCollisionComponent, UpdateCellInterval))
    {
        if (CompanionPatch)
        {
            CompanionPatch->UpdateCellInterval = UpdateCellInterval;
        }
        return;
    }
}
#endif

void UCosmicCollisionComponent::RebuildCollision()
{
    bNeedsRebuild = true;
    bBaseGeometryGenerated = false;
    if (PlanetRadius > 0.0)
    {
        BuildBaseGrid(PlanetRadius);
    }
}

void UCosmicCollisionComponent::BuildBaseGrid(double Radius)
{
    const int32 VertRes = CollisionResolution + 1;
    const int32 TotalVertices = VertRes * VertRes;
    const int32 HalfRes = CollisionResolution / 2;

    PlanetRadius = Radius;

    BaseVertices.Empty();
    BaseNormals.Empty();

    BaseVertices.Reserve(TotalVertices);
    BaseNormals.Reserve(TotalVertices);

    for (int32 y = 0; y < VertRes; ++y)
    {
        for (int32 x = 0; x < VertRes; ++x)
        {
            const double WorldX = (x - HalfRes) * CollisionTriangleSize;
            const double WorldY = (y - HalfRes) * CollisionTriangleSize;

            FVector BasePosition;
            FVector Normal;
            FCosmicClipmapGeometry::ProjectPlanarGridPointToSphere(
                WorldX, WorldY, Radius, BasePosition, Normal);

            BaseVertices.Add(BasePosition);
            BaseNormals.Add(Normal);
        }
    }

    Tris.Empty();
    for (int32 y = 0; y < CollisionResolution; ++y)
    {
        for (int32 x = 0; x < CollisionResolution; ++x)
        {
            int32 i0 = y * VertRes + x;
            int32 i1 = i0 + 1;
            int32 i2 = i0 + VertRes;
            int32 i3 = i2 + 1;

            if (i0 >= TotalVertices || i1 >= TotalVertices ||
                i2 >= TotalVertices || i3 >= TotalVertices)
            {
                continue;
            }

            Tris.Add(i0);
            Tris.Add(i2);
            Tris.Add(i1);

            Tris.Add(i1);
            Tris.Add(i2);
            Tris.Add(i3);
        }
    }

    Verts = BaseVertices;
    bBaseGeometryGenerated = true;

    if (CompanionPatch)
    {
        CompanionPatch->Tris = Tris;
        CompanionPatch->BaseVertices = BaseVertices;
        CompanionPatch->BaseNormals = BaseNormals;
        CompanionPatch->PlanetRadius = Radius;
        CompanionPatch->bBaseGeometryGenerated = true;
    }
}

void UCosmicCollisionComponent::GenerateCollisionMesh(double Radius)
{
    if (bIsCompanion) return;

    EnsureCompanionCreated();
    BuildBaseGrid(Radius);
}

FRotator UCosmicCollisionComponent::ComputePatchRotation(const FVector& Normal)
{
    const FVector Up = Normal;

    // Choose a non-collinear vector
    const FVector Tangent = (FMath::Abs(Up.Z) < 0.99f)
        ? FVector(0, 0, 1)
        : FVector(1, 0, 0);

    FVector Right = FVector::CrossProduct(Tangent, Up);
    Right.Normalize();

    const FVector Forward = FVector::CrossProduct(Up, Right);

    return FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();
}

void UCosmicCollisionComponent::RequestCollisionUpdate(
    const FVector& SurfacePos,
    const FVector& SurfaceNormal,
    double InPlanetRadius,
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy,
    const FVector& PlanetCenter)
{
    if (bIsCompanion) return;

    EnsureCompanionCreated();

    if (!bBaseGeometryGenerated || PlanetRadius != InPlanetRadius)
    {
        BuildBaseGrid(InPlanetRadius);
    }

    if (!NoiseGenerationStrategy.IsValid()) return;

    // If an update is currently in flight, queue the newest request
    if (UpdateState != ECollisionUpdateState::Idle)
    {
        UE_LOG(LogCosmicCollision, Verbose, TEXT("[CollisionComponent] Queued update on %s because state is not Idle"), *GetName());
        bHasQueuedUpdate = true;
        QueuedSurfacePos = SurfacePos;
        QueuedSurfaceNormal = SurfaceNormal;
        QueuedPlanetRadius = InPlanetRadius;
        QueuedNoiseStrategy = NoiseGenerationStrategy;
        QueuedPlanetCenter = PlanetCenter;
        return;
    }

    UE_LOG(LogCosmicCollision, Log, TEXT("[CollisionComponent] Starting async noise task on %s at (%s)"),
        *GetName(), *SurfacePos.ToCompactString());

    PendingTransform = FTransform(ComputePatchRotation(SurfaceNormal), SurfacePos);
    LastUpdatedLocation = SurfacePos;

    NoiseTask = new FAsyncTask<FCosmicCollisionGenerationTask>(
        BaseVertices,
        BaseNormals,
        PendingTransform,
        PlanetCenter,
        NoiseGenerationStrategy
    );

    if (!bIsActive)
    {
        // First build: execute immediately and synchronously so actor has immediate ground collision
        UE_LOG(LogCosmicCollision, Log, TEXT("[CollisionComponent] First build on %s: executing synchronously for immediate collision"), *GetName());
        NoiseTask->StartSynchronousTask();

        TArray<FVector> CalculatedVertices = MoveTemp(NoiseTask->GetTask().CalculatedVertices);
        delete NoiseTask;
        NoiseTask = nullptr;

        EnsureCompanionCreated();

        UCosmicCollisionComponent* StandbyPatch = bPrimaryIsActiveBody ? CompanionPatch : this;
        if (StandbyPatch)
        {
            UpdateState = ECollisionUpdateState::PhysicsCooking;
            StandbyPatch->StartPatchCook(
                MoveTemp(CalculatedVertices),
                PendingTransform,
                false, // Synchronous cook
                [this](bool bSuccess)
                {
                    OnStandbyCookFinished(bSuccess);
                }
            );
        }
        else
        {
            UpdateState = ECollisionUpdateState::Idle;
        }
    }
    else
    {
        UE_LOG(LogCosmicCollision, Log, TEXT("[CollisionComponent] Starting async noise task on %s at (%s)"),
            *GetName(), *SurfacePos.ToCompactString());
        NoiseTask->StartBackgroundTask();
        UpdateState = ECollisionUpdateState::NoiseTaskRunning;
    }
}

void UCosmicCollisionComponent::UpdateCollisionMesh(TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy, const FVector& PlanetCenter)
{
    if (bIsCompanion) return;

    RequestCollisionUpdate(
        GetComponentLocation(),
        GetComponentRotation().Vector(),
        PlanetRadius,
        NoiseGenerationStrategy,
        PlanetCenter
    );
}

void UCosmicCollisionComponent::StartPatchCook(
    TArray<FVector>&& InVerts,
    const FTransform& InTransform,
    bool bAsync,
    TFunction<void(bool)> OnComplete)
{
    PatchCookFinishedCallback = MoveTemp(OnComplete);

    // Abort previous async creations if any
    for (UBodySetup* OldBody : AsyncBodySetupQueue)
    {
        if (OldBody)
        {
            OldBody->AbortPhysicsMeshAsyncCreation();
        }
    }
    AsyncBodySetupQueue.Empty();

    Verts = MoveTemp(InVerts);

    // Teleport to target transform with NoCollision active
    SetWorldLocationAndRotation(
        InTransform.GetLocation(),
        InTransform.Rotator(),
        false,
        nullptr,
        ETeleportType::TeleportPhysics
    );

    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        if (PatchCookFinishedCallback)
        {
            auto Callback = MoveTemp(PatchCookFinishedCallback);
            Callback(false);
        }
        return;
    }

    if (bAsync)
    {
        UE_LOG(LogCosmicCollision, Log, TEXT("[CollisionComponent] StartPatchCook: Cooking async on %s (Verts=%d)"), *GetName(), Verts.Num());
        UBodySetup* NewSetup = CreateBodySetupHelper();
        AsyncBodySetupQueue.Add(NewSetup);

        NewSetup->CreatePhysicsMeshesAsync(
            FOnAsyncPhysicsCookFinished::CreateUObject(
                this,
                &UCosmicCollisionComponent::FinishPhysicsAsyncCook,
                NewSetup
            )
        );
    }
    else
    {
        UE_LOG(LogCosmicCollision, Log, TEXT("[CollisionComponent] StartPatchCook: Cooking synchronous on %s (Verts=%d)"), *GetName(), Verts.Num());
        CreateProcMeshBodySetup();
        BodySetup->InvalidatePhysicsData();
        BodySetup->CreatePhysicsMeshes();
        FinishPhysicsAsyncCook(true, BodySetup);
    }
}

void UCosmicCollisionComponent::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup)
{
    UE_LOG(LogCosmicCollision, Log, TEXT("[CollisionComponent] FinishPhysicsAsyncCook on %s (Success=%s)"), *GetName(), bSuccess ? TEXT("true") : TEXT("false"));

    if (bSuccess && FinishedBodySetup)
    {
        if (BodySetup && BodySetup != FinishedBodySetup)
        {
            BodySetup->ClearPhysicsMeshes();
        }
        BodySetup = FinishedBodySetup;
    }

    AsyncBodySetupQueue.Remove(FinishedBodySetup);

    if (PatchCookFinishedCallback)
    {
        auto Callback = MoveTemp(PatchCookFinishedCallback);
        Callback(bSuccess);
    }
}

void UCosmicCollisionComponent::OnStandbyCookFinished(bool bSuccess)
{
    if (bSuccess)
    {
        UCosmicCollisionComponent* StandbyPatch = bPrimaryIsActiveBody ? CompanionPatch : this;
        UCosmicCollisionComponent* ActivePatch = bPrimaryIsActiveBody ? this : CompanionPatch;

        // Atomically handover collision
        if (StandbyPatch)
        {
            StandbyPatch->ActivatePhysics();
        }

        if (ActivePatch && bIsActive)
        {
            ActivePatch->DeactivatePhysics();
        }

        bPrimaryIsActiveBody = !bPrimaryIsActiveBody;
        bIsActive = true;

        UE_LOG(LogCosmicCollision, Log, TEXT("[CollisionComponent] OnStandbyCookFinished: Handover complete. Active is now %s"),
            bPrimaryIsActiveBody ? TEXT("Primary") : TEXT("Companion"));
    }
    else
    {
        UE_LOG(LogCosmicCollision, Error, TEXT("[CollisionComponent] OnStandbyCookFinished FAILED on %s"), *GetName());
    }

    UpdateState = ECollisionUpdateState::Idle;

    // Process any request that arrived while cooking
    if (bHasQueuedUpdate)
    {
        bHasQueuedUpdate = false;
        RequestCollisionUpdate(
            QueuedSurfacePos,
            QueuedSurfaceNormal,
            QueuedPlanetRadius,
            QueuedNoiseStrategy,
            QueuedPlanetCenter
        );
    }
}

void UCosmicCollisionComponent::ActivatePhysics()
{
    UE_LOG(LogCosmicCollision, Log, TEXT("[CollisionComponent] ActivatePhysics on %s (Profile: BlockAll, QueryAndPhysics)"), *GetName());
    SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    RecreatePhysicsState();
}

void UCosmicCollisionComponent::DeactivatePhysics()
{
    UE_LOG(LogCosmicCollision, Log, TEXT("[CollisionComponent] DeactivatePhysics on %s (NoCollision)"), *GetName());
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DestroyPhysicsState();
}

void UCosmicCollisionComponent::ClearCollision()
{
    if (NoiseTask)
    {
        NoiseTask->EnsureCompletion();
        delete NoiseTask;
        NoiseTask = nullptr;
    }

    for (UBodySetup* Setup : AsyncBodySetupQueue)
    {
        if (Setup)
        {
            Setup->AbortPhysicsMeshAsyncCreation();
        }
    }
    AsyncBodySetupQueue.Empty();

    if (BodySetup)
    {
        BodySetup->ClearPhysicsMeshes();
        BodySetup = nullptr;
    }

    DestroyPhysicsState();
    DeactivatePhysics();

    Verts.Empty();
    bIsActive = false;
    bNeedsRebuild = false;
    UpdateState = ECollisionUpdateState::Idle;
    bHasQueuedUpdate = false;
    LastUpdatedLocation = FVector(MAX_flt);

    if (CompanionPatch)
    {
        CompanionPatch->ClearCollision();
    }

    bPrimaryIsActiveBody = true;
}

bool UCosmicCollisionComponent::IsBuilt() const
{
    return bIsActive;
}

void UCosmicCollisionComponent::DrawDebugCollisionMesh()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UCosmicCollisionComponent* ActivePatch = bPrimaryIsActiveBody ? this : CompanionPatch;
    if (ActivePatch && ActivePatch->Verts.Num() > 0 && ActivePatch->Tris.Num() > 0)
    {
        const FTransform& ActiveTransform = ActivePatch->GetComponentTransform();
        for (int32 i = 0; i < ActivePatch->Tris.Num(); i += 3)
        {
            FVector A = ActiveTransform.TransformPosition(ActivePatch->Verts[ActivePatch->Tris[i]]);
            FVector B = ActiveTransform.TransformPosition(ActivePatch->Verts[ActivePatch->Tris[i + 1]]);
            FVector C = ActiveTransform.TransformPosition(ActivePatch->Verts[ActivePatch->Tris[i + 2]]);

            DrawDebugLine(World, A, B, DebugColor, false, -1.0f, 0, DebugLineWidth);
            DrawDebugLine(World, B, C, DebugColor, false, -1.0f, 0, DebugLineWidth);
            DrawDebugLine(World, C, A, DebugColor, false, -1.0f, 0, DebugLineWidth);
        }
    }

    // In-flight standby patch visualization
    if (UpdateState != ECollisionUpdateState::Idle)
    {
        UCosmicCollisionComponent* StandbyPatch = bPrimaryIsActiveBody ? CompanionPatch : this;
        if (StandbyPatch && StandbyPatch->Verts.Num() > 0 && StandbyPatch->Tris.Num() > 0)
        {
            const FTransform& StandbyTransform = StandbyPatch->GetComponentTransform();
            for (int32 i = 0; i < StandbyPatch->Tris.Num(); i += 3)
            {
                FVector A = StandbyTransform.TransformPosition(StandbyPatch->Verts[StandbyPatch->Tris[i]]);
                FVector B = StandbyTransform.TransformPosition(StandbyPatch->Verts[StandbyPatch->Tris[i + 1]]);
                FVector C = StandbyTransform.TransformPosition(StandbyPatch->Verts[StandbyPatch->Tris[i + 2]]);

                DrawDebugLine(World, A, B, StandbyDebugColor, false, -1.0f, 0, DebugLineWidth * 0.5f);
                DrawDebugLine(World, B, C, StandbyDebugColor, false, -1.0f, 0, DebugLineWidth * 0.5f);
                DrawDebugLine(World, C, A, StandbyDebugColor, false, -1.0f, 0, DebugLineWidth * 0.5f);
            }
        }
    }
}

UBodySetup* UCosmicCollisionComponent::CreateBodySetupHelper()
{
    UBodySetup* NewBodySetup = NewObject<UBodySetup>(this);

    NewBodySetup->BodySetupGuid = FGuid::NewGuid();
    NewBodySetup->bGenerateMirroredCollision = false;
    NewBodySetup->bDoubleSidedGeometry = true;

    NewBodySetup->CollisionTraceFlag =
        bUseComplexAsSimpleCollision ?
        CTF_UseComplexAsSimple :
        CTF_UseDefault;

    return NewBodySetup;
}

void UCosmicCollisionComponent::CreateProcMeshBodySetup()
{
    if (!BodySetup)
    {
        BodySetup = CreateBodySetupHelper();
    }
}

UBodySetup* UCosmicCollisionComponent::GetBodySetup()
{
    if (!BodySetup)
    {
        CreateProcMeshBodySetup();
    }

    return BodySetup;
}

bool UCosmicCollisionComponent::GetPhysicsTriMeshData(
    FTriMeshCollisionData* CollisionData,
    bool InUseAllTriData)
{
    if (!CollisionData || Verts.Num() == 0 || Tris.Num() == 0) return false;

    bool bCopyUVs = UPhysicsSettings::Get()->bSupportUVFromHitResults;

    if (bCopyUVs)
        CollisionData->UVs.AddZeroed(1);

    for (const FVector& V : Verts)
    {
        CollisionData->Vertices.Add((FVector3f)V);

        if (bCopyUVs)
            CollisionData->UVs[0].Add(FVector2D::ZeroVector);
    }

    int32 NumTris = Tris.Num() / 3;

    for (int32 i = 0; i < NumTris; i++)
    {
        FTriIndices Tri;
        Tri.v0 = Tris[i * 3 + 0];
        Tri.v1 = Tris[i * 3 + 1];
        Tri.v2 = Tris[i * 3 + 2];

        CollisionData->Indices.Add(Tri);
        CollisionData->MaterialIndices.Add(0);
    }

    CollisionData->bFlipNormals = true;
    CollisionData->bFastCook = true;

    return true;
}

bool UCosmicCollisionComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
    return Tris.Num() >= 3 && Verts.Num() > 0;
}

bool UCosmicCollisionComponent::GetTriMeshSizeEstimates(
    FTriMeshCollisionDataEstimates& OutTriMeshEstimates,
    bool bInUseAllTriData) const
{
    OutTriMeshEstimates.VerticeCount = Verts.Num();
    return true;
}