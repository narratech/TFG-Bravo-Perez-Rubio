// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Planet/CosmicAtmosphereComponent.h"
#include "ModulesBridge/CosmicCameraBridge.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#endif

UCosmicAtmosphereComponent::UCosmicAtmosphereComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f;
	bTickInEditor = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultAtmoMat(
		TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicAtmosphere.M_CosmicAtmosphere"));
	if (DefaultAtmoMat.Succeeded())
	{
		AtmosphereMaterial = DefaultAtmoMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultAtmoMatInside(
		TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicAtmosphere_Inside.M_CosmicAtmosphere_Inside"));
	if (DefaultAtmoMatInside.Succeeded())
	{
		AtmosphereMaterialInside = DefaultAtmoMatInside.Object;
	}
}

void UCosmicAtmosphereComponent::OnRegister()
{
	Super::OnRegister();

	if (bHasAtmosphere)
	{
		EnsureAtmosphereShellMesh();
		UpdateShellScale();
		UpdateMaterialParameters();
		UpdateCameraShellMode(true);
	}
	else if (AtmosphereShellMesh)
	{
		AtmosphereShellMesh->SetVisibility(false, true);
	}
}

void UCosmicAtmosphereComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bHasAtmosphere)
	{
		EnsureAtmosphereShellMesh();
		UpdateShellScale();
		UpdateMaterialParameters();
		UpdateCameraShellMode(true);
	}
}

void UCosmicAtmosphereComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHasAtmosphere || !IsValid(AtmosphereShellMesh))
	{
		return;
	}

	// Keep exterior (hardware depth-tested) vs interior (depth-test disabled) shell synced every frame
	UpdateCameraShellMode(false);

	// Throttle directional light polling
	LightRefreshTimer += DeltaTime;
	if (bAutoSyncDirectionalLight && LightRefreshTimer >= 0.1f)
	{
		LightRefreshTimer = 0.0f;

		FVector SunDir;
		FLinearColor SunCol;
		ResolveSunLighting(SunDir, SunCol);

		if (!SunDir.Equals(LastSyncedSunDir, 1e-4f) || !SunCol.Equals(LastSyncedSunColor, 1e-3f))
		{
			const FLinearColor SunDirVec(SunDir.X, SunDir.Y, SunDir.Z, 0.0f);
			if (IsValid(DynamicAtmosphereMaterial))
			{
				DynamicAtmosphereMaterial->SetVectorParameterValue(TEXT("SunDir"), SunDirVec);
				DynamicAtmosphereMaterial->SetVectorParameterValue(TEXT("SunColor"), SunCol);
			}
			if (IsValid(DynamicAtmosphereMaterialInside))
			{
				DynamicAtmosphereMaterialInside->SetVectorParameterValue(TEXT("SunDir"), SunDirVec);
				DynamicAtmosphereMaterialInside->SetVectorParameterValue(TEXT("SunColor"), SunCol);
			}
			LastSyncedSunDir = SunDir;
			LastSyncedSunColor = SunCol;
		}
	}
}

void UCosmicAtmosphereComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	ClearAtmosphere();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

#if WITH_EDITOR
void UCosmicAtmosphereComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!bHasAtmosphere)
	{
		if (AtmosphereShellMesh)
		{
			AtmosphereShellMesh->SetVisibility(false, true);
		}
		return;
	}

	const FName PropName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropName == GET_MEMBER_NAME_CHECKED(UCosmicAtmosphereComponent, AtmosphereMaterial))
	{
		DynamicAtmosphereMaterial = nullptr;
	}
	else if (PropName == GET_MEMBER_NAME_CHECKED(UCosmicAtmosphereComponent, AtmosphereMaterialInside))
	{
		DynamicAtmosphereMaterialInside = nullptr;
	}

	RebuildAtmosphere();
}
#endif

void UCosmicAtmosphereComponent::InitAtmosphere(double InPlanetRadiusKm, USceneComponent* Parent)
{
	PlanetRadiusKm = FMath::Max(InPlanetRadiusKm, 0.005);

	if (Parent && GetAttachParent() != Parent)
	{
		AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform);
		SetRelativeLocation(FVector::ZeroVector);
		SetRelativeRotation(FRotator::ZeroRotator);
	}

	if (!bHasAtmosphere)
	{
		if (AtmosphereShellMesh)
		{
			AtmosphereShellMesh->SetVisibility(false, true);
		}
		return;
	}

	EnsureAtmosphereShellMesh();
	UpdateShellScale();
	UpdateMaterialParameters();
	UpdateCameraShellMode(true);
}

void UCosmicAtmosphereComponent::RebuildAtmosphere()
{
	if (!bHasAtmosphere)
	{
		if (AtmosphereShellMesh)
		{
			AtmosphereShellMesh->SetVisibility(false, true);
		}
		return;
	}

	BuiltSphereSegments = 0;
	bShellModeInitialized = false;
	EnsureAtmosphereShellMesh();
	UpdateShellScale();
	UpdateMaterialParameters();
	UpdateCameraShellMode(true);
}

void UCosmicAtmosphereComponent::ClearAtmosphere()
{
	if (IsValid(AtmosphereShellMesh))
	{
		AtmosphereShellMesh->DestroyComponent();
	}
	AtmosphereShellMesh = nullptr;
	DynamicAtmosphereMaterial = nullptr;
	DynamicAtmosphereMaterialInside = nullptr;
	BuiltSphereSegments = 0;
	bShellModeInitialized = false;
}

void UCosmicAtmosphereComponent::ResetPointersAfterDuplicate(USceneComponent* NewRoot)
{
	AtmosphereShellMesh = nullptr;
	DynamicAtmosphereMaterial = nullptr;
	DynamicAtmosphereMaterialInside = nullptr;
	BuiltSphereSegments = 0;
	bShellModeInitialized = false;
	CachedDirectionalLight.Reset();

	if (NewRoot && GetAttachParent() != NewRoot)
	{
		AttachToComponent(NewRoot, FAttachmentTransformRules::KeepRelativeTransform);
		SetRelativeLocation(FVector::ZeroVector);
		SetRelativeRotation(FRotator::ZeroRotator);
	}
}

float UCosmicAtmosphereComponent::GetEffectiveAtmosphereHeightKm() const
{
	if (bAutoAtmosphereHeight)
	{
		return FMath::Max(static_cast<float>(PlanetRadiusKm * static_cast<double>(AtmosphereHeightRatio)), 0.005f);
	}
	return FMath::Max(AtmosphereHeightKm, 0.005f);
}

double UCosmicAtmosphereComponent::GetEffectiveBottomRadiusKm() const
{
	return FMath::Max(PlanetRadiusKm + static_cast<double>(GroundRadiusOffsetKm), 0.005);
}

void UCosmicAtmosphereComponent::EnsureAtmosphereShellMesh()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld())
	{
		return;
	}

	if (!IsValid(AtmosphereShellMesh))
	{
		AtmosphereShellMesh = NewObject<UProceduralMeshComponent>(Owner, NAME_None, RF_Transient);
		if (!AtmosphereShellMesh)
		{
			return;
		}

		AtmosphereShellMesh->bUseAsyncCooking = false;
		AtmosphereShellMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		AtmosphereShellMesh->SetCastShadow(false);
		AtmosphereShellMesh->bCastDynamicShadow = false;
		AtmosphereShellMesh->bCastStaticShadow = false;
		AtmosphereShellMesh->SetReceivesDecals(false);
		AtmosphereShellMesh->bAffectDistanceFieldLighting = false;
		AtmosphereShellMesh->bAffectDynamicIndirectLighting = false;
		AtmosphereShellMesh->bNeverDistanceCull = true;
		AtmosphereShellMesh->TranslucencySortPriority = TranslucencySortPriority;

		AtmosphereShellMesh->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
		AtmosphereShellMesh->SetRelativeLocation(FVector::ZeroVector);
		AtmosphereShellMesh->SetRelativeRotation(FRotator::ZeroRotator);
		AtmosphereShellMesh->RegisterComponent();
		BuiltSphereSegments = 0;
		bShellModeInitialized = false;
	}
	else
	{
		AtmosphereShellMesh->TranslucencySortPriority = TranslucencySortPriority;
	}

	const int32 ClampedSegs = FMath::Clamp(SphereSegments, 16, 96);
	if (BuiltSphereSegments != ClampedSegs)
	{
		BuildDualWindingUnitSpheres(AtmosphereShellMesh, ClampedSegs, FMath::Max(12, ClampedSegs / 2));
		BuiltSphereSegments = ClampedSegs;
		bShellModeInitialized = false;
	}

	AtmosphereShellMesh->SetVisibility(bHasAtmosphere, true);

	if (!AtmosphereMaterial)
	{
		AtmosphereMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicAtmosphere.M_CosmicAtmosphere"));
	}

	if (!AtmosphereMaterialInside)
	{
		AtmosphereMaterialInside = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicAtmosphere_Inside.M_CosmicAtmosphere_Inside"));
	}

	if (AtmosphereMaterial)
	{
		if (!IsValid(DynamicAtmosphereMaterial) || DynamicAtmosphereMaterial->Parent != AtmosphereMaterial)
		{
			DynamicAtmosphereMaterial = UMaterialInstanceDynamic::Create(AtmosphereMaterial, this);
		}
		AtmosphereShellMesh->SetMaterial(0, DynamicAtmosphereMaterial);
	}

	UMaterialInterface* InsideSourceMat = AtmosphereMaterialInside ? AtmosphereMaterialInside.Get() : AtmosphereMaterial.Get();
	if (InsideSourceMat)
	{
		if (!IsValid(DynamicAtmosphereMaterialInside) || DynamicAtmosphereMaterialInside->Parent != InsideSourceMat)
		{
			DynamicAtmosphereMaterialInside = UMaterialInstanceDynamic::Create(InsideSourceMat, this);
		}
		AtmosphereShellMesh->SetMaterial(1, DynamicAtmosphereMaterialInside);
	}
}

void UCosmicAtmosphereComponent::BuildDualWindingUnitSpheres(
	UProceduralMeshComponent* MeshComp,
	int32 LongitudeSegments,
	int32 LatitudeSegments) const
{
	if (!MeshComp)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<FVector> OutwardNormals;
	TArray<FVector> InwardNormals;
	TArray<FVector2D> UVs;
	TArray<int32> OutwardTriangles;
	TArray<int32> InwardTriangles;

	const int32 RingVerts = LongitudeSegments + 1;
	const int32 TotalVerts = (LatitudeSegments + 1) * RingVerts;
	const int32 TotalIndices = LatitudeSegments * LongitudeSegments * 6;

	Vertices.Reserve(TotalVerts);
	OutwardNormals.Reserve(TotalVerts);
	InwardNormals.Reserve(TotalVerts);
	UVs.Reserve(TotalVerts);
	OutwardTriangles.Reserve(TotalIndices);
	InwardTriangles.Reserve(TotalIndices);

	for (int32 Lat = 0; Lat <= LatitudeSegments; ++Lat)
	{
		const float V = static_cast<float>(Lat) / static_cast<float>(LatitudeSegments);
		const float Theta = V * PI;
		const float SinTheta = FMath::Sin(Theta);
		const float CosTheta = FMath::Cos(Theta);

		for (int32 Lon = 0; Lon <= LongitudeSegments; ++Lon)
		{
			const float U = static_cast<float>(Lon) / static_cast<float>(LongitudeSegments);
			const float Phi = U * 2.0f * PI;
			const float SinPhi = FMath::Sin(Phi);
			const float CosPhi = FMath::Cos(Phi);

			const FVector Pos(SinTheta * CosPhi, SinTheta * SinPhi, CosTheta);
			Vertices.Add(Pos);
			OutwardNormals.Add(Pos);
			InwardNormals.Add(-Pos);
			UVs.Add(FVector2D(U, V));
		}
	}

	for (int32 Lat = 0; Lat < LatitudeSegments; ++Lat)
	{
		for (int32 Lon = 0; Lon < LongitudeSegments; ++Lon)
		{
			const int32 I0 = Lat * RingVerts + Lon;
			const int32 I1 = I0 + 1;
			const int32 I2 = (Lat + 1) * RingVerts + Lon;
			const int32 I3 = I2 + 1;

			if (Lat > 0)
			{
				// Outward-facing (front faces when camera is outside atmosphere -> hardware depth test works)
				OutwardTriangles.Add(I0);
				OutwardTriangles.Add(I1);
				OutwardTriangles.Add(I2);

				// Inward-facing (back faces when camera is inside atmosphere or on planet surface)
				InwardTriangles.Add(I0);
				InwardTriangles.Add(I2);
				InwardTriangles.Add(I1);
			}

			if (Lat < LatitudeSegments - 1)
			{
				OutwardTriangles.Add(I1);
				OutwardTriangles.Add(I3);
				OutwardTriangles.Add(I2);

				InwardTriangles.Add(I1);
				InwardTriangles.Add(I2);
				InwardTriangles.Add(I3);
			}
		}
	}

	MeshComp->ClearAllMeshSections();
	// Section 0: Exterior outward sphere (DisableDepthTest = false)
	MeshComp->CreateMeshSection_LinearColor(0, Vertices, OutwardTriangles, OutwardNormals, UVs, {}, {}, false);
	// Section 1: Interior inward sphere (DisableDepthTest = true)
	MeshComp->CreateMeshSection_LinearColor(1, Vertices, InwardTriangles, InwardNormals, UVs, {}, {}, false);
}

bool UCosmicAtmosphereComponent::GetActiveCameraLocation(FVector& OutCameraLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (World->IsGameWorld())
	{
		if (APlayerCameraManager* CamMgr = UGameplayStatics::GetPlayerCameraManager(World, 0))
		{
			OutCameraLocation = CamMgr->GetCameraLocation();
			return true;
		}
	}

#if WITH_EDITOR
	if (GCurrentLevelEditingViewportClient)
	{
		OutCameraLocation = GCurrentLevelEditingViewportClient->GetViewLocation();
		return true;
	}
#endif

	if (!FCosmicCameraBridge::CameraLocation.IsZero())
	{
		OutCameraLocation = FCosmicCameraBridge::CameraLocation;
		return true;
	}

	return false;
}

void UCosmicAtmosphereComponent::UpdateCameraShellMode(bool bForce)
{
	if (!IsValid(AtmosphereShellMesh) || AtmosphereShellMesh->GetNumSections() < 2)
	{
		return;
	}

	FVector CamLoc;
	if (!GetActiveCameraLocation(CamLoc))
	{
		if (bForce || !bShellModeInitialized)
		{
			bUsingInsideShell = false;
			bShellModeInitialized = true;
			AtmosphereShellMesh->SetMeshSectionVisible(0, true);
			AtmosphereShellMesh->SetMeshSectionVisible(1, false);
		}
		return;
	}

	const double CamDistKm = FVector::Dist(CamLoc, GetComponentLocation()) / 100000.0;
	const double TopRadiusKm = GetEffectiveBottomRadiusKm() + static_cast<double>(GetEffectiveAtmosphereHeightKm());
	const double ShellRadiusKm = TopRadiusKm * 1.03;
	// Adaptive safety margin so small planets (e.g. 0.1 km) and large planets (e.g. 1000 km) never clip the outer shell
	const double SwitchMarginKm = FMath::Clamp(static_cast<double>(GetEffectiveAtmosphereHeightKm()) * 0.15, 0.01, 2.5);
	const bool bShouldUseInside = (CamDistKm <= ShellRadiusKm + SwitchMarginKm);

	if (bForce || !bShellModeInitialized || bShouldUseInside != bUsingInsideShell)
	{
		bUsingInsideShell = bShouldUseInside;
		bShellModeInitialized = true;
		AtmosphereShellMesh->SetMeshSectionVisible(0, !bUsingInsideShell);
		AtmosphereShellMesh->SetMeshSectionVisible(1, bUsingInsideShell);
	}
}

void UCosmicAtmosphereComponent::UpdateShellScale()
{
	if (!IsValid(AtmosphereShellMesh))
	{
		return;
	}

	// Add 3% chordal safety margin so faceted sphere edges always enclose the exact R_top mathematical sphere
	const double TopRadiusKm = GetEffectiveBottomRadiusKm() + static_cast<double>(GetEffectiveAtmosphereHeightKm());
	const double ShellRadiusCm = TopRadiusKm * 100000.0 * 1.03;
	AtmosphereShellMesh->SetRelativeScale3D(FVector(ShellRadiusCm));
}

void UCosmicAtmosphereComponent::ResolveSunLighting(FVector& OutSunDir, FLinearColor& OutSunColor)
{
	OutSunDir = ManualSunDirection.GetSafeNormal();
	if (OutSunDir.IsNearlyZero())
	{
		OutSunDir = FVector(-0.93f, -0.03f, 0.36f).GetSafeNormal();
	}
	OutSunColor = FLinearColor(1.0f, 0.98f, 0.94f, 1.0f) * (10.0f * SunIntensityScale);

	if (!bAutoSyncDirectionalLight || !GetWorld())
	{
		return;
	}

	if (!CachedDirectionalLight.IsValid())
	{
		for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
		{
			if (IsValid(*It))
			{
				CachedDirectionalLight = *It;
				break;
			}
		}
	}

	if (ADirectionalLight* DirLight = CachedDirectionalLight.Get())
	{
		OutSunDir = (-DirLight->GetActorForwardVector()).GetSafeNormal();
		if (UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(DirLight->GetLightComponent()))
		{
			const FLinearColor BaseCol = LightComp->GetLightColor();
			const float ScaledIntensity = FMath::Max(LightComp->Intensity, 0.0f) * SunIntensityScale;
			OutSunColor = BaseCol * ScaledIntensity;
		}
	}
}

void UCosmicAtmosphereComponent::ApplyParametersToMID(UMaterialInstanceDynamic* MID, const FVector& SunDir, const FLinearColor& SunCol) const
{
	if (!IsValid(MID))
	{
		return;
	}

	const float BottomKm = static_cast<float>(GetEffectiveBottomRadiusKm());
	const float HeightKm = GetEffectiveAtmosphereHeightKm();

	// UE5 Earth reference atmosphere height is 60 km. When bScalePhysicsToAtmosphereHeight is enabled,
	// rescale extinction/scattering coefficients and scale heights so the optical depth stays physically
	// consistent at any planet size (from 50 m moons to 6,360+ km Earths).
	const float PhysicsScale = bScalePhysicsToAtmosphereHeight ? (60.0f / FMath::Max(HeightKm, 0.005f)) : 1.0f;
	const float InvPhysicsScale = 1.0f / PhysicsScale;

	const FLinearColor EffRayleigh = RayleighScattering * (RayleighScatteringScale * PhysicsScale);
	const float EffRayleighH = FMath::Max(RayleighExponentialDistribution * InvPhysicsScale, 1e-4f);

	const FLinearColor EffMieScat = MieScattering * (MieScatteringScale * PhysicsScale);
	const FLinearColor EffMieAbs = MieAbsorption * (MieAbsorptionScale * PhysicsScale);
	const float EffMieH = FMath::Max(MieExponentialDistribution * InvPhysicsScale, 1e-4f);

	const FLinearColor EffOzone = OtherAbsorption * (OtherAbsorptionScale * PhysicsScale);
	const float EffOzoneTipAlt = OtherTentDistribution.TipAltitude * InvPhysicsScale;
	const float EffOzoneWidth = FMath::Max(OtherTentDistribution.Width * InvPhysicsScale, 1e-4f);

	// Geometry
	MID->SetScalarParameterValue(TEXT("BottomRadiusKm"), BottomKm);
	MID->SetScalarParameterValue(TEXT("AtmosphereHeightKm"), HeightKm);

	// Rayleigh
	MID->SetVectorParameterValue(TEXT("RayleighScattering"), EffRayleigh);
	MID->SetScalarParameterValue(TEXT("RayleighScaleHeightKm"), EffRayleighH);

	// Mie
	MID->SetVectorParameterValue(TEXT("MieScattering"), EffMieScat);
	MID->SetVectorParameterValue(TEXT("MieAbsorption"), EffMieAbs);
	MID->SetScalarParameterValue(TEXT("MieAnisotropy"), FMath::Clamp(MieAnisotropy, 0.0f, 0.995f));
	MID->SetScalarParameterValue(TEXT("MieScaleHeightKm"), EffMieH);

	// Ozone / Absorption
	MID->SetVectorParameterValue(TEXT("OzoneAbsorption"), EffOzone);
	MID->SetScalarParameterValue(TEXT("OzoneTipAltitudeKm"), EffOzoneTipAlt);
	MID->SetScalarParameterValue(TEXT("OzoneTipValue"), OtherTentDistribution.TipValue);
	MID->SetScalarParameterValue(TEXT("OzoneWidthKm"), EffOzoneWidth);

	// Multi-scattering & Art Direction
	MID->SetVectorParameterValue(TEXT("GroundAlbedo"), GroundAlbedo);
	MID->SetScalarParameterValue(TEXT("MultiScatteringFactor"), MultiScatteringFactor);
	MID->SetVectorParameterValue(TEXT("SkyLuminanceFactor"), SkyLuminanceFactor);
	MID->SetScalarParameterValue(TEXT("AerialPerspectiveDistanceScale"), AerialPerspectiveViewDistanceScale);
	MID->SetScalarParameterValue(TEXT("AerialPerspectiveStartDepthKm"), AerialPerspectiveStartDepthKm);

	// Sun Lighting & Solar Disk
	MID->SetVectorParameterValue(TEXT("SunDir"), FLinearColor(SunDir.X, SunDir.Y, SunDir.Z, 0.0f));
	MID->SetVectorParameterValue(TEXT("SunColor"), SunCol);
	const float SunDiskCos = bRenderSunDisk
		? FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(SunDiskRadiusDeg, 0.05f, 10.0f)))
		: 1.0f;
	MID->SetScalarParameterValue(TEXT("SunDiskCosHalfAngle"), SunDiskCos);
	MID->SetScalarParameterValue(TEXT("SunDiskIntensity"), bRenderSunDisk ? SunDiskIntensity : 0.0f);

	// Distance-Adaptive LOD
	MID->SetScalarParameterValue(TEXT("NearViewSamples"), static_cast<float>(NearViewSamples));
	MID->SetScalarParameterValue(TEXT("FarViewSamples"), static_cast<float>(FarViewSamples));
	MID->SetScalarParameterValue(TEXT("SunTransmittanceSamples"), static_cast<float>(SunTransmittanceSamples));
	MID->SetScalarParameterValue(TEXT("LodNearAltitudeKm"), LodNearAltitudeKm);
	MID->SetScalarParameterValue(TEXT("LodFarAltitudeKm"), LodFarAltitudeKm);
}

void UCosmicAtmosphereComponent::UpdateMaterialParameters()
{
	FVector SunDir;
	FLinearColor SunCol;
	ResolveSunLighting(SunDir, SunCol);
	LastSyncedSunDir = SunDir;
	LastSyncedSunColor = SunCol;

	ApplyParametersToMID(DynamicAtmosphereMaterial, SunDir, SunCol);
	ApplyParametersToMID(DynamicAtmosphereMaterialInside, SunDir, SunCol);
}
