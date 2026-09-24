// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Terrain/CosmicOceanComponent.h"
#include "Terrain/CosmicMeshComponent.h"
#include "Terrain/CosmicClipmapGeometry.h"
#include "Terrain/CosmicOceanGenerationTask.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UCosmicOceanComponent::UCosmicOceanComponent()
{
    bTickInEditor = true;
    PrimaryComponentTick.bCanEverTick = true;

    static ConstructorHelpers::FObjectFinder<UMaterialInstance> OceanMaterialAsset(
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/Ocean/MI_CosmicOceanV3.MI_CosmicOceanV3")
    );
    if (OceanMaterialAsset.Succeeded())
    {
        OceanMaterial = OceanMaterialAsset.Object;
    }
}

void UCosmicOceanComponent::InitOcean(double PlanetRadiusKm, USceneComponent* Parent)
{
    if (IsRunningDedicatedServer() || (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer))
    {
        return;
    }

    PlanetRadiusCm = PlanetRadiusKm * 100000.0;
    ParentRoot = Parent;
}

int64 UCosmicOceanComponent::GetCalculatedBaseGridSpacing() const
{
    if (bAutoCalculateGridSpacing)
    {
        const int32 SafeLevels = FMath::Clamp(OceanNumLevels, 1, 8);
        const int32 SafeRes = FMath::Max(8, OceanResolution);
        const double Denominator = SafeRes * FMath::Pow(2.0f, SafeLevels - 1);
        const int64 Spacing = static_cast<int64>((PlanetRadiusCm * 2.0) / Denominator);
        return FMath::Max(static_cast<int64>(MinTriangleSize), Spacing);
    }

    return FMath::Max(static_cast<int64>(MinTriangleSize), OceanBaseGridSpacing);
}

void UCosmicOceanComponent::RegenerateOcean()
{
    if (IsRunningDedicatedServer() || (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer))
    {
        return;
    }

    if (bInit)
    {
        ClearOcean();
    }

    if (!bHasOcean || !ParentRoot)
    {
        return;
    }

    BuildDynamicMaterial();
    BuildNearOceanMesh();
    BuildFarOceanMesh();

    bInit = true;

    SetPerformanceMode(bPerformanceMode);
}

void UCosmicOceanComponent::ClearOcean()
{
    CancelAsyncWork();

    if (NearOceanMesh)
    {
        NearOceanMesh->ClearAllMeshSections();
        NearOceanMesh->CancelAsyncWork();
        NearOceanMesh->DestroyComponent();
        NearOceanMesh = nullptr;
    }

    if (FarOceanMesh)
    {
        FarOceanMesh->ClearAllMeshSections();
        FarOceanMesh->CancelAsyncWork();
        FarOceanMesh->DestroyComponent();
        FarOceanMesh = nullptr;
    }

    DynamicOceanMat = nullptr;
    DynamicFarOceanMat = nullptr;
    bInit = false;
    bPerformanceMode = true;
    bNearMeshPositioned = false;
    AppliedCoarsestCenter = FIntPoint(MAX_int32, MAX_int32);
    AppliedProjectionRevision = MAX_uint64;
    CurrentOceanBaseGridSpacing = GetCalculatedBaseGridSpacing();
    AppliedBaseGridSpacing = CurrentOceanBaseGridSpacing;
    CurrentDistanceToSurface = -1.0;
    LastAppliedDistanceToSurface = -1.0;
    CurrentViewerCoordinates = FVector2D::ZeroVector;
}

void UCosmicOceanComponent::ResetPointersAfterDuplicate(USceneComponent* NewRoot)
{
    CancelAsyncWork();
    ParentRoot = NewRoot;
    NearOceanMesh = nullptr;
    FarOceanMesh = nullptr;
    DynamicOceanMat = nullptr;
    DynamicFarOceanMat = nullptr;
    bInit = false;
    bPerformanceMode = true;
    bNearMeshPositioned = false;
    AppliedCoarsestCenter = FIntPoint(MAX_int32, MAX_int32);
    AppliedProjectionRevision = MAX_uint64;
    CurrentOceanBaseGridSpacing = GetCalculatedBaseGridSpacing();
    AppliedBaseGridSpacing = CurrentOceanBaseGridSpacing;
    CurrentDistanceToSurface = -1.0;
    LastAppliedDistanceToSurface = -1.0;
    CurrentViewerCoordinates = FVector2D::ZeroVector;
}

void UCosmicOceanComponent::BuildNearOceanMesh()
{
    if (NearOceanMesh)
    {
        NearOceanMesh->ClearAllMeshSections();
        NearOceanMesh->CancelAsyncWork();
        NearOceanMesh->DestroyComponent();
        NearOceanMesh = nullptr;
    }

    if (!bHasOcean || !ParentRoot) return;

    FName ComponentName = *FString::Printf(TEXT("TerrainOceanMesh_NearClipmap"));
    NearOceanMesh = NewObject<UCosmicMeshComponent>(GetOwner(), ComponentName, RF_Transient | RF_DuplicateTransient);
    if (!NearOceanMesh) return;

    NearOceanMesh->RegisterComponent();
    NearOceanMesh->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);

    const int32 NumLevels = FMath::Clamp(OceanNumLevels, 1, 8);
    int32 Res = FMath::Clamp(OceanResolution, 8, 256);
    if (Res % 4 != 0)
    {
        Res += 4 - (Res % 4);
    }
    OceanResolution = Res;

    const int32 VertRes = Res + 1;
    const int32 VertsPerLevel = VertRes * VertRes;
    const int32 TotalVerts = NumLevels * VertsPerLevel;
    const int32 HalfRes = Res / 2;

    const double EffectiveRadius = PlanetRadiusCm + SeaLevelKm * 100000.0;
    CurrentOceanBaseGridSpacing = GetCalculatedBaseGridSpacing();
    AppliedBaseGridSpacing = CurrentOceanBaseGridSpacing;
    const int64 BaseGridSpacing = CurrentOceanBaseGridSpacing;

    // Determine initial patch transform
    FVector AnchorNormal = CurrentProjectionFrame.GetUnitAxis(EAxis::Z);
    if (AnchorNormal.IsNearlyZero())
    {
        AnchorNormal = CurrentProjectionFrame.GetTranslation().GetSafeNormal();
    }
    if (AnchorNormal.IsNearlyZero())
    {
        AnchorNormal = FVector::UpVector;
    }
    const FRotator PatchRotation = CurrentProjectionFrame.GetRotation().Rotator();
    const FVector SurfacePos = AnchorNormal * EffectiveRadius;
    const FTransform PatchTransform(PatchRotation, SurfacePos, FVector::OneVector);
    const FMatrix TransformMatrix = PatchTransform.ToMatrixWithScale();
    const FVector SphereCenter(0.0, 0.0, -EffectiveRadius);

    TArray<FVector> InitialVertices;
    TArray<FVector> InitialNormals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;
    TArray<int32> Triangles;

    InitialVertices.Reserve(TotalVerts);
    InitialNormals.Reserve(TotalVerts);
    UVs.Reserve(TotalVerts);
    Tangents.Reserve(TotalVerts);

    // Build vertices, normals, tangents, UVs for each LOD level
    for (int32 L = 0; L < NumLevels; ++L)
    {
        const int64 LevelSpacing = BaseGridSpacing * (1LL << L);
        const int64 LevelMultiplier = 1LL << ((NumLevels - 1) - L);
        const FIntPoint LevelCenter(
            static_cast<int32>(static_cast<int64>(CurrentCoarsestCenter.X) * LevelMultiplier),
            static_cast<int32>(static_cast<int64>(CurrentCoarsestCenter.Y) * LevelMultiplier)
        );

        for (int32 y = 0; y < VertRes; ++y)
        {
            for (int32 x = 0; x < VertRes; ++x)
            {
                const double WorldX = (LevelCenter.X + (x - HalfRes)) * LevelSpacing;
                const double WorldY = (LevelCenter.Y + (y - HalfRes)) * LevelSpacing;

                FVector BasePosition;
                FVector Normal;
                FVector TangentDir;
                FCosmicClipmapGeometry::ProjectPlanarGridPointToSphere(
                    WorldX, WorldY, EffectiveRadius, BasePosition, Normal, TangentDir);

                // Transform to planet / component space
                InitialVertices.Add(TransformMatrix.TransformPosition(BasePosition));
                InitialNormals.Add(TransformMatrix.TransformVector(Normal).GetSafeNormal());

                const FVector RotatedTangent = TransformMatrix.TransformVector(TangentDir).GetSafeNormal();
                Tangents.Add(FProcMeshTangent(RotatedTangent.X, RotatedTangent.Y, RotatedTangent.Z));

                UVs.Add(FVector2D((float)x / Res, (float)y / Res));
            }
        }
    }

    // Build triangles matching CosmicMeshComponent::BuildBaseProjectedMesh exactly
    Triangles.Reserve(NumLevels * Res * Res * 6);
    for (int32 L = 0; L < NumLevels; ++L)
    {
        const int32 LevelOffset = L * VertsPerLevel;
        const bool bIsRing = (L > 0);
        FCosmicClipmapGeometry::GenerateClipmapLevelTriangles(Res, bIsRing, LevelOffset, Triangles);
    }

    NearOceanMesh->CreateMeshSection_LinearColor(
        0,
        InitialVertices,
        Triangles,
        InitialNormals,
        UVs,
        TArray<FLinearColor>(),
        Tangents,
        false
    );

    NearOceanMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    NearOceanMesh->SetMeshActive(!bPerformanceMode);

    if (DynamicOceanMat)
    {
        NearOceanMesh->SetMaterial(0, DynamicOceanMat);
    }
}

void UCosmicOceanComponent::BuildFarOceanMesh()
{
    if (FarOceanMesh)
    {
        FarOceanMesh->ClearAllMeshSections();
        FarOceanMesh->CancelAsyncWork();
        FarOceanMesh->DestroyComponent();
        FarOceanMesh = nullptr;
    }

    if (!bHasOcean || !ParentRoot) return;

    FName ComponentName = *FString::Printf(TEXT("TerrainOceanMesh_FarSphere"));
    FarOceanMesh = NewObject<UCosmicMeshComponent>(GetOwner(), ComponentName, RF_Transient | RF_DuplicateTransient);
    if (!FarOceanMesh) return;

    FarOceanMesh->RegisterComponent();
    FarOceanMesh->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);

    FarOceanMesh->Resolution = FMath::Clamp(FarSphereResolution, 16, 256);
    FarOceanMesh->bIsRing = false;
    FarOceanMesh->PlanetRadius = PlanetRadiusCm + SeaLevelKm * 100000.0;
    FarOceanMesh->bIsPlanet = false;

    FarOceanMesh->BuildSphereMesh();
    FarOceanMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FarOceanMesh->SetMeshActive(bPerformanceMode);

    if (DynamicFarOceanMat)
    {
        FarOceanMesh->SetMaterial(0, DynamicFarOceanMat);
    }
    else if (DynamicOceanMat)
    {
        FarOceanMesh->SetMaterial(0, DynamicOceanMat);
    }
}

void UCosmicOceanComponent::BuildDynamicMaterial()
{
    UMaterialInterface* BaseMaterial = OceanMaterial;

    if (!BaseMaterial)
    {
        // Fallback in case OceanMaterial was cleared or not yet assigned
        const TCHAR* CandidatePaths[] = {
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/Ocean/MI_CosmicOceanV3.MI_CosmicOceanV3"),
            TEXT("/CosmicArchitect/Resources/Materials/Ocean/MI_CosmicOceanV3.MI_CosmicOceanV3"),
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/Ocean/MI_CosmicOceanV2.MI_CosmicOceanV2")
        };

        for (const TCHAR* Path : CandidatePaths)
        {
            BaseMaterial = LoadObject<UMaterialInterface>(nullptr, Path);
            if (BaseMaterial)
            {
                OceanMaterial = Cast<UMaterialInstance>(BaseMaterial);
                break;
            }
        }
    }

    if (BaseMaterial)
    {
        DynamicOceanMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
        DynamicFarOceanMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    }
    else
    {
        DynamicOceanMat = nullptr;
        DynamicFarOceanMat = nullptr;
    }

    if (NearOceanMesh && DynamicOceanMat)
    {
        NearOceanMesh->SetMaterial(0, DynamicOceanMat);
    }

    if (FarOceanMesh && DynamicFarOceanMat)
    {
        FarOceanMesh->SetMaterial(0, DynamicFarOceanMat);
    }
    else if (FarOceanMesh && DynamicOceanMat)
    {
        FarOceanMesh->SetMaterial(0, DynamicOceanMat);
    }

    UpdateWaveParameters();
}

void UCosmicOceanComponent::UpdateWaveParameters()
{
    const double EffectiveRadius = PlanetRadiusCm + SeaLevelKm * 100000.0;
    const FVector OwnerLocation = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;

    //Near Clipmap Ocean Material
    if (DynamicOceanMat)
    {
        DynamicOceanMat->SetScalarParameterValue(FName("PlanetRadius"), static_cast<float>(EffectiveRadius));
        DynamicOceanMat->SetVectorParameterValue(FName("PlanetCenter"), OwnerLocation);

        // Wave parameters (M_CosmicOceanV3 / SphericalGerstner)
        DynamicOceanMat->SetScalarParameterValue(FName("WaveHeight"), WaveHeight);
        DynamicOceanMat->SetScalarParameterValue(FName("WaveLength"), WaveLength);
        DynamicOceanMat->SetScalarParameterValue(FName("WaveSpeed"), WaveSpeed);
        DynamicOceanMat->SetScalarParameterValue(FName("WaveSteepness"), WaveSteepness);
        DynamicOceanMat->SetScalarParameterValue(FName("WaveChop"), WaveChop);
        DynamicOceanMat->SetScalarParameterValue(FName("WaveCount"), WaveCount);
        DynamicOceanMat->SetScalarParameterValue(FName("WaveSpread"), WaveSpread);
        DynamicOceanMat->SetVectorParameterValue(FName("WindDirection"), WindDirection);

        // Distance Fade Range (in centimeters for UE material parameters)
        DynamicOceanMat->SetScalarParameterValue(FName("WaveFadeStart"), WaveFadeStartKm * 100000.0f);
        DynamicOceanMat->SetScalarParameterValue(FName("WaveFadeEnd"), WaveFadeEndKm * 100000.0f);
        DynamicOceanMat->SetScalarParameterValue(FName("DistanceWaterBegin"), WaveFadeStartKm * 100000.0f);
        DynamicOceanMat->SetScalarParameterValue(FName("DistanceWaterFade"), (WaveFadeEndKm - WaveFadeStartKm) * 100000.0f);

        // Appearance / SingleLayerWater
        DynamicOceanMat->SetVectorParameterValue(FName("WaterColor"), WaterColor);
        DynamicOceanMat->SetVectorParameterValue(FName("WaterAbsortion"), WaterAbsortion);
        DynamicOceanMat->SetVectorParameterValue(FName("WaterScattering"), WaterScattering);
        DynamicOceanMat->SetScalarParameterValue(FName("WaterScatteringAmount"), WaterScatteringAmount);
        DynamicOceanMat->SetScalarParameterValue(FName("WaterRoughness"), WaterRoughness);
    }

    //Distant Sphere Ocean Material
    if (DynamicFarOceanMat)
    {
        DynamicFarOceanMat->SetScalarParameterValue(FName("PlanetRadius"), static_cast<float>(EffectiveRadius));
        DynamicFarOceanMat->SetVectorParameterValue(FName("PlanetCenter"), OwnerLocation);

        // Force wave displacement to ZERO on the distant global sphere
        DynamicFarOceanMat->SetScalarParameterValue(FName("WaveHeight"), 0.0f);
        DynamicFarOceanMat->SetScalarParameterValue(FName("WaveCount"), 0.0f);
        DynamicFarOceanMat->SetScalarParameterValue(FName("WaveSteepness"), 0.0f);
        DynamicFarOceanMat->SetScalarParameterValue(FName("WaveChop"), 0.0f);
        DynamicFarOceanMat->SetScalarParameterValue(FName("WaveSpeed"), 0.0f);
        DynamicFarOceanMat->SetScalarParameterValue(FName("WaveLength"), WaveLength);
        DynamicFarOceanMat->SetScalarParameterValue(FName("WaveSpread"), WaveSpread);
        DynamicFarOceanMat->SetVectorParameterValue(FName("WindDirection"), WindDirection);

        // Appearance matches the near ocean seamlessly
        DynamicFarOceanMat->SetVectorParameterValue(FName("WaterColor"), WaterColor);
        DynamicFarOceanMat->SetVectorParameterValue(FName("WaterAbsortion"), WaterAbsortion);
        DynamicFarOceanMat->SetVectorParameterValue(FName("WaterScattering"), WaterScattering);
        DynamicFarOceanMat->SetScalarParameterValue(FName("WaterScatteringAmount"), WaterScatteringAmount);
        DynamicFarOceanMat->SetScalarParameterValue(FName("WaterRoughness"), WaterRoughness);
    }
}

void UCosmicOceanComponent::UpdateOceanLOD(
    const FTransform& InProjectionFrame,
    const FIntPoint& InCoarsestCenter,
    uint64 InProjectionRevision,
    bool bInPerformanceMode,
    double InDistanceToSurface,
    const FVector2D& InViewerCoordinates)
{
    if (!bHasOcean || !bInit) return;

    if (bPerformanceMode != bInPerformanceMode)
    {
        SetPerformanceMode(bInPerformanceMode);
    }

    if (bPerformanceMode)
    {
        return;
    }

    // Compute the expected ocean center in ocean coarsest grid units
    const int32 NumLevels = FMath::Clamp(OceanNumLevels, 1, 8);
    const int64 OceanCoarsestSpacing = CurrentOceanBaseGridSpacing * (1LL << (NumLevels - 1));

    FIntPoint OceanCoarsestCenter = InCoarsestCenter;
    if (OceanCoarsestSpacing > 0 && !InViewerCoordinates.IsZero())
    {
        OceanCoarsestCenter = FIntPoint(
            FMath::RoundToInt(InViewerCoordinates.X / static_cast<double>(OceanCoarsestSpacing)),
            FMath::RoundToInt(InViewerCoordinates.Y / static_cast<double>(OceanCoarsestSpacing))
        );
    }

    const bool bCenterOrRevisionChanged =
        InProjectionRevision != AppliedProjectionRevision ||
        OceanCoarsestCenter != AppliedCoarsestCenter;

    bool bDistanceChangedSignificantly = false;
    if (InDistanceToSurface >= 0.0)
    {
        if (LastAppliedDistanceToSurface < 0.0)
        {
            bDistanceChangedSignificantly = true;
        }
        else
        {
            const double DeltaDist = FMath::Abs(InDistanceToSurface - LastAppliedDistanceToSurface);
            const double RefDist = FMath::Max(100.0, FMath::Min(InDistanceToSurface, LastAppliedDistanceToSurface));
            if (DeltaDist / RefDist > 0.15)
            {
                bDistanceChangedSignificantly = true;
            }
        }
    }

    if (bCenterOrRevisionChanged || bDistanceChangedSignificantly)
    {
        CurrentProjectionFrame = InProjectionFrame;
        CurrentCoarsestCenter = OceanCoarsestCenter;
        CurrentProjectionRevision = InProjectionRevision;
        CurrentDistanceToSurface = InDistanceToSurface;
        CurrentViewerCoordinates = InViewerCoordinates;

        RequestOceanMeshUpdate();
    }
}

void UCosmicOceanComponent::SetPerformanceMode(bool bActive)
{
    bPerformanceMode = bActive;

    if (bPerformanceMode)
    {
        CancelAsyncWork();
        bNearMeshPositioned = false;
        if (NearOceanMesh)
        {
            NearOceanMesh->SetMeshActive(false);
        }
        if (FarOceanMesh)
        {
            FarOceanMesh->SetMeshActive(true);
        }
    }
    else
    {
        // Keep FarOceanMesh visible until NearOceanMesh receives its first
        // updated position from the async task, preventing wrong-position pop-in.
        if (!bNearMeshPositioned)
        {
            if (FarOceanMesh)
            {
                FarOceanMesh->SetMeshActive(true);
            }
            if (NearOceanMesh)
            {
                NearOceanMesh->SetMeshActive(false);
            }
        }
        else
        {
            if (FarOceanMesh)
            {
                FarOceanMesh->SetMeshActive(false);
            }
            if (NearOceanMesh)
            {
                NearOceanMesh->SetMeshActive(true);
            }
        }
        RequestOceanMeshUpdate();
    }
}

void UCosmicOceanComponent::RequestOceanMeshUpdate()
{
    if (!NearOceanMesh || bIsGeneratingOcean || bPerformanceMode) return;

    const double EffectiveRadius = PlanetRadiusCm + SeaLevelKm * 100000.0;

    FVector AnchorNormal = CurrentProjectionFrame.GetUnitAxis(EAxis::Z);
    if (AnchorNormal.IsNearlyZero())
    {
        AnchorNormal = CurrentProjectionFrame.GetTranslation().GetSafeNormal();
    }
    if (AnchorNormal.IsNearlyZero())
    {
        AnchorNormal = FVector::UpVector;
    }

    const FRotator PatchRotation = CurrentProjectionFrame.GetRotation().Rotator();
    const FVector SurfacePos = AnchorNormal * EffectiveRadius;
    const FTransform PatchTransform(PatchRotation, SurfacePos, FVector::OneVector);

    FCosmicOceanClipmapSettings Settings;
    Settings.NumLevels = FMath::Clamp(OceanNumLevels, 1, 8);
    Settings.Resolution = OceanResolution;
    Settings.BaseGridSpacing = CurrentOceanBaseGridSpacing;
    Settings.MaxBaseGridSpacing = GetCalculatedBaseGridSpacing();
    Settings.MinTriangleSize = FMath::Max(10, MinTriangleSize);
    Settings.OceanRadius = EffectiveRadius;
    Settings.DistanceToSurface = CurrentDistanceToSurface;
    Settings.PatchTransform = PatchTransform;
    Settings.CoarsestGridCenter = CurrentCoarsestCenter;
    Settings.ViewerCoordinates = CurrentViewerCoordinates;
    Settings.bUseViewerCoordinates = !CurrentViewerCoordinates.IsZero();
    Settings.ProjectionRevision = CurrentProjectionRevision;

    bIsGeneratingOcean = true;
    OceanTask = new FAsyncTask<FCosmicOceanGenerationTask>(MoveTemp(Settings));
    OceanTask->StartBackgroundTask();
}

bool UCosmicOceanComponent::CheckAndApplyOceanMeshUpdate()
{
    if (!OceanTask || !OceanTask->IsDone()) return false;

    FCosmicOceanGenerationTask& CompletedTask = OceanTask->GetTask();

    AppliedCoarsestCenter = CompletedTask.CalculatedGridCenter;
    CurrentCoarsestCenter = CompletedTask.CalculatedGridCenter;
    AppliedProjectionRevision = CompletedTask.CalculatedProjectionRevision;
    AppliedBaseGridSpacing = CompletedTask.CalculatedBaseGridSpacing;
    CurrentOceanBaseGridSpacing = CompletedTask.CalculatedBaseGridSpacing;
    LastAppliedDistanceToSurface = CurrentDistanceToSurface;

    if (NearOceanMesh && !bPerformanceMode)
    {
        NearOceanMesh->UpdateMeshSection_LinearColor(
            0,
            CompletedTask.CalculatedVertices,
            CompletedTask.CalculatedNormals,
            TArray<FVector2D>(),
            TArray<FLinearColor>(),
            TArray<FProcMeshTangent>()
        );

        // Only swap to NearOceanMesh after its vertices have been correctly positioned
        if (!bNearMeshPositioned)
        {
            bNearMeshPositioned = true;
            NearOceanMesh->SetMeshActive(true);
            if (FarOceanMesh)
            {
                FarOceanMesh->SetMeshActive(false);
            }
        }
    }

    delete OceanTask;
    OceanTask = nullptr;
    bIsGeneratingOcean = false;

    return true;
}

bool UCosmicOceanComponent::IsTaskActive() const
{
    return OceanTask && !OceanTask->IsDone();
}

void UCosmicOceanComponent::CancelAsyncWork()
{
    bIsGeneratingOcean = false;
    if (OceanTask)
    {
        if (OceanTask->Cancel() || OceanTask->IsDone())
        {
            delete OceanTask;
        }
        else
        {
            OceanTask->EnsureCompletion();
            delete OceanTask;
        }
        OceanTask = nullptr;
    }
}

void UCosmicOceanComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelAsyncWork();
    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void UCosmicOceanComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    const FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // Wave and appearance parameters (update MID directly without rebuilding geometry or reregistering components)
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveHeight) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveLength) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveSpeed) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveSteepness) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveChop) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveCount) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveSpread) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveFadeStartKm) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveFadeEndKm) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WindDirection) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaterColor) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaterAbsortion) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaterScattering) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaterScatteringAmount) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaterRoughness))
    {
        UpdateWaveParameters();
        return;
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, bHasOcean))
    {
        bHasOcean ? RegenerateOcean() : ClearOcean();
        return;
    }

    // Geometry parameters requiring full reconstruction
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, OceanResolution) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, OceanNumLevels) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, MinTriangleSize) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, OceanBaseGridSpacing) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, bAutoCalculateGridSpacing) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, FarSphereResolution) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, SeaLevelKm))
    {
        RegenerateOcean();
        return;
    }

    // Material override change
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, OceanMaterial))
    {
        BuildDynamicMaterial();
        return;
    }
}
#endif

void UCosmicOceanComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bHasOcean || !bInit) return;

    if (GetOwner())
    {
        const FVector OwnerLocation = GetOwner()->GetActorLocation();
        const double EffectiveRadius = PlanetRadiusCm + SeaLevelKm * 100000.0;

        if (DynamicOceanMat)
        {
            DynamicOceanMat->SetVectorParameterValue(FName("PlanetCenter"), OwnerLocation);
            DynamicOceanMat->SetScalarParameterValue(FName("PlanetRadius"), static_cast<float>(EffectiveRadius));
        }

        if (DynamicFarOceanMat)
        {
            DynamicFarOceanMat->SetVectorParameterValue(FName("PlanetCenter"), OwnerLocation);
            DynamicFarOceanMat->SetScalarParameterValue(FName("PlanetRadius"), static_cast<float>(EffectiveRadius));
        }
    }

    if (bAutoApplyInTick)
    {
        CheckAndApplyOceanMeshUpdate();
    }
}
