// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "Terrain/CosmicClipmapComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Terrain/CosmicMeshComponent.h"
#include "Terrain/CosmicCollisionComponent.h"
#include "CosmicNoiseClass.h"
#include "CosmicDefaultNoiseStrategy.h"
#include "CosmicFoliageSpawner.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture.h"
#include "DrawDebugHelpers.h"
#include "ModulesBridge/CosmicCameraBridge.h"
#include "GameFramework/Pawn.h"


UCosmicClipmapComponent::UCosmicClipmapComponent()
{ 

    bTickInEditor = true;

	PrimaryComponentTick.bCanEverTick = true;
}

bool UCosmicClipmapComponent::UpdateCollisionNearPlayer(const FVector& SurfacePos, const FVector& SurfaceNormal, const double DistanceToSurface)
{
    if (!CollisionComponent) return false;

    // Only generate collision if player is near surface
    if (DistanceToSurface < CollisionComponent->MaxCollisionDistance)
    {
        if (!CollisionComponent->IsBuilt()) 
        {
            CollisionComponent->GenerateCollisionMesh(PlanetRadius);
        }
        CollisionComponent->SetWorldLocationAndRotation(
            SurfacePos,
            GetPatchRotation(SurfaceNormal),
            false, 
            nullptr,
            ETeleportType::TeleportPhysics // clean teleport
        );
        CollisionComponent->UpdateCollisionMesh(NoiseGenerationStrategy, CurrentActorPosition);
        return true;
    }
    else if(CollisionComponent->IsBuilt())
    {
        // Clear collision if far away
        CollisionComponent->ClearCollision();
        return true;
    }

    return false;
}


void UCosmicClipmapComponent::BeginPlay()
{

    Super::BeginPlay();

    TimeToRefreshActive = TimeToRefresh;

    // Initialize values for shift
    LastSurfaceAngles = FVector2D::ZeroVector;
    AccumulatedLinearDelta = FVector2D::ZeroVector;

    FVector SurfacePos;
    FVector N;
    FVector ViewerPos;
    float DistanceToSurface;

    bPerformaceMode = true;

    ElapsedTime = FMath::FRandRange(0.f, TimeToRefresh);

    DistanceToSurface = GetDistanceToSurface(ViewerPos, SurfacePos, N);

    UpdateMeshPhase(ViewerPos, SurfacePos, N, DistanceToSurface);

    UpdateCollisionNearPlayer(SurfacePos, N, DistanceToSurface);

    LastSurfaceAngles = GetSurfaceAngles(SurfacePos);
    LastPlayerPos = ViewerPos;
    LastMeshPlayerPos = ViewerPos;
}

void UCosmicClipmapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {

    ClearLevels();
    Super::EndPlay(EndPlayReason);
}

void UCosmicClipmapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ElapsedTime += DeltaTime;

    if (DynamicPlanetMat) {
        DynamicPlanetMat->SetVectorParameterValue("PlanetCenter", GetOwner()->GetActorLocation());
    }

    if (ElapsedTime <= TimeToRefresh)
        return;

    if (!UseClipmap && bPerformanceBuild)
        return;

    ElapsedTime = ElapsedTime - TimeToRefresh;

    FVector SurfacePos;
    FVector N;
    FVector ViewerPos;
    float DistanceToSurface;  
 
    if (!bPerformaceMode) {

        bool UpdateFoliageExtra = false;
        DistanceToSurface = GetDistanceToSurface(ViewerPos, SurfacePos, N);

        double PhaseStart = FPlatformTime::Seconds();

        // PER-PHASE EXECUTION
        switch (CurrentPhase)
        {
        case EUpdatePhase::Foliage:
            UpdateFoliagePhase(DeltaTime, SurfacePos + N * DistanceToSurface, DistanceToSurface);
            break;

        case EUpdatePhase::Collision:
            UpdateFoliageExtra = !UpdateCollisionPhase(ViewerPos, SurfacePos, N, DistanceToSurface);
            break;

        case EUpdatePhase::Mesh:
            UpdateMeshPhase(ViewerPos, SurfacePos, N, DistanceToSurface);
            break;
        }

        // If collision doesn't need update, request foliage update
        if (UpdateFoliageExtra)
        {
            //CurrentPhase = EUpdatePhase::Foliage;
            UpdateFoliagePhase(DeltaTime, SurfacePos + N * DistanceToSurface, DistanceToSurface);
        }
        
        CurrentPhase = (EUpdatePhase)(((uint8)CurrentPhase + 1) % 3);
    }
    else
    {
        DistanceToSurface = GetFastDistanceToSurface(ViewerPos, SurfacePos, N);
        UpdateMeshPhase(ViewerPos, SurfacePos, N, DistanceToSurface);
    }
}

void UCosmicClipmapComponent::UpdateFoliagePhase(float DeltaTime, const FVector& ViewerPos, float DistanceToSurface)
{
    if (FoliageSpawnerComponent)
    {
        FoliageSpawnerComponent->UpdateFoliageSpawner(
            TimeToRefresh, ViewerPos, CurrentActorPosition,
            PlanetRadius, DistanceToSurface, NoiseGenerationStrategy
        );
    }
}

bool UCosmicClipmapComponent::UpdateCollisionPhase(const FVector& ViewerPos, const FVector& SurfacePos,
    const FVector& N, float DistanceToSurface)
{
    if (CollisionComponent &&
        !LastMeshPlayerPos.Equals(ViewerPos, CollisionComponent->CollisionTriangleSize))
    {
        LastMeshPlayerPos = ViewerPos;
        return UpdateCollisionNearPlayer(SurfacePos, N, DistanceToSurface);
    }
    return false;
}

void UCosmicClipmapComponent::UpdateMeshPhase(const FVector& ViewerPos, const FVector& SurfacePos,
    const FVector& N, float DistanceToSurface)
{
    if (!FarLevel) return;

    // Permanent update of FarLevel (performance)
    if (!bPerformanceBuild)
    {
        FarLevel->RequestMeshUpdate(NoiseGenerationStrategy);
        bPerformanceBuild = FarLevel->CheckAndApplyMeshUpdate();
    }

    if (!UseClipmap) return;

    // Mode change detection
    bool bPrevPerformanceMode = bPerformaceMode;
    bPerformaceMode = DistanceToSurface > PlanetRadius * HeightVisibility;

    // Create normal levels if necessary and not in performance mode
    if (!bInit && !bPerformaceMode)
    {
        CreateLevels();
    }

    // Handle mode transitions
    if (bPrevPerformanceMode != bPerformaceMode)
    {
        if (bPerformaceMode) // Go to performance mode (wait for pending tasks)
        {
            // Activate FarLevel immediately (already built)
            FarLevel->SetMeshActive(true);

            for (size_t i = 0; i < Levels.Num(); i++)
            {
                Levels[i]->SetMeshActive(false);
            }
            // Start waiting to hide levels once their tasks finish
            bWaitingForPerformanceTransition = true;
        }
        else // Go to normal mode (wait for levels to be ready)
        {
            bWaitingForNormalTransition = true;

            ConfigureLevelsForViewer(N);

            for (size_t i = 0; i < Levels.Num(); i++)
            {
                Levels[i]->RequestMeshUpdate(NoiseGenerationStrategy);
            }

            return;
        }
    }

    // If waiting for transition to performance
    if (bWaitingForPerformanceTransition)
    {
        bool allTasksDone = true;
        for (size_t i = 0; i < Levels.Num(); i++)
        {
            if (Levels[i]->IsTaskActive())
            {
                allTasksDone = false;
                break;
            }
        }

        if (!allTasksDone)
        {
            return;  // Keep waiting, FarLevel is already visible meanwhile
        }

        // Tasks finished: discard results and hide levels
        for (size_t i = 0; i < Levels.Num(); i++)
        {
            Levels[i]->CancelAsyncWork();
        }

        bWaitingForPerformanceTransition = false;
        return;  // Do not continue with normal logic
    }

    // If waiting for transition to normal
    if (bWaitingForNormalTransition)
    {
        bool allTasksDone = true;
        for (size_t i = 0; i < Levels.Num(); i++)
        {
            if (Levels[i]->IsTaskActive())
            {
                allTasksDone = false;
                break;
            }
        }

        if (!allTasksDone)
        {
            return;  // Keep waiting, FarLevel remains visible
        }

        // All tasks finished: apply results and show levels
        for (size_t i = 0; i < Levels.Num(); i++)
        {
            Levels[i]->CheckAndApplyMeshUpdate();
            Levels[i]->SetMeshActive(true);
        }
        FarLevel->SetMeshActive(false);

        bWaitingForNormalTransition = false;
    }

    // If in performance mode or frozen, do not continue
    if (bPerformaceMode || FreezeGeneration) return;

    // Check whether updates have finished
    bool MeshesUpdated = true;
    for (size_t i = 0; i < Levels.Num(); i++)
    {
        if (Levels[i]->IsTaskActive())
        {
            MeshesUpdated = false;
            break;
        }
    }

    if (!MeshesUpdated) return;

    // Tasks finished: apply updates
    for (size_t i = 0; i < Levels.Num(); i++)
    {
        Levels[i]->CheckAndApplyMeshUpdate();
    }

    // Normal clipmap logic (normal mode already set)

    // Check whether clipmap levels need to be decreased or increased
    bool UpdateClipmapLevels = false;
    if (Levels.Num() > 1)
    {
        UCosmicMeshComponent* MeshLast = Levels.Last();
        UCosmicMeshComponent* MeshFirst = Levels[0];

        const bool bLastVisible = IsClipmapRingVisible(Levels.Num() - 1, DistanceToSurface);

        if (!bLastVisible && MeshFirst->GridSpacing > MinTriangleSize)
        {
            const int32 Steps = CalculateDecreaseSteps(DistanceToSurface);
            DecreaseClipmapLevelFull(Steps);
            UpdateClipmapLevels = true;
        }
        else if (IsClipmapRingVisible(MeshLast->GridSpacing * 2, MeshLast->Resolution, DistanceToSurface)
            && MeshLast->GridSpacing < BaseSpacing * FMath::Pow(2.0f, NumLevels - 1))
        {
            const int32 Steps = CalculateIncreaseSteps(DistanceToSurface);
            IncreaseClipmapLevelFull(Steps);
            UpdateClipmapLevels = true;
        }
    }

    // Movement is computed in absolute tangent plane coordinates.
    // Common center is quantized to coarsest spacing so that all
    // levels retain coincident vertices on their 2:1 borders.
    const bool bProjectionUpdate = ConfigureLevelsForViewer(N);

    if (bProjectionUpdate || UpdateClipmapLevels)
    {
        for (size_t i = 0; i < Levels.Num(); i++)
        {
            if (Levels[i]->IsPlanetaryProjectionUpdateRequired())
            {
                Levels[i]->RequestMeshUpdate(NoiseGenerationStrategy);
            }
        }
    } 
}


#if WITH_EDITOR
void UCosmicClipmapComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // FULL REBUILD
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicClipmapComponent, BaseResolution) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicClipmapComponent, NumLevels))
    {
        ClearLevels();

        CreatePerformanceLevel(true);

        if (!bPerformaceMode)
        {
            CreateLevels();
        }

        return;
    }

    //  BASE MATERIAL / TEXTURE
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicClipmapComponent, BaseMaterial) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicClipmapComponent, DefaultTexture))
    {
        BuildDynamicMaterial();
        return;
    }

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicClipmapComponent, UseClipmap))
    {
        if (!UseClipmap)
        {
            ClearLevels();

            CreatePerformanceLevel(true);
        }
        return;
    }

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicClipmapComponent, PlanetGridSnapAngleDegrees))
    {
        bSnappedProjectionValid = false;
        bCoarsestGridCenterValid = false;
        ++SnappedProjectionRevision;
        return;
    }

}
#endif

void UCosmicClipmapComponent::CreateLevels()
{

    if (bInit)
    {
        ClearLevels();
    }

    if (CollisionComponent && ParentRoot)
    {
        CollisionComponent->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);
        CollisionComponent->GenerateCollisionMesh(PlanetRadius);
    }

    // Validate parameters
    if (NumLevels <= 0 || BaseResolution <= 0 || PlanetRadius <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Parámetros inválidos para CreateLevels"));
        return;
    }

    int32 Remainder = BaseResolution % 4;

    if (Remainder != 0) {
        BaseResolution += 4 - Remainder;
    }

    Levels.Empty();
    Levels.SetNum(NumLevels);

    TotalShift = FIntPoint::ZeroValue;

    AActor* Owner = GetOwner();
    if (!Owner) LastPlayerPos = FVector::Zero();
    else LastPlayerPos = Owner->GetActorLocation();
        
    BaseGridSpacing = BaseSpacing = (PlanetRadius * 2.0f) / (BaseResolution * FMath::Pow(2.0f, NumLevels - 1));
     
    FVector SurfacePos;
    FVector N;
    FVector ViewerPos;

    if (IsPlanet) {
        GetDistanceToSurface(ViewerPos, SurfacePos, N);
    }

    UpdateSnappedProjectionFrame(N);
    const FRotator PatchRotation = SnappedProjectionFrame.GetRotation().Rotator();

    // Ensure dynamic material exists before instantiating levels
    if (!DynamicPlanetMat && BaseMaterial)
    {
        BuildDynamicMaterial();
    }

    // Create each level
    for (int32 L = 0; L < NumLevels; ++L)
    {
        // Create unique name for component
        FName ComponentName = *FString::Printf(TEXT("TerrainClipmapMesh_Level_%d"), L);

        // Create component
        UCosmicMeshComponent* Mesh = NewObject<UCosmicMeshComponent>(
            GetOwner(),
            ComponentName,
            RF_Transient | RF_DuplicateTransient  // Mark as transient
        );

        if (!Mesh)
        {
            UE_LOG(LogTemp, Error, TEXT("No se pudo crear ClipmapMeshComponent para nivel %d"), L);
            continue;
        }

        // Register component
        Mesh->RegisterComponent();

        // Attach to root
        if (ParentRoot)
        {
            Mesh->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);
        }

        // Configure properties
        Mesh->LevelIndex = L;
        Mesh->Resolution = BaseResolution;
        Mesh->GridSpacing = BaseGridSpacing * FMath::Pow(2.0f, L); // (1 << L) for ints
        Mesh->bIsRing = (L > 0);
        Mesh->PlanetRadius = PlanetRadius;
        Mesh->bIsPlanet = true;

        Mesh->SetPositionAndRotation(SnappedProjectionFrame.GetTranslation(), PatchRotation);

        // Build mesh
        Mesh->BuildBaseProjectedMesh();
        Mesh->SetMeshActive(false);
        
        // Assign material
        if (DynamicPlanetMat)
        {
            Mesh->SetMaterial(0, DynamicPlanetMat);
        }

        // Store reference
        Levels[L] = Mesh;
    }

    bInit = true;

    ConfigureLevelsForViewer(N);
    for (UCosmicMeshComponent* Mesh : Levels)
    {
        if (Mesh)
        {
            Mesh->RequestMeshUpdate(NoiseGenerationStrategy);
        }
    }
}

void UCosmicClipmapComponent::CreatePerformanceLevel(bool bActive)
{
    if (FarLevel) return;

    FName ComponentName = *FString::Printf(TEXT("TerrainClipmapMesh_Performance_%d"), 0);

    UCosmicMeshComponent* Mesh = NewObject<UCosmicMeshComponent>(
        GetOwner(),
        ComponentName,
        RF_Transient | RF_DuplicateTransient  // Mark as transient
    );

    if (Mesh)
    {
        Mesh->RegisterComponent();

        if (ParentRoot)
        {
            Mesh->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);
        }

        Mesh->LevelIndex = NumLevels - 1;
        Mesh->Resolution = BaseResolution;
        Mesh->GridSpacing = BaseGridSpacing * FMath::Pow(2.0f, NumLevels - 1);
        Mesh->bIsRing = false;
        Mesh->PlanetRadius = PlanetRadius;
        Mesh->bIsPlanet = false;

        Mesh->BuildSphereMesh();
        Mesh->SetMeshActive(bActive);

        FarLevel = Mesh;

        BuildDynamicMaterial();
    }

    UpdateNoiseEvaluator();

    bPerformaceMode = true;

    bInit = false;
}


void UCosmicClipmapComponent::ClearLevels()
{
    bInit = false;

    int LevelsCleared = 0;  

    TotalShift = FIntPoint::ZeroValue;
    bSnappedProjectionValid = false;
    bCoarsestGridCenterValid = false;

    // Destroy components in Levels array
    for (UCosmicMeshComponent* Mesh : Levels)
    {
        // Disable and clear mesh
        Mesh->ClearAllMeshSections();
        Mesh->CancelAsyncWork();
        Mesh->DestroyComponent();
        Mesh = nullptr;
    }

    // Destroy outer level
    if (FarLevel)
    {
        FarLevel->ClearAllMeshSections();
        FarLevel->CancelAsyncWork();
        FarLevel->DestroyComponent();
        FarLevel = nullptr;
    }

    Levels.Empty();

    if (CollisionComponent && CollisionComponent->IsBuilt())
    {
        CollisionComponent->ClearCollision();
    }


    DynamicPlanetMat = nullptr;
    

    bPerformanceBuild = false;
}

void UCosmicClipmapComponent::ResetPointersAfterDuplicate(USceneComponent* NewRoot)
{
    ParentRoot = NewRoot;
    Levels.Empty();
    FarLevel = nullptr;
    DynamicPlanetMat = nullptr;
    bInit = false;
    bPerformanceBuild = false;
    bPerformaceMode = false;
    TotalShift = FIntPoint::ZeroValue;
    bSnappedProjectionValid = false;
    bCoarsestGridCenterValid = false;
    ++SnappedProjectionRevision;
}

void UCosmicClipmapComponent::SetMaterialData(FColor Color1, FColor Color2, FColor ColorCold, FColor ColorHot,
    FColor ColorSlope, float ScaleL, float ScaleM, float ScaleS)
{
    PlanetMainColor1 = Color1;
    PlanetMainColor2 = Color2;
    PlanetColdColor = ColorCold;
    PlanetHotColor = ColorHot;
    PlanetSlopeColor = ColorSlope;
    NoiseScaleLarge = ScaleL;
    NoiseScaleMedium = ScaleM;
    NoiseScaleSmall = ScaleS;

    if (DynamicPlanetMat)
    {
        DynamicPlanetMat->SetScalarParameterValue(FName("PlanetRadius"), PlanetRadius);
        DynamicPlanetMat->SetVectorParameterValue(FName("BaseColor"), Color1);
        DynamicPlanetMat->SetVectorParameterValue(FName("MidColor"), Color2);
        DynamicPlanetMat->SetVectorParameterValue(FName("ColdColor"), ColorCold);
        DynamicPlanetMat->SetVectorParameterValue(FName("HotColor"), ColorHot);
        DynamicPlanetMat->SetVectorParameterValue(FName("SlopeColor"), ColorSlope);
        DynamicPlanetMat->SetScalarParameterValue(FName("NoiseScaleLarge"), ScaleL);
        DynamicPlanetMat->SetScalarParameterValue(FName("NoiseScaleMedium"), ScaleM);
        DynamicPlanetMat->SetScalarParameterValue(FName("NoiseScaleSmall"), ScaleS);
        if(DefaultTexture)
            DynamicPlanetMat->SetTextureParameterValue(FName("Floortexture"), DefaultTexture);
    }
}

void UCosmicClipmapComponent::BuildDynamicMaterial() 
{
    if (BaseMaterial) {

        DynamicPlanetMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);

        SetMaterialData(PlanetMainColor1, PlanetMainColor2, PlanetColdColor, PlanetHotColor,
            PlanetSlopeColor, NoiseScaleLarge, NoiseScaleMedium, NoiseScaleSmall);
    } 
    else {
        DynamicPlanetMat = nullptr;
    }

    for (size_t i = 0; i < Levels.Num(); i++)
    {
        if (Levels[i])
        {
            Levels[i]->SetMaterial(0, DynamicPlanetMat);
        }
    }

    if (FarLevel)
        FarLevel->SetMaterial(0, DynamicPlanetMat);
}

void UCosmicClipmapComponent::UpdateNoiseEvaluator()
{
    if (NoiseClass)
    {
        NoiseGenerationStrategy = NoiseClass->CreateStrategy();
    }
    else
    {
        // default fallback
        TSharedPtr<FCosmicDefaultNoiseStrategy> Strategy = MakeShared<FCosmicDefaultNoiseStrategy>();

        Strategy->Initialize(
            1337,
            FCosmicNoiseLayer(), 
            FCosmicNoiseBiomeParameters()
        );

        NoiseGenerationStrategy = Strategy;
    }
}

void UCosmicClipmapComponent::RequestCompleteMeshUpdate()
{
    bPerformanceBuild = false;

    UpdateNoiseEvaluator();
    
    if(!bPerformaceMode)
    {
        // A new revision invalidates caches even when observer has
        // not moved (for example, after changing noise seed).
        ++SnappedProjectionRevision;

        const AActor* Owner = GetOwner();
        const FVector ViewerNormal = Owner
            ? (GetPlayerLocation() - Owner->GetActorLocation()).GetSafeNormal()
            : FVector::UpVector;
        ConfigureLevelsForViewer(ViewerNormal);

        for (size_t i = 0; i < Levels.Num(); i++)
        {        
            Levels[i]->RequestMeshUpdate(NoiseGenerationStrategy);
        }
    }

    if (FoliageSpawnerComponent)
    {
        FoliageSpawnerComponent->ClearFoliage();
    } 
}

FRotator UCosmicClipmapComponent::GetPatchRotation(const FVector& N) const
{
    const FVector Up = N;

    // Choose a non-collinear vector
    const FVector Tangent = (FMath::Abs(Up.Z) < 0.99f)
        ? FVector(0, 0, 1)
        : FVector(1, 0, 0);

    // Right ends up normalized if Up and Tangent are unit vectors
    FVector Right = FVector::CrossProduct(Tangent, Up);
    Right.Normalize(); 

    const FVector Forward = FVector::CrossProduct(Up, Right); // already unit

    return FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();
}

bool UCosmicClipmapComponent::UpdateSnappedProjectionFrame(const FVector& ViewerNormal)
{
    FVector SafeNormal = ViewerNormal.GetSafeNormal();
    if (SafeNormal.IsNearlyZero())
    {
        SafeNormal = bSnappedProjectionValid
            ? SnappedProjectionFrame.TransformVectorNoScale(FVector::UpVector)
            : FVector::UpVector;
    }

    const double SnapRadians = FMath::DegreesToRadians(
        FMath::Clamp(static_cast<double>(PlanetGridSnapAngleDegrees), 0.1, 45.0));
    const double Longitude = FMath::Atan2(SafeNormal.Y, SafeNormal.X);
    const double Latitude = FMath::Asin(FMath::Clamp(SafeNormal.Z, -1.0, 1.0));

    const int32 LongitudeCell = FMath::RoundToInt(Longitude / SnapRadians);
    const int32 LatitudeCell = FMath::RoundToInt(Latitude / SnapRadians);
    const FIntPoint NewProjectionKey(LongitudeCell, LatitudeCell);

    if (bSnappedProjectionValid && NewProjectionKey == SnappedProjectionKey)
    {
        return false;
    }

    const double SnappedLongitude = LongitudeCell * SnapRadians;
    const double SnappedLatitude = FMath::Clamp(
        LatitudeCell * SnapRadians,
        -HALF_PI,
        HALF_PI);
    const double CosLatitude = FMath::Cos(SnappedLatitude);

    const FVector AnchorNormal(
        CosLatitude * FMath::Cos(SnappedLongitude),
        CosLatitude * FMath::Sin(SnappedLongitude),
        FMath::Sin(SnappedLatitude));
    const FVector East(
        -FMath::Sin(SnappedLongitude),
        FMath::Cos(SnappedLongitude),
        0.0);
    const FVector North = FVector::CrossProduct(AnchorNormal, East).GetSafeNormal();
    const FQuat FrameRotation = FRotationMatrix::MakeFromXY(East, North).ToQuat();

    SnappedProjectionFrame = FTransform(
        FrameRotation,
        AnchorNormal * PlanetRadius,
        FVector::OneVector);
    SnappedProjectionKey = NewProjectionKey;
    bSnappedProjectionValid = true;
    bCoarsestGridCenterValid = false;
    ++SnappedProjectionRevision;

    return true;
}

FVector2D UCosmicClipmapComponent::ProjectDirectionToSnappedFrame(const FVector& Direction) const
{
    if (!bSnappedProjectionValid)
    {
        return FVector2D::ZeroVector;
    }

    const FVector SafeDirection = Direction.GetSafeNormal();
    const FVector FrameX = SnappedProjectionFrame.TransformVectorNoScale(FVector::ForwardVector);
    const FVector FrameY = SnappedProjectionFrame.TransformVectorNoScale(FVector::RightVector);

    return FVector2D(
        PlanetRadius * FVector::DotProduct(SafeDirection, FrameX),
        PlanetRadius * FVector::DotProduct(SafeDirection, FrameY));
}

bool UCosmicClipmapComponent::ConfigureLevelsForViewer(const FVector& ViewerNormal)
{
    if (Levels.IsEmpty() || !Levels.Last())
    {
        return false;
    }

    const bool bFrameChanged = UpdateSnappedProjectionFrame(ViewerNormal);
    const FVector2D ViewerCoordinates = ProjectDirectionToSnappedFrame(ViewerNormal);
    const int64 CoarsestSpacing = Levels.Last()->GridSpacing;
    if (CoarsestSpacing <= 0)
    {
        return false;
    }

    const FIntPoint NewCoarsestCenter(
        FMath::RoundToInt(ViewerCoordinates.X / static_cast<double>(CoarsestSpacing)),
        FMath::RoundToInt(ViewerCoordinates.Y / static_cast<double>(CoarsestSpacing)));
    const bool bCenterChanged =
        !bCoarsestGridCenterValid || NewCoarsestCenter != CoarsestGridCenter;

    CoarsestGridCenter = NewCoarsestCenter;
    bCoarsestGridCenterValid = true;

    bool bRequiresUpdate = bFrameChanged || bCenterChanged;

    for (int32 LevelIndex = 0; LevelIndex < Levels.Num(); ++LevelIndex)
    {
        UCosmicMeshComponent* Mesh = Levels[LevelIndex];
        if (!Mesh || Mesh->GridSpacing <= 0)
        {
            continue;
        }

        const int64 LevelMultiplier = CoarsestSpacing / Mesh->GridSpacing;
        const FIntPoint LevelCenter(
            static_cast<int32>(static_cast<int64>(CoarsestGridCenter.X) * LevelMultiplier),
            static_cast<int32>(static_cast<int64>(CoarsestGridCenter.Y) * LevelMultiplier));

        Mesh->ConfigurePlanetaryProjection(
            SnappedProjectionFrame,
            LevelCenter,
            SnappedProjectionRevision,
            LevelIndex < Levels.Num() - 1);
        bRequiresUpdate |= Mesh->IsPlanetaryProjectionUpdateRequired();
    }

    return bRequiresUpdate;
}

FIntPoint UCosmicClipmapComponent::ComputeGridShiftPlanar(
    const FVector& PlayerPos,
    float GridSpacing)
{
    FVector PlanetCenter = GetOwner()->GetActorLocation();

    FVector FrameDelta = PlayerPos - LastPlayerPos;

    LastPlayerPos = PlayerPos;

    AccumulatedDelta += FrameDelta;

    int32 ShiftX = FMath::FloorToInt(AccumulatedDelta.X / GridSpacing);
    int32 ShiftY = FMath::FloorToInt(AccumulatedDelta.Y / GridSpacing);

    AccumulatedDelta.X -= ShiftX * GridSpacing;
    AccumulatedDelta.Y -= ShiftY * GridSpacing;

    return FIntPoint(ShiftX, ShiftY);
}

FIntPoint UCosmicClipmapComponent::ComputeGridShiftSpherical(const FVector& PlayerPos, const FVector& CurrentSurfacePos, int64 GridSpacing)
{
    FVector PlanetCenter = CurrentActorPosition;

    // If first time, no movement
    if (LastPlayerPos.IsZero())
    {
        LastPlayerPos = PlayerPos;
        LastSurfaceAngles = GetSurfaceAngles(CurrentSurfacePos);
        return FIntPoint::ZeroValue;
    }

    // Get spherical angles (longitude and latitude) for both positions
    FVector2D CurrentAngles = GetSurfaceAngles(CurrentSurfacePos - CurrentActorPosition);
    FVector2D PreviousAngles = LastSurfaceAngles;

    // Calculate angular displacement (in radians)
    FVector2D DeltaAngles = CurrentAngles - PreviousAngles;

    // Normalize longitude to [-PI, PI] range to take shortest path
    if (DeltaAngles.X > PI) DeltaAngles.X -= 2 * PI;
    if (DeltaAngles.X < -PI) DeltaAngles.X += 2 * PI;

    // Convert angular displacement to surface linear distance
    FVector2D LinearDelta = DeltaAngles * PlanetRadius;

    // Accumulate linear displacement
    AccumulatedLinearDelta += LinearDelta;

    // Calculate how many "grid steps" we moved
    // GridSpacing is the distance between vertices on tangent plane
    int32 ShiftX = FMath::FloorToInt(AccumulatedLinearDelta.X / GridSpacing);
    int32 ShiftY = FMath::FloorToInt(AccumulatedLinearDelta.Y / GridSpacing);

    // Subtract what was already used
    AccumulatedLinearDelta.X -= ShiftX * GridSpacing;
    AccumulatedLinearDelta.Y -= ShiftY * GridSpacing;

    // Save for next frame
    LastPlayerPos = PlayerPos;
    LastSurfaceAngles = CurrentAngles;
    PreviousSurfacePos = CurrentSurfacePos;

    return FIntPoint(ShiftX, ShiftY);
}

FIntPoint UCosmicClipmapComponent::ComputeGridShift(const FVector& PlayerPos, const FVector& CurrentSurfacePos, float GridSpacing)
{
    if (IsPlanet)
    {
        // Use spherical version
        return ComputeGridShiftSpherical(PlayerPos, CurrentSurfacePos, GridSpacing);
    }
    else
    {
        // Use original planar version
        return ComputeGridShiftPlanar(PlayerPos, GridSpacing);
    }
}

FVector2D UCosmicClipmapComponent::GetSurfaceAngles(const FVector& SurfacePos)
{
    // Assuming planet center is at (0,0,0) or adjusting
    FVector Normalized = SurfacePos.GetSafeNormal();

    // Longitude: angle on XY plane (-PI to PI)
    double Longitude = FMath::Atan2(Normalized.Y, Normalized.X);

    // Latitude: angle from equator (-PI/2 to PI/2)
    double Latitude = FMath::Asin(FMath::Clamp(Normalized.Z, -0.999999f, 0.999999f));

    return FVector2D(Longitude, Latitude);
}

FVector UCosmicClipmapComponent::GetPlayerLocation()
{
    FVector PlayerLocation = FVector::ZeroVector;

    if (GetWorld()->IsGameWorld())
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            APawn* Pawn = PC->GetPawn();
            if (Pawn)
            {
                PlayerLocation = Pawn->GetActorLocation();
            }
        }
    }

#if WITH_EDITOR
    // In editor, if no game camera, use editor camera
    if (PlayerLocation.IsZero())
    {
        PlayerLocation = FCosmicCameraBridge::CameraLocation;
    }
#endif

    return PlayerLocation;
}

double UCosmicClipmapComponent::GetDistanceToSurface(
    FVector& OutViewerPos,
    FVector& OutSurfacePos,
    FVector& OutN)
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    OutViewerPos = GetPlayerLocation();
    CurrentActorPosition = Owner->GetActorLocation();

    FVector CenterToViewer = OutViewerPos - CurrentActorPosition;
    double DistanceToCenter = CenterToViewer.Length();

    OutN = CenterToViewer.GetSafeNormal();

    FLinearColor Dummy;
    float Height = 0;

    if(NoiseGenerationStrategy)
        NoiseGenerationStrategy->EvaluatePoint(OutN, Height, Dummy);

    double SurfaceRadius = PlanetRadius + Height;

    OutSurfacePos = CurrentActorPosition + OutN * PlanetRadius;

    double DistanceToSurface = DistanceToCenter - SurfaceRadius;

    if (DistanceToCenter <= SurfaceRadius)
    {
        return 0.0;
    }

    return DistanceToSurface;
}

double UCosmicClipmapComponent::GetFastDistanceToSurface(
    FVector& OutViewerPos,
    FVector& OutSurfacePos,
    FVector& OutN)
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    OutViewerPos = GetPlayerLocation();
    CurrentActorPosition = Owner->GetActorLocation();

    FVector CenterToViewer = OutViewerPos - CurrentActorPosition;
    double DistanceToCenter = CenterToViewer.Length();

    OutN = CenterToViewer.GetSafeNormal();

    OutSurfacePos = CurrentActorPosition + OutN * PlanetRadius;

    double DistanceToSurface = DistanceToCenter - PlanetRadius;

    if (DistanceToCenter <= PlanetRadius)
    {
        return 0.0;
    }

    return DistanceToSurface;
}

float UCosmicClipmapComponent::GetDistanceToPlainSurface(FVector& OutViewerPos, FVector& OutSurfacePos, FVector& OutN)
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    // Viewer position
    OutViewerPos = GetPlayerLocation();

    // Plane base point
    CurrentActorPosition = Owner->GetActorLocation();

    // Plane normal
    OutN = FVector::UpVector;

    // Vector from plane to viewer
    FVector PlaneToViewer = OutViewerPos - CurrentActorPosition;

    // Signed distance to plane
    float Distance = FVector::DotProduct(PlaneToViewer, OutN);

    // Projection of viewer onto plane
    OutSurfacePos = OutViewerPos - Distance * OutN;

    return Distance;
}

int32 UCosmicClipmapComponent::CalculateDecreaseSteps(const double DistanceToSurface) const
{
    // must decrease at least 1, find minimum n such that
    // last ring is visible after n halvings
    int32 Steps = 1;
    const int64 LastResolution = Levels.Last()->Resolution;
    const int64 LastSpacing = Levels.Last()->GridSpacing;
    const int64 FirstSpacing = Levels[0]->GridSpacing;

    while (!IsClipmapRingVisible(LastSpacing >> Steps, LastResolution, DistanceToSurface))
    {
        // Do not decrease further if first level would reach minimum limit
        if ((FirstSpacing >> (Steps + 1)) <= MinTriangleSize)
            break;
        Steps++;
    }
    return Steps;
}

int32 UCosmicClipmapComponent::CalculateIncreaseSteps(const double DistanceToSurface) const
{
    // find how many consecutive doublings
    // remain visible without exceeding maximum allowed spacing
    int32 Steps = 1;
    const int64 LastResolution = Levels.Last()->Resolution;
    const int64 LastSpacing = Levels.Last()->GridSpacing;
    const int64 MaxSpacing = static_cast<int64>(BaseSpacing * FMath::Pow(2.0f, NumLevels - 1));

    while (IsClipmapRingVisible(LastSpacing << (Steps + 1), LastResolution, DistanceToSurface)
        && (LastSpacing << Steps) < MaxSpacing)
    {
        Steps++;
    }
    return Steps;
}


bool UCosmicClipmapComponent::IsClipmapRingVisible(const int32 LevelIndex, const double DistanceToSurface) const
{  
    
    // Calculate clipmap radius on surface
    int64 ClipmapSurfaceRadius = Levels[LevelIndex]->GridSpacing * (Levels[LevelIndex]->Resolution - 2) / 2;

    // Maximum visible radius from this altitude (projection on surface)
    double VisibleRadius = PlanetRadius * FMath::Sin(FMath::Acos(PlanetRadius / (PlanetRadius + DistanceToSurface)));

    // Clipmap is visible if its radius is smaller than visible radius
    return ClipmapSurfaceRadius <= VisibleRadius * 2.f; 
}

bool UCosmicClipmapComponent::IsClipmapRingVisible(const int64 GridSpacing, const int64 Resolution, const double DistanceToSurface) const
{
    int64 ClipmapSurfaceRadius = GridSpacing * (Resolution - 2) / 2;

    // Maximum visible radius from this altitude (projection on surface)
    double VisibleRadius = PlanetRadius * FMath::Sin(FMath::Acos(PlanetRadius / (PlanetRadius + DistanceToSurface)));

    // Clipmap is visible if its radius is smaller than visible radius
    return ClipmapSurfaceRadius <= VisibleRadius * 2.f;
}


void UCosmicClipmapComponent::DecreaseClipmapLevelFull(int32 Steps)
{
    if (NumLevels <= 1 || Steps <= 0) return;

    // Apply all halvings to BaseGridSpacing at once
    const int64 Divisor = static_cast<int64>(1) << Steps; // 2^Steps
    BaseGridSpacing /= Divisor;
    TotalShift *= Divisor;

    // Rebuild spacings from new base
    float NewGridSpacing = BaseGridSpacing;
    for (int32 i = 0; i < NumLevels; i++)
    {
        Levels[i]->ReScaleLevel(NewGridSpacing);
        NewGridSpacing *= 2.f;
    }
}

void UCosmicClipmapComponent::IncreaseClipmapLevelFull(int32 Steps)
{
    if (NumLevels <= 1 || Steps <= 0) return;

    const int64 Multiplier = static_cast<int64>(1) << Steps; // 2^Steps
    BaseGridSpacing *= Multiplier;
    TotalShift /= Multiplier;

    // Rebuild spacings from new base
    float NewGridSpacing = BaseGridSpacing;
    for (int32 i = 0; i < NumLevels; i++)
    {
        Levels[i]->ReScaleLevel(NewGridSpacing);
        NewGridSpacing *= 2.f;
    }
}





