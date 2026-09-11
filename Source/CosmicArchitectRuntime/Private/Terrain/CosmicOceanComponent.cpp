// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Terrain/CosmicOceanComponent.h"
#include "Terrain/CosmicMeshComponent.h"
#include "Terrain/CosmicOceanGenerationTask.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"

UCosmicOceanComponent::UCosmicOceanComponent()
{
    bTickInEditor = true;
    PrimaryComponentTick.bCanEverTick = true;
}

void UCosmicOceanComponent::InitOcean(double PlanetRadiusKm, USceneComponent* Parent)
{
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
    bInit = false;
    bPerformanceMode = true;
    bNearMeshPositioned = false;
    AppliedCoarsestCenter = FIntPoint(MAX_int32, MAX_int32);
    AppliedProjectionRevision = MAX_uint64;
}

void UCosmicOceanComponent::ResetPointersAfterDuplicate(USceneComponent* NewRoot)
{
    CancelAsyncWork();
    ParentRoot = NewRoot;
    NearOceanMesh = nullptr;
    FarOceanMesh = nullptr;
    DynamicOceanMat = nullptr;
    bInit = false;
    bPerformanceMode = true;
    bNearMeshPositioned = false;
    AppliedCoarsestCenter = FIntPoint(MAX_int32, MAX_int32);
    AppliedProjectionRevision = MAX_uint64;
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
    const int64 BaseGridSpacing = GetCalculatedBaseGridSpacing();

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
                // Exact formula from CosmicMeshComponent::BuildBaseProjectedMesh
                const double WorldX = (LevelCenter.X + (x - HalfRes)) * LevelSpacing;
                const double WorldY = (LevelCenter.Y + (y - HalfRes)) * LevelSpacing;
                const double Distance2D = FMath::Sqrt(WorldX * WorldX + WorldY * WorldY);
                FVector BasePosition;

                if (Distance2D <= EffectiveRadius && Distance2D > 0.001)
                {
                    const double ZOffset = FMath::Sqrt(EffectiveRadius * EffectiveRadius - Distance2D * Distance2D);
                    BasePosition = FVector(WorldX, WorldY, -EffectiveRadius + ZOffset);
                }
                else if (Distance2D <= 0.001)
                {
                    BasePosition = FVector::ZeroVector;
                }
                else
                {
                    const double Scale = EffectiveRadius / Distance2D;
                    BasePosition = FVector(WorldX * Scale, WorldY * Scale, -EffectiveRadius);
                }

                // Normal from local sphere center
                FVector Normal = (BasePosition - SphereCenter);
                if (Normal.SizeSquared() > 0.001)
                {
                    Normal.Normalize();
                }
                else
                {
                    Normal = FVector::UpVector;
                }

                // Tangent
                FVector TangentDir = FVector(-Normal.Y, Normal.X, 0.0);
                if (TangentDir.SizeSquared() > 0.001)
                {
                    TangentDir.Normalize();
                }
                else
                {
                    TangentDir = FVector(1.0, 0.0, 0.0);
                }

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
    for (int32 L = 0; L < NumLevels; ++L)
    {
        const int32 LevelOffset = L * VertsPerLevel;
        const int32 LevelMaxVertex = LevelOffset + VertsPerLevel;
        const bool bIsRing = (L > 0);

        for (int32 y = 0; y < Res; ++y)
        {
            for (int32 x = 0; x < Res; ++x)
            {
                const int32 i0 = LevelOffset + y * VertRes + x;
                const int32 i1 = i0 + 1;
                const int32 i2 = i0 + VertRes;
                const int32 i3 = i2 + 1;

                if (bIsRing)
                {
                    const bool bInsideInner =
                        x > HalfRes / 2 &&
                        x < Res - HalfRes / 2 &&
                        y > HalfRes / 2 &&
                        y < Res - HalfRes / 2;

                    if (bInsideInner)
                    {
                        continue;
                    }
                }

                if (i0 >= TotalVerts || i1 >= TotalVerts ||
                    i2 >= TotalVerts || i3 >= TotalVerts)
                {
                    continue;
                }

                const bool bBorder =
                    (x == 0) ||
                    (x == Res - 1) ||
                    (y == 0) ||
                    (y == Res - 1);

                // LEVEL BORDER STITCHING (2:1 quad transition)
                if (bBorder)
                {
                    // Horizontal borders
                    if ((y == 0 || y == Res - 1) && (x % 2 == 0) && x < Res - 1)
                    {
                        const int32 i4 = i1 + 1;
                        const int32 i5 = i3 + 1;

                        if (i4 < LevelMaxVertex && i5 < LevelMaxVertex)
                        {
                            if (y == Res - 1) // Bottom border
                            {
                                if (x != Res - 2)
                                {
                                    Triangles.Add(i1);
                                    Triangles.Add(i5);
                                    Triangles.Add(i4);
                                }

                                if (x != 0)
                                {
                                    Triangles.Add(i1);
                                    Triangles.Add(i0);
                                    Triangles.Add(i2);
                                }

                                Triangles.Add(i2);
                                Triangles.Add(i5);
                                Triangles.Add(i1);
                            }
                            else // Top border
                            {
                                if (x != 0)
                                {
                                    Triangles.Add(i0);
                                    Triangles.Add(i2);
                                    Triangles.Add(i3);
                                }

                                if (x != Res - 2)
                                {
                                    Triangles.Add(i3);
                                    Triangles.Add(i5);
                                    Triangles.Add(i4);
                                }

                                Triangles.Add(i0);
                                Triangles.Add(i3);
                                Triangles.Add(i4);
                            }
                        }
                    }
                    // Vertical borders
                    else if ((x == 0 || x == Res - 1) && (y % 2 == 0) && y < Res - 1)
                    {
                        const int32 i4 = i2 + VertRes;
                        const int32 i5 = i3 + VertRes;

                        if (i4 < LevelMaxVertex && i5 < LevelMaxVertex)
                        {
                            if (x == Res - 1) // Right border
                            {
                                Triangles.Add(i1);
                                Triangles.Add(i2);
                                Triangles.Add(i5);

                                if (y != 0)
                                {
                                    Triangles.Add(i2);
                                    Triangles.Add(i1);
                                    Triangles.Add(i0);
                                }

                                if (y != Res - 2)
                                {
                                    Triangles.Add(i2);
                                    Triangles.Add(i4);
                                    Triangles.Add(i5);
                                }
                            }
                            else // Left border
                            {
                                if (y != 0)
                                {
                                    Triangles.Add(i0);
                                    Triangles.Add(i3);
                                    Triangles.Add(i1);
                                }

                                if (y != Res - 2)
                                {
                                    Triangles.Add(i3);
                                    Triangles.Add(i4);
                                    Triangles.Add(i5);
                                }

                                Triangles.Add(i0);
                                Triangles.Add(i4);
                                Triangles.Add(i3);
                            }
                        }
                    }
                }
                else
                {
                    // NORMAL INTERIOR (Exact winding matching CosmicMeshComponent)
                    Triangles.Add(i0);
                    Triangles.Add(i2);
                    Triangles.Add(i1);

                    Triangles.Add(i1);
                    Triangles.Add(i2);
                    Triangles.Add(i3);
                }
            }
        }
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

    if (DynamicOceanMat)
    {
        FarOceanMesh->SetMaterial(0, DynamicOceanMat);
    }
}

void UCosmicOceanComponent::BuildDynamicMaterial()
{
    UMaterialInterface* BaseMaterial = nullptr;

    if (bUseGeneratedMaterial)
    {
        // Try to load M_CosmicOceanV3 instance or master material (both plugin mount variants)
        const TCHAR* CandidatePaths[] = {
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicOceanV3.MI_CosmicOceanV3"),
            TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicOceanV3.MI_CosmicOceanV3"),
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicOceanV3.M_CosmicOceanV3"),
            TEXT("/CosmicArchitect/Resources/Materials/M_CosmicOceanV3.M_CosmicOceanV3"),
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicOceanV2.MI_CosmicOceanV2"),
            TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicOceanGerstner.MI_CosmicOceanGerstner")
        };

        for (const TCHAR* Path : CandidatePaths)
        {
            BaseMaterial = LoadObject<UMaterialInterface>(nullptr, Path);
            if (BaseMaterial)
            {
                break;
            }
        }

        if (!BaseMaterial)
        {
            BaseMaterial = OceanMaterial;
        }
    }
    else
    {
        BaseMaterial = OceanMaterial;
    }

    if (BaseMaterial)
    {
        DynamicOceanMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    }
    else
    {
        DynamicOceanMat = nullptr;
    }

    if (NearOceanMesh && DynamicOceanMat)
    {
        NearOceanMesh->SetMaterial(0, DynamicOceanMat);
    }

    if (FarOceanMesh && DynamicOceanMat)
    {
        FarOceanMesh->SetMaterial(0, DynamicOceanMat);
    }

    if (DynamicOceanMat && bUseGeneratedMaterial)
    {
        UpdateWaveParameters();
    }
}

void UCosmicOceanComponent::UpdateWaveParameters()
{
    if (!DynamicOceanMat || !bUseGeneratedMaterial) return;

    const double EffectiveRadius = PlanetRadiusCm + SeaLevelKm * 100000.0;
    DynamicOceanMat->SetScalarParameterValue(FName("PlanetRadius"), static_cast<float>(EffectiveRadius));

    if (GetOwner())
    {
        DynamicOceanMat->SetVectorParameterValue(FName("PlanetCenter"), GetOwner()->GetActorLocation());
    }

    // Wave parameters (M_CosmicOceanV3 / SphericalGerstner)
    DynamicOceanMat->SetScalarParameterValue(FName("WaveHeight"), WaveHeight);
    DynamicOceanMat->SetScalarParameterValue(FName("WaveLength"), WaveLength);
    DynamicOceanMat->SetScalarParameterValue(FName("WaveSpeed"), WaveSpeed);
    DynamicOceanMat->SetScalarParameterValue(FName("WaveSteepness"), WaveSteepness);
    DynamicOceanMat->SetScalarParameterValue(FName("WaveChop"), WaveChop);
    DynamicOceanMat->SetScalarParameterValue(FName("WaveCount"), WaveCount);
    DynamicOceanMat->SetScalarParameterValue(FName("WaveSpread"), WaveSpread);
    DynamicOceanMat->SetVectorParameterValue(FName("WindDirection"), WindDirection);

    // Appearance / SingleLayerWater
    DynamicOceanMat->SetVectorParameterValue(FName("WaterColor"), WaterColor);
    DynamicOceanMat->SetVectorParameterValue(FName("WaterAbsortion"), WaterAbsortion);
    DynamicOceanMat->SetVectorParameterValue(FName("WaterScattering"), WaterScattering);
    DynamicOceanMat->SetScalarParameterValue(FName("WaterScatteringAmount"), WaterScatteringAmount);
    DynamicOceanMat->SetScalarParameterValue(FName("WaterRoughness"), WaterRoughness);
}

void UCosmicOceanComponent::UpdateOceanLOD(
    const FTransform& InProjectionFrame,
    const FIntPoint& InCoarsestCenter,
    uint64 InProjectionRevision,
    bool bInPerformanceMode)
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

    const bool bNeedsUpdate =
        InProjectionRevision != AppliedProjectionRevision ||
        InCoarsestCenter != AppliedCoarsestCenter;

    if (bNeedsUpdate)
    {
        CurrentProjectionFrame = InProjectionFrame;
        CurrentCoarsestCenter = InCoarsestCenter;
        CurrentProjectionRevision = InProjectionRevision;

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
    Settings.BaseGridSpacing = GetCalculatedBaseGridSpacing();
    Settings.OceanRadius = EffectiveRadius;
    Settings.PatchTransform = PatchTransform;
    Settings.CoarsestGridCenter = CurrentCoarsestCenter;
    Settings.ProjectionRevision = CurrentProjectionRevision;

    bIsGeneratingOcean = true;
    OceanTask = new FAsyncTask<FCosmicOceanGenerationTask>(MoveTemp(Settings));
    OceanTask->StartBackgroundTask();
}

bool UCosmicOceanComponent::CheckAndApplyOceanMeshUpdate()
{
    if (!OceanTask) return true;
    if (!OceanTask->IsDone()) return false;

    FCosmicOceanGenerationTask& CompletedTask = OceanTask->GetTask();

    AppliedCoarsestCenter = CompletedTask.CalculatedGridCenter;
    AppliedProjectionRevision = CompletedTask.CalculatedProjectionRevision;

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

    if (!bPerformanceMode &&
        (CurrentProjectionRevision != AppliedProjectionRevision ||
         CurrentCoarsestCenter != AppliedCoarsestCenter))
    {
        RequestOceanMeshUpdate();
    }

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
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

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

    // Material mode or override material change
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, bUseGeneratedMaterial) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, OceanMaterial))
    {
        BuildDynamicMaterial();
        return;
    }

    // Wave and appearance parameters (update MID directly without rebuilding geometry)
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveHeight) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveLength) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveSpeed) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveSteepness) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveChop) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveCount) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveSpread) ||
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
}
#endif

void UCosmicOceanComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bHasOcean || !bInit) return;

    if (DynamicOceanMat && GetOwner())
    {
        DynamicOceanMat->SetVectorParameterValue(FName("PlanetCenter"), GetOwner()->GetActorLocation());
        const double EffectiveRadius = PlanetRadiusCm + SeaLevelKm * 100000.0;
        DynamicOceanMat->SetScalarParameterValue(FName("PlanetRadius"), static_cast<float>(EffectiveRadius));
    }

    CheckAndApplyOceanMeshUpdate();
}
