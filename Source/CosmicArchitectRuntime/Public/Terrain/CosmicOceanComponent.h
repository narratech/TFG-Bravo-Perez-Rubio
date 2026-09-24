// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Terrain/CosmicOceanGenerationTask.h"
#include "CosmicOceanComponent.generated.h"

class UCosmicMeshComponent;
class UMaterialInstance;
class UMaterialInstanceDynamic;

/**
 * Component responsible for generating and managing planetary ocean meshes
 * using a single multi-level procedural clipmap for near detail and a full
 * UV sphere for distant orbital observation.
 *
 * All snapping and rescaling are computed asynchronously on a background
 * thread without CPU noise, driving the M_CosmicOceanV3 analytical Gerstner shader.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
	HideCategories = (Activation, Tags, AssetUserData, Navigation, Rendering, Replication, Input, Actor, Collision, Cooking))
class COSMICARCHITECTRUNTIME_API UCosmicOceanComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/**
	 * Ocean component constructor.
	 */
	UCosmicOceanComponent();

	/**
	 * Initializes ocean system.
	 *
	 * @param PlanetRadiusKm Planet radius in kilometers.
	 * @param Parent Parent component to which ocean mesh will be attached.
	 */
	void InitOcean(double PlanetRadiusKm, USceneComponent* Parent);

	/**
	 * Completely regenerates both near clipmap and far sphere ocean meshes.
	 */
	void RegenerateOcean();

	/**
	 * Removes and destroys current ocean meshes.
	 */
	void ClearOcean();

	/**
	 * Clears inherited references after duplication without destroying original actor mesh.
	 *
	 * @param NewRoot Root component of new actor.
	 */
	void ResetPointersAfterDuplicate(USceneComponent* NewRoot);

	/**
	 * Updates the ocean LOD state, projection frame, snapping coordinates, and altitude.
	 * Coordinated by UCosmicClipmapComponent to maintain unified frame synchronization.
	 */
	void UpdateOceanLOD(
		const FTransform& InProjectionFrame,
		const FIntPoint& InCoarsestCenter,
		uint64 InProjectionRevision,
		bool bInPerformanceMode,
		double InDistanceToSurface = -1.0,
		const FVector2D& InViewerCoordinates = FVector2D::ZeroVector);

	/**
	 * Switches between near clipmap mesh and distant sphere mesh.
	 */
	void SetPerformanceMode(bool bActive);

	/**
	 * Checks whether an active asynchronous generation task exists.
	 */
	bool IsTaskActive() const;

	/**
	 * Cancels any active asynchronous task.
	 */
	void CancelAsyncWork();

	/**
	 * Checks whether the background task is finished and uploads updated vertices to GPU.
	 */
	bool CheckAndApplyOceanMeshUpdate();

	/**
	 * Checks whether a background task exists and has completed its computation.
	 */
	FORCEINLINE bool HasCompletedTask() const { return OceanTask != nullptr && OceanTask->IsDone(); }

	/**
	 * Calculates the base grid spacing in centimeters for Level 0.
	 */
	int64 GetCalculatedBaseGridSpacing() const;

	/**
	 * Indicates whether the planet has an ocean.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	bool bHasOcean = true;

	/**
	 * If true, CheckAndApplyOceanMeshUpdate is automatically called on every TickComponent.
	 * If false, external systems (such as UCosmicClipmapComponent) control when the mesh update is applied (e.g. during collision phase).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	bool bAutoApplyInTick = false;

	/**
	 * Sea level relative to planet radius in kilometers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean"))
	double SeaLevelKm = -0.01;

	/**
	 * Base vertex resolution per LOD level (must be divisible by 4, e.g. 64, 128, 256).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Clipmap",
		meta = (EditCondition = "bHasOcean", ClampMin = "8", ClampMax = "128"))
	int32 OceanResolution = 64;

	/**
	 * Number of concentric LOD levels packed into the single near procedural mesh.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Clipmap",
		meta = (EditCondition = "bHasOcean", ClampMin = "1", ClampMax = "8"))
	int32 OceanNumLevels = 6;

	/**
	 * Minimum triangle size allowed in centimeters.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Clipmap",
		meta = (EditCondition = "bHasOcean", ClampMin = "50"))
	int32 MinTriangleSize = 300;

	/**
	 * Base grid spacing for level 0. If bAutoCalculateGridSpacing is true, this is calculated automatically.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Clipmap",
		meta = (EditCondition = "bHasOcean && !bAutoCalculateGridSpacing"))
	int64 OceanBaseGridSpacing = 300;

	/**
	 * Automatically derives OceanBaseGridSpacing from planet radius and NumLevels (matching clipmap).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Clipmap",
		meta = (EditCondition = "bHasOcean"))
	bool bAutoCalculateGridSpacing = true;

	/**
	 * Resolution of the distant spherical ocean mesh used in orbital / performance mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|FarSphere",
		meta = (EditCondition = "bHasOcean", ClampMin = "16", ClampMax = "256"))
	int32 FarSphereResolution = 96;

	/**
	 * Base material used to render ocean (defaults to MI_CosmicOceanV3).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Material",
		meta = (EditCondition = "bHasOcean"))
	UMaterialInstance* OceanMaterial = nullptr;

	/**
	 * Maximum wave height in cm (peak-to-trough amplitude, e.g. 150 = 1.5m).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean", ClampMin = "0.0"))
	float WaveHeight = 150.0f;

	/**
	 * Dominant swell wavelength in cm (distance between crests, e.g. 6000 = 60m).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean", ClampMin = "20.0"))
	float WaveLength = 6000.0f;

	/**
	 * Wave movement speed multiplier (1.0 = standard physical speed).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean", ClampMin = "0.0", ClampMax = "10.0"))
	float WaveSpeed = 0.7f;

	/**
	 * Crest sharpness / trochoid peak [0.0 = smooth swell, 1.0 = sharp peaked waves].
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean", ClampMin = "0.0", ClampMax = "1.0"))
	float WaveSteepness = 0.7f;

	/**
	 * Secondary cross-waves and surface turbulence [0.0 = uniform swell, 1.0 = open sea, 2.0 = stormy].
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean", ClampMin = "0.0", ClampMax = "2.0"))
	float WaveChop = 1.0f;

	/**
	 * Number of active Gerstner waves evaluated by the shader (1 to 12).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean", ClampMin = "1.0", ClampMax = "12.0"))
	float WaveCount = 8.0f;

	/**
	 * Directional dispersion of cross-waves [0.0 = aligned swell, 1.0 = standard, 1.5 = wild].
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean", ClampMin = "0.0", ClampMax = "2.0"))
	float WaveSpread = 2.0f;

	/**
	 * Distance from camera in kilometers where ocean waves begin fading out.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean", ClampMin = "0.1", ClampMax = "100.0"))
	float WaveFadeStartKm = 1.0f;

	/**
	 * Distance from camera in kilometers where ocean waves are completely faded to a calm surface (0 displacement).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean", ClampMin = "0.2", ClampMax = "300.0"))
	float WaveFadeEndKm = 1.5f;

	/**
	 * Primary wind and dominant swell direction vector on the sphere.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean"))
	FVector WindDirection = FVector(0.0f, 0.0f, 1.0f);

	/**
	 * Base surface water tint color.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Appearance",
		meta = (EditCondition = "bHasOcean"))
	FLinearColor WaterColor = FLinearColor(0.15f, 0.5f, 0.66f, 1.0f);

	/**
	 * Deep water absorption color coefficients.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Appearance",
		meta = (EditCondition = "bHasOcean"))
	FLinearColor WaterAbsortion = FLinearColor(0.35f, 0.07f, 0.03f, 1.0f);

	/**
	 * Water scattering color coefficients.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Appearance",
		meta = (EditCondition = "bHasOcean"))
	FLinearColor WaterScattering = FLinearColor(0.029f, 0.0875f, 0.104f, 1.0f);

	/**
	 * Water scattering amount multiplier.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Appearance",
		meta = (EditCondition = "bHasOcean", ClampMin = "0.0", ClampMax = "10.0"))
	float WaterScatteringAmount = 1.0f;

	/**
	 * Base surface roughness for specular highlight.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Appearance",
		meta = (EditCondition = "bHasOcean", ClampMin = "0.0", ClampMax = "1.0"))
	float WaterRoughness = 0.005f;

protected:

	/** Dynamic material instance of near clipmap ocean. */
	UPROPERTY(Transient, DuplicateTransient)
	UMaterialInstanceDynamic* DynamicOceanMat = nullptr;

	/** Dynamic material instance of distant spherical ocean (zero displacement for orbital efficiency). */
	UPROPERTY(Transient, DuplicateTransient)
	UMaterialInstanceDynamic* DynamicFarOceanMat = nullptr;

	/** Single procedural mesh component containing all concentric near clipmap levels. */
	UPROPERTY(Transient, DuplicateTransient)
	UCosmicMeshComponent* NearOceanMesh = nullptr;

	/** Full spherical mesh used for distant observation (orbital / performance mode). */
	UPROPERTY(Transient, DuplicateTransient)
	UCosmicMeshComponent* FarOceanMesh = nullptr;

	/** Root component to which ocean meshes are attached. */
	UPROPERTY(Transient, DuplicateTransient)
	USceneComponent* ParentRoot = nullptr;

	/** Planet radius in centimeters. */
	double PlanetRadiusCm = 100000.0;

	/** Indicates whether ocean system has already been initialized. */
	bool bInit = false;

	/** Indicates whether currently in performance / orbital mode. */
	bool bPerformanceMode = true;

	/** Indicates whether near ocean mesh has received its first valid async update. */
	bool bNearMeshPositioned = false;

	/** Active asynchronous task calculating spherical snapping. */
	FAsyncTask<FCosmicOceanGenerationTask>* OceanTask = nullptr;

	/** Indicates whether an async task is currently running. */
	bool bIsGeneratingOcean = false;

	/** Active tangent frame and center for snapping. */
	FTransform CurrentProjectionFrame = FTransform::Identity;
	FIntPoint CurrentCoarsestCenter = FIntPoint::ZeroValue;
	uint64 CurrentProjectionRevision = 0;

	/** Last applied center and revision. */
	FIntPoint AppliedCoarsestCenter = FIntPoint(MAX_int32, MAX_int32);
	uint64 AppliedProjectionRevision = MAX_uint64;

	/** Current dynamically rescaled base grid spacing. */
	int64 CurrentOceanBaseGridSpacing = 200;

	/** Last applied base grid spacing. */
	int64 AppliedBaseGridSpacing = 200;

	/** Current distance to surface used for rescaling evaluation. */
	double CurrentDistanceToSurface = -1.0;

	/** Last distance to surface that triggered an applied update. */
	double LastAppliedDistanceToSurface = -1.0;

	/** Current viewer coordinates in tangent projection plane. */
	FVector2D CurrentViewerCoordinates = FVector2D::ZeroVector;

	/** Builds the combined multi-level near clipmap mesh. */
	void BuildNearOceanMesh();

	/** Builds the distant spherical mesh. */
	void BuildFarOceanMesh();

	/** Builds and applies dynamic ocean material. */
	void BuildDynamicMaterial();

	/** Updates all wave and appearance parameters on the dynamic material instance. */
	void UpdateWaveParameters();

	/** Requests an asynchronous mesh snapping computation. */
	void RequestOceanMeshUpdate();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:

	/** Updates dynamic ocean material parameters and processes async task completion every frame. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};