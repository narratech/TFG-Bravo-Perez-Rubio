// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Planet/CosmicCloudComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

UCosmicCloudComponent::UCosmicCloudComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	bTickInEditor = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultCloudMat(
		TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicVolumetricClouds.M_CosmicVolumetricClouds"));
	if (DefaultCloudMat.Succeeded())
	{
		CloudMaterial = DefaultCloudMat.Object;
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
	}
}

void UCosmicCloudComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHasClouds || !DynamicCloudMaterial)
	{
		return;
	}

	if (bAutoSyncDirectionalLight)
	{
		FVector SunDir;
		FLinearColor SunCol;
		ResolveSunLighting(SunDir, SunCol);

		if (!SunDir.Equals(LastSyncedSunDir, 1e-4f) || !SunCol.Equals(LastSyncedSunColor, 1e-3f))
		{
			DynamicCloudMaterial->SetVectorParameterValue(TEXT("SunDir"), FLinearColor(SunDir.X, SunDir.Y, SunDir.Z, 0.0f));
			DynamicCloudMaterial->SetVectorParameterValue(TEXT("SunColor"), SunCol);
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
	EnsureCloudShellMesh();
	UpdateShellScale();
	UpdateMaterialParameters();
}

void UCosmicCloudComponent::ClearClouds()
{
	if (IsValid(CloudShellMesh))
	{
		CloudShellMesh->DestroyComponent();
	}
	CloudShellMesh = nullptr;
	DynamicCloudMaterial = nullptr;
	BuiltSphereSegments = 0;
}

void UCosmicCloudComponent::ResetPointersAfterDuplicate(USceneComponent* NewRoot)
{
	CloudShellMesh = nullptr;
	DynamicCloudMaterial = nullptr;
	BuiltSphereSegments = 0;
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
	}

	const int32 ClampedSegs = FMath::Clamp(SphereSegments, 16, 96);
	if (BuiltSphereSegments != ClampedSegs)
	{
		BuildInvertedUnitSphere(CloudShellMesh, ClampedSegs, FMath::Max(12, ClampedSegs / 2));
		BuiltSphereSegments = ClampedSegs;
	}

	CloudShellMesh->SetVisibility(bHasClouds, true);

	if (!CloudMaterial)
	{
		CloudMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicVolumetricClouds.M_CosmicVolumetricClouds"));
	}

	if (CloudMaterial)
	{
		if (!IsValid(DynamicCloudMaterial) || DynamicCloudMaterial->Parent != CloudMaterial)
		{
			DynamicCloudMaterial = UMaterialInstanceDynamic::Create(CloudMaterial, this);
		}
		CloudShellMesh->SetMaterial(0, DynamicCloudMaterial);
	}
}

void UCosmicCloudComponent::BuildInvertedUnitSphere(
	UProceduralMeshComponent* MeshComp,
	int32 LongitudeSegments,
	int32 LatitudeSegments) const
{
	if (!MeshComp)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	const int32 RingVerts = LongitudeSegments + 1;
	Vertices.Reserve((LatitudeSegments + 1) * RingVerts);
	Normals.Reserve((LatitudeSegments + 1) * RingVerts);
	UVs.Reserve((LatitudeSegments + 1) * RingVerts);
	Triangles.Reserve(LatitudeSegments * LongitudeSegments * 6);

	for (int32 Lat = 0; Lat <= LatitudeSegments; ++Lat)
	{
		const float V = static_cast<float>(Lat) / static_cast<float>(LatitudeSegments);
		const float Theta = V * PI; // 0 (north pole) to PI (south pole)
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
			Normals.Add(-Pos); // Inward-facing normal
			UVs.Add(FVector2D(U, V));
		}
	}

	// Inverted winding so only back faces (interior of the bounding sphere) are rasterized.
	// This guarantees 1 fragment per screen pixel both from outside space and from inside the atmosphere.
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
				Triangles.Add(I0);
				Triangles.Add(I2);
				Triangles.Add(I1);
			}

			if (Lat < LatitudeSegments - 1)
			{
				Triangles.Add(I1);
				Triangles.Add(I2);
				Triangles.Add(I3);
			}
		}
	}

	MeshComp->ClearAllMeshSections();
	MeshComp->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, {}, {}, false);
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

void UCosmicCloudComponent::UpdateMaterialParameters()
{
	if (!IsValid(DynamicCloudMaterial))
	{
		return;
	}

	// Geometry
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("PlanetRadiusKm"), static_cast<float>(PlanetRadiusKm));
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("LayerBottomKm"), LayerBottomAltitudeKm);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("LayerHeightKm"), LayerHeightKm);

	// Shape & Weather
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("WeatherScale"), WeatherScale);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("ShapeScale"), ShapeScale);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("DetailScale"), DetailScale);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("SwirlStrength"), SwirlStrength);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("Coverage"), Coverage);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("Density"), Density);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("ErosionStrength"), ErosionStrength);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("WindSpeed"), WindSpeed);

	// Lighting & Scattering
	FVector SunDir;
	FLinearColor SunCol;
	ResolveSunLighting(SunDir, SunCol);
	DynamicCloudMaterial->SetVectorParameterValue(TEXT("SunDir"), FLinearColor(SunDir.X, SunDir.Y, SunDir.Z, 0.0f));
	DynamicCloudMaterial->SetVectorParameterValue(TEXT("SunColor"), SunCol);
	LastSyncedSunDir = SunDir;
	LastSyncedSunColor = SunCol;

	DynamicCloudMaterial->SetVectorParameterValue(TEXT("CloudAlbedo"), CloudAlbedo);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("PhaseG"), PhaseG);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("PhaseG2"), PhaseG2);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("PhaseBlend"), PhaseBlend);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("MultiScatterContribution"), MultiScatterContribution);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("MultiScatterOcclusion"), MultiScatterOcclusion);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("MultiScatterEccentricity"), MultiScatterEccentricity);
	DynamicCloudMaterial->SetVectorParameterValue(TEXT("AmbientSkyColor"), AmbientSkyColor);
	DynamicCloudMaterial->SetVectorParameterValue(TEXT("AmbientGroundColor"), AmbientGroundColor);
	DynamicCloudMaterial->SetVectorParameterValue(TEXT("AtmosphereTintColor"), AtmosphereTintColor);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("AtmosphereBlendStrength"), AtmosphereBlendStrength);

	// Distance-Adaptive LOD
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("NearMaxSteps"), static_cast<float>(NearMaxSteps));
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("FarMaxSteps"), static_cast<float>(FarMaxSteps));
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("NearShadowSteps"), static_cast<float>(NearShadowSteps));
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("FarShadowSteps"), static_cast<float>(FarShadowSteps));
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("LodNearAltitudeKm"), LodNearAltitudeKm);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("LodFarAltitudeKm"), LodFarAltitudeKm);
	DynamicCloudMaterial->SetScalarParameterValue(TEXT("DetailFadeAltitudeKm"), DetailFadeAltitudeKm);
}
