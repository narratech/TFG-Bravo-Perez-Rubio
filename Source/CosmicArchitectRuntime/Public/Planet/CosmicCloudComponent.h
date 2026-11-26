// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CosmicCloudComponent.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ADirectionalLight;

/**
 * UCosmicCloudComponent
 *
 * Per-planet distance-adaptive volumetric cloud layer for CosmicArchitect.
 * Renders an inward-facing bounding sphere proxy driven by CosmicVolumetricClouds.ush:
 *   - Up close / in-atmosphere: high-step 3D Perlin-Worley + detail erosion raymarching
 *     with dual-lobe Henyey-Greenstein, 3-octave multi-scattering, sun self-shadowing,
 *     and soft SceneDepth terrain intersection.
 *   - From orbit / deep space: smoothly lowers raymarch & shadow steps, disables sub-pixel
 *     detail fetches, and only shades pixels inside the planet's tight bounding sphere.
 */
UCLASS(ClassGroup = (CosmicArchitect), meta = (BlueprintSpawnableComponent),
	HideCategories = (Rendering, Lighting, Navigation, Replication, Physics, Collision,
		Activation, AssetUserData, HLOD, Cooking, Tags, ComponentReplication))
class COSMICARCHITECTRUNTIME_API UCosmicCloudComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UCosmicCloudComponent();

	/** Initializes or updates the volumetric cloud shell for a planet of the given radius (in km). */
	UFUNCTION(BlueprintCallable, Category = "Cosmic Architect|Clouds")
	void InitClouds(double InPlanetRadiusKm, USceneComponent* Parent = nullptr);

	/** Rebuilds the inward-facing proxy sphere mesh and refreshes all material parameters. */
	UFUNCTION(BlueprintCallable, Category = "Cosmic Architect|Clouds")
	void RebuildClouds();

	/** Removes and destroys the internal cloud shell mesh component. */
	UFUNCTION(BlueprintCallable, Category = "Cosmic Architect|Clouds")
	void ClearClouds();

	/** Clears inherited pointers after Editor actor duplication without affecting the original actor. */
	void ResetPointersAfterDuplicate(USceneComponent* NewRoot);

	/** Pushes all UPROPERTY cloud parameters and sun lighting to the dynamic material instance. */
	UFUNCTION(BlueprintCallable, Category = "Cosmic Architect|Clouds")
	void UpdateMaterialParameters();

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// -----------------------------------------------------------------------
	// Enable & Material
	// -----------------------------------------------------------------------

	/** Enables or disables the volumetric cloud layer on this planet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds")
	bool bHasClouds = false;

	/** Master material or material instance using CosmicVolumetricClouds.ush. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds", meta = (EditCondition = "bHasClouds"))
	TObjectPtr<UMaterialInterface> CloudMaterial = nullptr;

	// -----------------------------------------------------------------------
	// Layer Geometry (Kilometers)
	// -----------------------------------------------------------------------

	/** Planet radius in kilometers (automatically synchronized by ACosmicPlanet). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Clouds|Layer")
	double PlanetRadiusKm = 100.0;

	/** Altitude of the cloud layer base above planet radius (in km). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Layer", meta = (EditCondition = "bHasClouds", ClampMin = "0.0"))
	float LayerBottomAltitudeKm = 1.8f;

	/** Vertical thickness of the volumetric cloud layer (in km). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Layer", meta = (EditCondition = "bHasClouds", ClampMin = "0.1"))
	float LayerHeightKm = 3.5f;

	// -----------------------------------------------------------------------
	// Shape & Weather (Matches MI_PlanetVolumetricCloud defaults)
	// -----------------------------------------------------------------------

	/** Macro weather pattern frequency across the planet sphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shape", meta = (EditCondition = "bHasClouds", ClampMin = "0.1"))
	float WeatherScale = 1.5f;

	/** Medium cumulus / stratocumulus shape frequency. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shape", meta = (EditCondition = "bHasClouds", ClampMin = "0.5"))
	float ShapeScale = 8.5f;

	/** High-frequency 3D detail erosion frequency (used up close). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shape", meta = (EditCondition = "bHasClouds", ClampMin = "1.0"))
	float DetailScale = 32.0f;

	/** 3D domain warp strength creating swirling weather fronts and cyclones. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shape", meta = (EditCondition = "bHasClouds", ClampMin = "0.0", ClampMax = "2.0"))
	float SwirlStrength = 0.35f;

	/** Global cloud coverage [0..1]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shape", meta = (EditCondition = "bHasClouds", ClampMin = "0.0", ClampMax = "1.0"))
	float Coverage = 0.56f;

	/** Optical extinction density multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shape", meta = (EditCondition = "bHasClouds", ClampMin = "0.001"))
	float Density = 0.15f;

	/** Edge erosion strength from the 3D detail volume texture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shape", meta = (EditCondition = "bHasClouds", ClampMin = "0.0", ClampMax = "1.0"))
	float ErosionStrength = 0.40f;

	/** Angular drift speed of the cloud shell around the planet's Z axis (rad/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Shape", meta = (EditCondition = "bHasClouds"))
	float WindSpeed = 0.0015f;

	// -----------------------------------------------------------------------
	// Lighting & Scattering
	// -----------------------------------------------------------------------

	/** Automatically tracks the scene's primary ADirectionalLight for sun direction and color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds"))
	bool bAutoSyncDirectionalLight = true;

	/** Manual sun direction (pointing toward the sun) used when bAutoSyncDirectionalLight is false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds && !bAutoSyncDirectionalLight"))
	FVector ManualSunDirection = FVector(-0.93f, -0.03f, 0.36f);

	/** Multiplier applied to the synced DirectionalLight intensity for cloud radiance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds", ClampMin = "0.0"))
	float SunIntensityScale = 0.42f;

	/** Base scattering albedo of the clouds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds"))
	FLinearColor CloudAlbedo = FLinearColor(0.96f, 0.98f, 1.0f, 1.0f);

	/** Forward scattering Henyey-Greenstein eccentricity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds", ClampMin = "-0.95", ClampMax = "0.95"))
	float PhaseG = 0.70f;

	/** Backward scattering Henyey-Greenstein eccentricity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds", ClampMin = "-0.95", ClampMax = "0.95"))
	float PhaseG2 = -0.30f;

	/** Blend weight between forward (0) and backward (1) phase lobes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds", ClampMin = "0.0", ClampMax = "1.0"))
	float PhaseBlend = 0.55f;

	/** Multi-scattering octave energy contribution (UE5 model). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds", ClampMin = "0.0", ClampMax = "1.0"))
	float MultiScatterContribution = 0.85f;

	/** Multi-scattering optical depth attenuation per octave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds", ClampMin = "0.05", ClampMax = "1.0"))
	float MultiScatterOcclusion = 0.25f;

	/** Multi-scattering phase eccentricity falloff per octave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds", ClampMin = "0.0", ClampMax = "1.0"))
	float MultiScatterEccentricity = 0.25f;

	/** Ambient sky light illuminating cloud tops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds"))
	FLinearColor AmbientSkyColor = FLinearColor(0.32f, 0.46f, 0.72f, 1.0f);

	/** Ambient planetary bounce light illuminating cloud bases. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds"))
	FLinearColor AmbientGroundColor = FLinearColor(0.14f, 0.19f, 0.28f, 1.0f);

	/** Atmospheric Rayleigh haze tint blended at grazing planetary limb angles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds"))
	FLinearColor AtmosphereTintColor = FLinearColor(0.32f, 0.58f, 1.0f, 1.0f);

	/** Strength of atmospheric limb blending (0 = disabled, 1 = full). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Lighting", meta = (EditCondition = "bHasClouds", ClampMin = "0.0", ClampMax = "2.0"))
	float AtmosphereBlendStrength = 0.65f;

	// -----------------------------------------------------------------------
	// Distance-Adaptive Performance / LOD
	// -----------------------------------------------------------------------

	/** Maximum raymarch steps when camera is close to or inside the cloud layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Performance", meta = (EditCondition = "bHasClouds", ClampMin = "8", ClampMax = "96"))
	int32 NearMaxSteps = 56;

	/** Raymarch steps when viewing the planet from far orbital space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Performance", meta = (EditCondition = "bHasClouds", ClampMin = "4", ClampMax = "32"))
	int32 FarMaxSteps = 10;

	/** Sun self-shadowing steps when camera is close to the clouds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Performance", meta = (EditCondition = "bHasClouds", ClampMin = "1", ClampMax = "8"))
	int32 NearShadowSteps = 5;

	/** Sun self-shadowing steps when viewing the planet from afar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Performance", meta = (EditCondition = "bHasClouds", ClampMin = "1", ClampMax = "4"))
	int32 FarShadowSteps = 1;

	/** Camera altitude above surface (km) where LOD transition begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Performance", meta = (EditCondition = "bHasClouds", ClampMin = "0.0"))
	float LodNearAltitudeKm = 12.0f;

	/** Camera altitude above surface (km) where full orbital optimization is reached. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Performance", meta = (EditCondition = "bHasClouds", ClampMin = "1.0"))
	float LodFarAltitudeKm = 120.0f;

	/** Camera altitude (km) at which fine 3D detail erosion texture fetches are fully disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Performance", meta = (EditCondition = "bHasClouds", ClampMin = "1.0"))
	float DetailFadeAltitudeKm = 45.0f;

	/** Tessellation segments of the inward-facing proxy bounding sphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clouds|Performance", meta = (EditCondition = "bHasClouds", ClampMin = "16", ClampMax = "96"))
	int32 SphereSegments = 48;

private:
	/** Ensures CloudShellMesh exists, is attached, and has its inward-facing sphere geometry built. */
	void EnsureCloudShellMesh();

	/** Generates an inward-facing unit sphere (radius 1 cm) with zero collision and zero overdraw. */
	void BuildInvertedUnitSphere(UProceduralMeshComponent* MeshComp, int32 LongitudeSegments, int32 LatitudeSegments) const;

	/** Updates the scale of CloudShellMesh to tightly enclose R_planet + LayerBottom + LayerHeight. */
	void UpdateShellScale();

	/** Resolves sun direction and radiance from the scene's ADirectionalLight (or manual fallback). */
	void ResolveSunLighting(FVector& OutSunDir, FLinearColor& OutSunColor);

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> CloudShellMesh = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicCloudMaterial = nullptr;

	TWeakObjectPtr<ADirectionalLight> CachedDirectionalLight;

	int32 BuiltSphereSegments = 0;
	float LightRefreshTimer = 0.0f;
	FVector LastSyncedSunDir = FVector::ZeroVector;
	FLinearColor LastSyncedSunColor = FLinearColor::Black;
};
