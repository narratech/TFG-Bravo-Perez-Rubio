// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Planet/CosmicCloudComponent.h"
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

UCosmicCloudComponent::UCosmicCloudComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f;
	bTickInEditor = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultCloudMat(
		TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicVolumetricClouds.M_CosmicVolumetricClouds"));
	if (DefaultCloudMat.Succeeded())
	{
		CloudMaterial = DefaultCloudMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultCloudMatInside(
		TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicVolumetricClouds_Inside.M_CosmicVolumetricClouds_Inside"));
	if (DefaultCloudMatInside.Succeeded())
	{
		CloudMaterialInside = DefaultCloudMatInside.Object;
	}
}

void UCosmicCloudComponent::OnRegister()
{
	Super::OnRegister();

	if (bHasClouds)
	{
		EnsureCloudShellMesh();
		UpdateShellScale();
		UpdateMaterialParameters();
		UpdateCameraShellMode(true);
	}
	else if (CloudShellMesh)
	{
		CloudShellMesh->SetVisibility(false, true);
	}
}

void UCosmicCloudComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bHasClouds)
	{
		EnsureCloudShellMesh();
		UpdateShellScale();
		UpdateMaterialParameters();
		UpdateCameraShellMode(true);
	}
}

void UCosmicCloudComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHasClouds || !IsValid(CloudShellMesh))
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
			if (IsValid(DynamicCloudMaterial))
			{
				DynamicCloudMaterial->SetVectorParameterValue(TEXT("SunDir"), SunDirVec);
				DynamicCloudMaterial->SetVectorParameterValue(TEXT("SunColor"), SunCol);
			}
			if (IsValid(DynamicCloudMaterialInside))
			{
				DynamicCloudMaterialInside->SetVectorParameterValue(TEXT("SunDir"), SunDirVec);
				DynamicCloudMaterialInside->SetVectorParameterValue(TEXT("SunColor"), SunCol);
			}
			LastSyncedSunDir = SunDir;
			LastSyncedSunColor = SunCol;
		}
	}
}

void UCosmicCloudComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	ClearClouds();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

#if WITH_EDITOR
void UCosmicCloudComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!bHasClouds)
	{
		if (CloudShellMesh)
		{
			CloudShellMesh->SetVisibility(false, true);
		}
		return;
	}

	const FName PropName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropName == GET_MEMBER_NAME_CHECKED(UCosmicCloudComponent, CloudMaterial))
	{
		DynamicCloudMaterial = nullptr;
	}
	else if (PropName == GET_MEMBER_NAME_CHECKED(UCosmicCloudComponent, CloudMaterialInside))
	{
		DynamicCloudMaterialInside = nullptr;
	}

	RebuildClouds();
}
#endif

void UCosmicCloudComponent::InitClouds(double InPlanetRadiusKm, USceneComponent* Parent)
{
	PlanetRadiusKm = FMath::Max(InPlanetRadiusKm, 0.1);

	if (Parent && GetAttachParent() != Parent)
	{
		AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform);
		SetRelativeLocation(FVector::ZeroVector);
		SetRelativeRotation(FRotator::ZeroRotator);
	}

	if (!bHasClouds)
	{
		if (CloudShellMesh)
		{
			CloudShellMesh->SetVisibility(false, true);
		}
		return;
	}

	EnsureCloudShellMesh();
	UpdateShellScale();
	UpdateMaterialParameters();
	UpdateCameraShellMode(true);
}

void UCosmicCloudComponent::RebuildClouds()
{
	if (!bHasClouds)
	{
		if (CloudShellMesh)
		{
			CloudShellMesh->SetVisibility(false, true);
		}
		return;
	}

	BuiltSphereSegments = 0;
	bShellModeInitialized = false;
	EnsureCloudShellMesh();
	UpdateShellScale();
	UpdateMaterialParameters();
	UpdateCameraShellMode(true);
}

void UCosmicCloudComponent::ClearClouds()
{
	if (IsValid(CloudShellMesh))
	{
		CloudShellMesh->DestroyComponent();
	}
	CloudShellMesh = nullptr;
	DynamicCloudMaterial = nullptr;
	DynamicCloudMaterialInside = nullptr;
	BuiltSphereSegments = 0;
	bShellModeInitialized = false;
}

void UCosmicCloudComponent::ResetPointersAfterDuplicate(USceneComponent* NewRoot)
{
	CloudShellMesh = nullptr;
	DynamicCloudMaterial = nullptr;
	DynamicCloudMaterialInside = nullptr;
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

void UCosmicCloudComponent::EnsureCloudShellMesh()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld())
	{
		return;
	}

	if (!IsValid(CloudShellMesh))
	{
		CloudShellMesh = NewObject<UProceduralMeshComponent>(Owner, NAME_None, RF_Transient);
		if (!CloudShellMesh)
		{
			return;
		}

		CloudShellMesh->bUseAsyncCooking = false;
		CloudShellMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CloudShellMesh->SetCastShadow(false);
		CloudShellMesh->bCastDynamicShadow = false;
		CloudShellMesh->bCastStaticShadow = false;
		CloudShellMesh->SetReceivesDecals(false);
		CloudShellMesh->bAffectDistanceFieldLighting = false;
		CloudShellMesh->bAffectDynamicIndirectLighting = false;
		CloudShellMesh->bNeverDistanceCull = true;
		CloudShellMesh->TranslucencySortPriority = 10;

		CloudShellMesh->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
		CloudShellMesh->SetRelativeLocation(FVector::ZeroVector);
		CloudShellMesh->SetRelativeRotation(FRotator::ZeroRotator);
		CloudShellMesh->RegisterComponent();
		BuiltSphereSegments = 0;
		bShellModeInitialized = false;
	}

	const int32 ClampedSegs = FMath::Clamp(SphereSegments, 16, 96);
	if (BuiltSphereSegments != ClampedSegs)
	{
		BuildDualWindingUnitSpheres(CloudShellMesh, ClampedSegs, FMath::Max(12, ClampedSegs / 2));
		BuiltSphereSegments = ClampedSegs;
		bShellModeInitialized = false;
	}

	CloudShellMesh->SetVisibility(bHasClouds, true);

	if (!CloudMaterial)
	{
		CloudMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicVolumetricClouds.M_CosmicVolumetricClouds"));
	}

	if (!CloudMaterialInside)
	{
		CloudMaterialInside = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicVolumetricClouds_Inside.M_CosmicVolumetricClouds_Inside"));
	}

	if (CloudMaterial)
	{
		if (!IsValid(DynamicCloudMaterial) || DynamicCloudMaterial->Parent != CloudMaterial)
		{
			DynamicCloudMaterial = UMaterialInstanceDynamic::Create(CloudMaterial, this);
		}
		CloudShellMesh->SetMaterial(0, DynamicCloudMaterial);
	}

	UMaterialInterface* InsideSourceMat = CloudMaterialInside ? CloudMaterialInside.Get() : CloudMaterial.Get();
	if (InsideSourceMat)
	{
		if (!IsValid(DynamicCloudMaterialInside) || DynamicCloudMaterialInside->Parent != InsideSourceMat)
		{
			DynamicCloudMaterialInside = UMaterialInstanceDynamic::Create(InsideSourceMat, this);
		}
		CloudShellMesh->SetMaterial(1, DynamicCloudMaterialInside);
	}
}

void UCosmicCloudComponent::BuildDualWindingUnitSpheres(
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
				// Outward-facing (front faces when camera is outside atmosphere -> hardware depth test works!)
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

bool UCosmicCloudComponent::GetActiveCameraLocation(FVector& OutCameraLocation) const
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

void UCosmicCloudComponent::UpdateCameraShellMode(bool bForce)
{
	if (!IsValid(CloudShellMesh) || CloudShellMesh->GetNumSections() < 2)
	{
		return;
	}

	FVector CamLoc;
	if (!GetActiveCameraLocation(CamLoc))
	{
		// Default to exterior shell when camera is unknown
		if (bForce || !bShellModeInitialized)
		{
			bUsingInsideShell = false;
			bShellModeInitialized = true;
			CloudShellMesh->SetMeshSectionVisible(0, true);
			CloudShellMesh->SetMeshSectionVisible(1, false);
		}
		return;
	}

	const double CamDistKm = FVector::Dist(CamLoc, GetComponentLocation()) / 100000.0;
	const double TopRadiusKm = PlanetRadiusKm + static_cast<double>(LayerBottomAltitudeKm) + static_cast<double>(LayerHeightKm);
	const double ShellRadiusKm = TopRadiusKm * 1.025;
	// Switch to interior shell 1.5 km before the camera reaches the outer shell mesh so near-clip never clips it
	const bool bShouldUseInside = (CamDistKm <= ShellRadiusKm + 1.5);

	if (bForce || !bShellModeInitialized || bShouldUseInside != bUsingInsideShell)
	{
		bUsingInsideShell = bShouldUseInside;
		bShellModeInitialized = true;
		CloudShellMesh->SetMeshSectionVisible(0, !bUsingInsideShell);
		CloudShellMesh->SetMeshSectionVisible(1, bUsingInsideShell);
	}
}

void UCosmicCloudComponent::UpdateShellScale()
{
	if (!IsValid(CloudShellMesh))
	{
		return;
	}

	// Add 2.5% chordal safety margin so faceted sphere edges always enclose the exact R_top mathematical sphere
	const double TopRadiusKm = PlanetRadiusKm + static_cast<double>(LayerBottomAltitudeKm) + static_cast<double>(LayerHeightKm);
	const double ShellRadiusCm = TopRadiusKm * 100000.0 * 1.025;
	CloudShellMesh->SetRelativeScale3D(FVector(ShellRadiusCm));
}

void UCosmicCloudComponent::ResolveSunLighting(FVector& OutSunDir, FLinearColor& OutSunColor)
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
		// DirectionalLight forward vector points in the direction light travels; negate to point toward the sun
		OutSunDir = (-DirLight->GetActorForwardVector()).GetSafeNormal();
		if (UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(DirLight->GetLightComponent()))
		{
			const FLinearColor BaseCol = LightComp->GetLightColor();
			const float ScaledIntensity = FMath::Max(LightComp->Intensity, 0.0f) * SunIntensityScale;
			OutSunColor = BaseCol * ScaledIntensity;
		}
	}
}

void UCosmicCloudComponent::ApplyParametersToMID(UMaterialInstanceDynamic* MID, const FVector& SunDir, const FLinearColor& SunCol) const
{
	if (!IsValid(MID))
	{
		return;
	}

	// Geometry
	MID->SetScalarParameterValue(TEXT("PlanetRadiusKm"), static_cast<float>(PlanetRadiusKm));
	MID->SetScalarParameterValue(TEXT("LayerBottomKm"), LayerBottomAltitudeKm);
	MID->SetScalarParameterValue(TEXT("LayerHeightKm"), LayerHeightKm);

	// Shape & Weather
	MID->SetScalarParameterValue(TEXT("WeatherScale"), WeatherScale);
	MID->SetScalarParameterValue(TEXT("ShapeScale"), ShapeScale);
	MID->SetScalarParameterValue(TEXT("DetailScale"), DetailScale);
	MID->SetScalarParameterValue(TEXT("SwirlStrength"), SwirlStrength);
	MID->SetScalarParameterValue(TEXT("Coverage"), Coverage);
	MID->SetScalarParameterValue(TEXT("Density"), Density);
	MID->SetScalarParameterValue(TEXT("ErosionStrength"), ErosionStrength);
	MID->SetScalarParameterValue(TEXT("WindSpeed"), WindSpeed);

	// Lighting & Scattering
	MID->SetVectorParameterValue(TEXT("SunDir"), FLinearColor(SunDir.X, SunDir.Y, SunDir.Z, 0.0f));
	MID->SetVectorParameterValue(TEXT("SunColor"), SunCol);
	MID->SetVectorParameterValue(TEXT("CloudAlbedo"), CloudAlbedo);
	MID->SetScalarParameterValue(TEXT("PhaseG"), PhaseG);
	MID->SetScalarParameterValue(TEXT("PhaseG2"), PhaseG2);
	MID->SetScalarParameterValue(TEXT("PhaseBlend"), PhaseBlend);
	MID->SetScalarParameterValue(TEXT("MultiScatterContribution"), MultiScatterContribution);
	MID->SetScalarParameterValue(TEXT("MultiScatterOcclusion"), MultiScatterOcclusion);
	MID->SetScalarParameterValue(TEXT("MultiScatterEccentricity"), MultiScatterEccentricity);
	MID->SetVectorParameterValue(TEXT("AmbientSkyColor"), AmbientSkyColor);
	MID->SetVectorParameterValue(TEXT("AmbientGroundColor"), AmbientGroundColor);
	MID->SetVectorParameterValue(TEXT("AtmosphereTintColor"), AtmosphereTintColor);
	MID->SetScalarParameterValue(TEXT("AtmosphereBlendStrength"), AtmosphereBlendStrength);

	// Distance-Adaptive LOD
	MID->SetScalarParameterValue(TEXT("NearMaxSteps"), static_cast<float>(NearMaxSteps));
	MID->SetScalarParameterValue(TEXT("FarMaxSteps"), static_cast<float>(FarMaxSteps));
	MID->SetScalarParameterValue(TEXT("NearShadowSteps"), static_cast<float>(NearShadowSteps));
	MID->SetScalarParameterValue(TEXT("FarShadowSteps"), static_cast<float>(FarShadowSteps));
	MID->SetScalarParameterValue(TEXT("LodNearAltitudeKm"), LodNearAltitudeKm);
	MID->SetScalarParameterValue(TEXT("LodFarAltitudeKm"), LodFarAltitudeKm);
	MID->SetScalarParameterValue(TEXT("DetailFadeAltitudeKm"), DetailFadeAltitudeKm);
}

void UCosmicCloudComponent::UpdateMaterialParameters()
{
	FVector SunDir;
	FLinearColor SunCol;
	ResolveSunLighting(SunDir, SunCol);
	LastSyncedSunDir = SunDir;
	LastSyncedSunColor = SunCol;

	ApplyParametersToMID(DynamicCloudMaterial, SunDir, SunCol);
	ApplyParametersToMID(DynamicCloudMaterialInside, SunDir, SunCol);
}
