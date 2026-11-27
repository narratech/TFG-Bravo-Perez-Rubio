// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CosmicAtmosphereComponent.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ADirectionalLight;

/**
 * Altitude-based tent distribution for ozone / secondary absorption layers.
 * Mirrors Unreal Engine's FTentDistribution from USkyAtmosphereComponent.
 */
USTRUCT(BlueprintType)
struct COSMICARCHITECTRUNTIME_API FCosmicAtmosphereTentDistribution
{
	GENERATED_BODY()

	/** Altitude (in km in reference 60 km atmosphere) where absorption density reaches TipValue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tent", meta = (UIMin = "0.0", UIMax = "60.0", ClampMin = "0.0"))
	float TipAltitude = 25.0f;

	/** Peak absorption density multiplier at TipAltitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tent", meta = (UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0"))
	float TipValue = 1.0f;

	/** Half-width (in km in reference 60 km atmosphere) of the tent distribution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tent", meta = (UIMin = "0.01", UIMax = "30.0", ClampMin = "0.01"))
	float Width = 15.0f;
};

/**
 * UCosmicAtmosphereComponent
 *
 * Per-planet physically-based atmospheric scattering component for CosmicArchitect.
 * Modeled after Unreal Engine's USkyAtmosphereComponent (Hillaire 2020) without its two limitations:
 *   1. Supports unlimited independent planetary atmospheres simultaneously in the same world.
 *   2. Works at any planet radius (no minimum radius clamp; supports 50 m moons to 10,000+ km planets)
 *      with optional automatic physical scaling so Earth-like defaults work out of the box at any scale.
 *
 * Renders via a dual-winding proxy sphere driven by CosmicAtmosphere.ush:
 *   - Exterior (Section 0, DisableDepthTest = false): hardware depth-tested against moons/ships/planets.
 *   - Interior (Section 1, DisableDepthTest = true): seamless sky dome & SceneDepth aerial perspective
 *     when inside the atmosphere or standing on the planet surface.
 */
UCLASS(ClassGroup = (CosmicArchitect), meta = (BlueprintSpawnableComponent),
	HideCategories = (Rendering, Navigation, Replication, Physics, Collision,
		Activation, AssetUserData, HLOD, Cooking, Tags, ComponentReplication))
class COSMICARCHITECTRUNTIME_API UCosmicAtmosphereComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UCosmicAtmosphereComponent();

	/** Initializes or synchronizes the atmosphere shell for a planet of the given radius (in km). */
	UFUNCTION(BlueprintCallable, Category = "Cosmic Architect|Atmosphere")
	void InitAtmosphere(double InPlanetRadiusKm, USceneComponent* Parent = nullptr);

	/** Rebuilds the dual-winding proxy sphere mesh and refreshes all material parameters. */
	UFUNCTION(BlueprintCallable, Category = "Cosmic Architect|Atmosphere")
	void RebuildAtmosphere();

	/** Removes and destroys the internal atmosphere shell mesh component. */
	UFUNCTION(BlueprintCallable, Category = "Cosmic Architect|Atmosphere")
	void ClearAtmosphere();

	/** Clears inherited pointers after Editor actor duplication without affecting the original actor. */
	void ResetPointersAfterDuplicate(USceneComponent* NewRoot);

	/** Pushes all UPROPERTY atmosphere parameters and sun lighting to the dynamic material instances. */
	UFUNCTION(BlueprintCallable, Category = "Cosmic Architect|Atmosphere")
	void UpdateMaterialParameters();

	/** Returns the effective atmosphere height in kilometers (auto-scaled or manual). */
	UFUNCTION(BlueprintPure, Category = "Cosmic Architect|Atmosphere")
	float GetEffectiveAtmosphereHeightKm() const;

	/** Returns the effective ground/bottom radius in kilometers (PlanetRadiusKm + GroundRadiusOffsetKm). */
	UFUNCTION(BlueprintPure, Category = "Cosmic Architect|Atmosphere")
	double GetEffectiveBottomRadiusKm() const;

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// -----------------------------------------------------------------------
	// Activation & Materials
	// -----------------------------------------------------------------------

	/** Enables or disables the atmosphere on this planet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere")
	bool bHasAtmosphere = false;

	/** Exterior/orbital material (DisableDepthTest = false for hardware occlusion). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (EditCondition = "bHasAtmosphere"))
	TObjectPtr<UMaterialInterface> AtmosphereMaterial = nullptr;

	/** Interior/in-atmosphere material (DisableDepthTest = true for seamless sky dome & surface view). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere", meta = (EditCondition = "bHasAtmosphere"))
	TObjectPtr<UMaterialInterface> AtmosphereMaterialInside = nullptr;

	// -----------------------------------------------------------------------
	// Planet & Atmosphere Geometry (No lower radius limit)
	// -----------------------------------------------------------------------

	/** Planet radius in kilometers (automatically synchronized by ACosmicPlanet; supports any radius > 0.005 km). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Planet", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.005"))
	double PlanetRadiusKm = 100.0;

	/** Offset applied to PlanetRadiusKm for the atmosphere bottom (negative sinks below terrain valleys/oceans). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Planet", meta = (EditCondition = "bHasAtmosphere"))
	float GroundRadiusOffsetKm = 0.0f;

	/** Ground albedo tinting the lower atmosphere via multi-scattering bounce. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Planet", meta = (EditCondition = "bHasAtmosphere", HideAlphaChannel))
	FLinearColor GroundAlbedo = FLinearColor(0.4f, 0.4f, 0.4f, 1.0f);

	/** When true, AtmosphereHeightKm is computed as PlanetRadiusKm * AtmosphereHeightRatio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Layer", meta = (EditCondition = "bHasAtmosphere"))
	bool bAutoAtmosphereHeight = true;

	/** Atmosphere thickness as a fraction of PlanetRadiusKm when bAutoAtmosphereHeight is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Layer", meta = (EditCondition = "bHasAtmosphere && bAutoAtmosphereHeight", ClampMin = "0.005", ClampMax = "1.0", UIMin = "0.01", UIMax = "0.35"))
	float AtmosphereHeightRatio = 0.12f;

	/** Manual height of the atmosphere layer above the ground in kilometers (used when bAutoAtmosphereHeight is false). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Layer", meta = (EditCondition = "bHasAtmosphere && !bAutoAtmosphereHeight", ClampMin = "0.005", UIMin = "0.05", UIMax = "200.0"))
	float AtmosphereHeightKm = 12.0f;

	/**
	 * When true, scattering coefficients and scale heights (authored for UE's 60 km Earth reference)
	 * are automatically rescaled to the effective AtmosphereHeightKm so small planets (e.g. 1 km, 25 km, 100 km)
	 * retain rich Earth-like skies and sunsets without manually multiplying scattering scales.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Layer", meta = (EditCondition = "bHasAtmosphere"))
	bool bScalePhysicsToAtmosphereHeight = true;

	/** Factor applied to multiple scattering and ground bounce (matches UE5 MultiScatteringFactor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Layer", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.0", ClampMax = "10.0", UIMin = "0.0", UIMax = "2.0"))
	float MultiScatteringFactor = 1.0f;

	// -----------------------------------------------------------------------
	// Atmosphere - Rayleigh (defaults match UE5 USkyAtmosphereComponent)
	// -----------------------------------------------------------------------

	/** Rayleigh scattering coefficient scale (1/km in 60 km reference atmosphere). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Rayleigh", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float RayleighScatteringScale = 0.033100f;

	/** Normalized Rayleigh scattering color at sea level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Rayleigh", meta = (EditCondition = "bHasAtmosphere", HideAlphaChannel))
	FLinearColor RayleighScattering = FLinearColor(0.175287f, 0.409607f, 1.0f, 1.0f);

	/** Altitude (km in 60 km reference) at which Rayleigh density drops to 1/e (~37%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Rayleigh", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.001", UIMin = "0.01", UIMax = "20.0"))
	float RayleighExponentialDistribution = 8.0f;

	// -----------------------------------------------------------------------
	// Atmosphere - Mie (defaults match UE5 USkyAtmosphereComponent)
	// -----------------------------------------------------------------------

	/** Mie scattering coefficient scale (1/km in 60 km reference atmosphere). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Mie", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0"))
	float MieScatteringScale = 0.003996f;

	/** Mie scattering color at sea level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Mie", meta = (EditCondition = "bHasAtmosphere", HideAlphaChannel))
	FLinearColor MieScattering = FLinearColor::White;

	/** Mie absorption coefficient scale (1/km in 60 km reference atmosphere). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Mie", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0"))
	float MieAbsorptionScale = 0.000444f;

	/** Mie absorption color at sea level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Mie", meta = (EditCondition = "bHasAtmosphere", HideAlphaChannel))
	FLinearColor MieAbsorption = FLinearColor::White;

	/** Mie Henyey-Greenstein anisotropy [0..0.995]. Closer to 1 creates sharper forward sun halos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Mie", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.0", ClampMax = "0.995", UIMin = "0.0", UIMax = "0.995"))
	float MieAnisotropy = 0.8f;

	/** Altitude (km in 60 km reference) at which Mie aerosol density drops to 1/e. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Mie", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.001", UIMin = "0.01", UIMax = "10.0"))
	float MieExponentialDistribution = 1.2f;

	// -----------------------------------------------------------------------
	// Atmosphere - Absorption / Ozone (defaults match UE5 USkyAtmosphereComponent)
	// -----------------------------------------------------------------------

	/** Secondary absorption layer scale (defaults to Earth's ozone). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Absorption", meta = (EditCondition = "bHasAtmosphere", DisplayName = "Absorption Scale", ClampMin = "0.0", UIMin = "0.0", UIMax = "0.2"))
	float OtherAbsorptionScale = 0.001881f;

	/** Secondary absorption color (defaults to Earth's ozone). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Absorption", meta = (EditCondition = "bHasAtmosphere", DisplayName = "Absorption", HideAlphaChannel))
	FLinearColor OtherAbsorption = FLinearColor(0.345561f, 1.0f, 0.045189f, 1.0f);

	/** Altitude-based tent distribution of absorption particles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Absorption", meta = (EditCondition = "bHasAtmosphere", DisplayName = "Tent Distribution"))
	FCosmicAtmosphereTentDistribution OtherTentDistribution;

	// -----------------------------------------------------------------------
	// Art Direction & Aerial Perspective
	// -----------------------------------------------------------------------

	/** Overall luminance multiplier / artistic tint for the atmosphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Art Direction", meta = (EditCondition = "bHasAtmosphere", HideAlphaChannel))
	FLinearColor SkyLuminanceFactor = FLinearColor::White;

	/** Scales aerial perspective thickness over terrain and opaque meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Art Direction", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.0", UIMin = "0.0", UIMax = "3.0"))
	float AerialPerspectiveViewDistanceScale = 1.0f;

	/** Distance (in km) from the camera before aerial perspective starts fading in on opaque surfaces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Art Direction", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.0", UIMin = "0.0", UIMax = "5.0"))
	float AerialPerspectiveStartDepthKm = 0.05f;

	// -----------------------------------------------------------------------
	// Sun Lighting & Solar Disk
	// -----------------------------------------------------------------------

	/** Automatically tracks the scene's primary ADirectionalLight for sun direction and color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Lighting", meta = (EditCondition = "bHasAtmosphere"))
	bool bAutoSyncDirectionalLight = true;

	/** Manual sun direction (pointing toward the sun) used when bAutoSyncDirectionalLight is false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Lighting", meta = (EditCondition = "bHasAtmosphere && !bAutoSyncDirectionalLight"))
	FVector ManualSunDirection = FVector(-0.93f, -0.03f, 0.36f);

	/** Multiplier applied to the synced DirectionalLight intensity for atmospheric radiance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Lighting", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.0"))
	float SunIntensityScale = 1.5f;

	/** Renders a transmittance-attenuated solar disk through the atmosphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Lighting", meta = (EditCondition = "bHasAtmosphere"))
	bool bRenderSunDisk = true;

	/** Angular radius of the solar disk in degrees (Earth's sun is ~0.535 deg). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Lighting", meta = (EditCondition = "bHasAtmosphere && bRenderSunDisk", ClampMin = "0.05", ClampMax = "10.0"))
	float SunDiskRadiusDeg = 0.65f;

	/** Brightness multiplier for the rendered solar disk. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Lighting", meta = (EditCondition = "bHasAtmosphere && bRenderSunDisk", ClampMin = "0.0"))
	float SunDiskIntensity = 25.0f;

	// -----------------------------------------------------------------------
	// Performance & Rendering
	// -----------------------------------------------------------------------

	/** View raymarch samples when close to or inside the atmosphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Performance", meta = (EditCondition = "bHasAtmosphere", ClampMin = "8", ClampMax = "64"))
	int32 NearViewSamples = 24;

	/** View raymarch samples when viewing the planet from far orbital space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Performance", meta = (EditCondition = "bHasAtmosphere", ClampMin = "6", ClampMax = "32"))
	int32 FarViewSamples = 12;

	/** Optical depth transmittance samples along each sun ray. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Performance", meta = (EditCondition = "bHasAtmosphere", ClampMin = "2", ClampMax = "16"))
	int32 SunTransmittanceSamples = 6;

	/** Camera altitude above surface (km) where LOD transition begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Performance", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.0"))
	float LodNearAltitudeKm = 15.0f;

	/** Camera altitude above surface (km) where orbital LOD is fully active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Performance", meta = (EditCondition = "bHasAtmosphere", ClampMin = "0.5"))
	float LodFarAltitudeKm = 150.0f;

	/** Tessellation segments of the dual-winding proxy bounding sphere. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Performance", meta = (EditCondition = "bHasAtmosphere", ClampMin = "16", ClampMax = "96"))
	int32 SphereSegments = 48;

	/** Translucency sort priority (default 5 so clouds at 10 composite cleanly over the sky). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atmosphere|Performance", meta = (EditCondition = "bHasAtmosphere"))
	int32 TranslucencySortPriority = 5;

private:
	/** Ensures AtmosphereShellMesh exists, is attached, and has its dual-winding unit spheres built. */
	void EnsureAtmosphereShellMesh();

	/** Generates both outward (section 0) and inward (section 1) unit spheres for seamless depth switching. */
	void BuildDualWindingUnitSpheres(UProceduralMeshComponent* MeshComp, int32 LongitudeSegments, int32 LatitudeSegments) const;

	/** Switches between exterior (hardware depth tested) and interior (depth-test disabled) shell sections. */
	void UpdateCameraShellMode(bool bForce = false);

	/** Resolves active camera world location in both Editor viewports and Game/PIE worlds. */
	bool GetActiveCameraLocation(FVector& OutCameraLocation) const;

	/** Applies all atmosphere parameters to a target dynamic material instance. */
	void ApplyParametersToMID(UMaterialInstanceDynamic* MID, const FVector& SunDir, const FLinearColor& SunCol) const;

	/** Updates the scale of AtmosphereShellMesh to tightly enclose BottomRadius + AtmosphereHeight. */
	void UpdateShellScale();

	/** Resolves sun direction and radiance from the scene's ADirectionalLight (or manual fallback). */
	void ResolveSunLighting(FVector& OutSunDir, FLinearColor& OutSunColor);

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> AtmosphereShellMesh = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicAtmosphereMaterial = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicAtmosphereMaterialInside = nullptr;

	TWeakObjectPtr<ADirectionalLight> CachedDirectionalLight;

	int32 BuiltSphereSegments = 0;
	bool bUsingInsideShell = false;
	bool bShellModeInitialized = false;
	float LightRefreshTimer = 0.0f;
	FVector LastSyncedSunDir = FVector::ZeroVector;
	FLinearColor LastSyncedSunColor = FLinearColor::Black;
};
