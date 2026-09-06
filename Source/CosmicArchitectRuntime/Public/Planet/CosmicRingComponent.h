// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CosmicRingComponent.generated.h"

/**
 * Delegate to notify external systems or Blueprints when an
 * asteroid sector has finished its asynchronous generation.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAsteroidFieldGenerated);

/**
 * UCosmicRingComponent
 *
 * Manages the visual and physical representation of planetary rings at true scale.
 * Implements a "Treadmill" system segmenting the ring into angular sectors,
 * dynamically loading and recycling asteroid instances (HISM) based on
 * observer proximity to optimize performance and memory.
 */ 
UCLASS(ClassGroup = (CosmicArchitect), meta = (BlueprintSpawnableComponent),
	HideCategories = (Rendering, Lighting, Navigation, Replication, Physics, Collision,
		Activation, AssetUserData, HLOD, Cooking, Tags, ComponentReplication))
	class COSMICARCHITECTRUNTIME_API UCosmicRingComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Subobject initialization and default component configuration. */
	UCosmicRingComponent();

protected:
	/** Initializes dynamic material state and transformations at simulation start. */
	virtual void BeginPlay() override;

#if WITH_EDITOR
	/** Updates visual properties in editor viewport upon parameter modification. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	/** Manages sector lifecycle and camera detection on each frame. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Configures component after world registration, setting initial scales and materials. */
	virtual void OnRegister() override;

	/** Ensures component maintains neutral relative position with respect to parent. */
	virtual void OnAttachmentChanged() override;

	/** Memory cleanup and destruction of instanced components before deleting object. */
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	// --- DESIGN PROPERTIES ---

	/** Base material interface for macro disc (LWC Shader). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	class UMaterialInterface* MacroRingMaterial;

	/** Static mesh used to represent each individual asteroid in sectors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	class UStaticMesh* AsteroidMesh;

	/** Base color for ring dust and asteroids. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	FLinearColor RingColor = FLinearColor(0.2f, 0.3f, 1.0f, 1.0f);

	/** Frequency of noise bands in procedural material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	double BandFrequency = 50.0;

	// --- DIMENSIONS ---

	/** True inner ring radius in Kilometers. Also determines shader internal UV mask. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double InnerRadiusKM = 2.0;

	/** True outer ring radius in Kilometers. Determines macro disc scale and external UV mask. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double OuterRadiusKM = 5.0;

	/** Total vertical ring thickness in Kilometers. Controls asteroid Z dispersion and proximity detection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double RingThicknessKM = 0.4;

	/** Orbital rotation of the ring system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	FRotator RingRotation = FRotator::ZeroRotator;

	// --- LOD ---

	/** Minimum random scale for asteroid instances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	float MinScale = 0.01f;

	/** Maximum random scale for asteroid instances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	float MaxScale = 0.05f;

	/**
	 * Distance in KM from the closest ring point (edge or surface) at which
	 * 3D asteroids begin generating. Evaluated on actual ring geometry
	 * (annulus + thickness), not from the component center.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	double AsteroidActivationDistanceKM = 8.0;

	/** Distance in KM where macro shader fade begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	double FadeMinDistanceKM = 1.0;

	/** Distance in KM where macro shader becomes completely hidden. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	double FadeMaxDistanceKM = 8.0;

	// --- OPTIMIZATION AND PERFORMANCE ---

	/** Angular width of each generated sector (in degrees). */
	UPROPERTY(EditAnywhere, Category = "Cosmic Architect | Optimization")
	float SectorAngleDegrees = 15.0f;

	/** Number of sectors adjacent to observer position kept active. */
	UPROPERTY(EditAnywhere, Category = "Cosmic Architect | Optimization")
	int32 VisibleSectors = 2;

	/** Number of individual asteroids to generate per active sector. */
	UPROPERTY(EditAnywhere, Category = "Cosmic Architect | Optimization")
	int32 AsteroidsPerSector = 500;

	/**
	 * Limit of instances (asteroids) processed per second between sector creation and destruction.
	 * A sector in progress always completes even if exceeding limit in that frame.
	 * Pending sectors are processed on the following frame.
	 */
	UPROPERTY(EditAnywhere, Category = "Cosmic Architect | Optimization")
	int32 MaxInstancesPerSecond = 500;

private:
	/** Component that renders macro ring material. */
	UPROPERTY(VisibleAnywhere, Category = "Cosmic Architect | Internal")
	class UStaticMeshComponent* MacroDiskComponent;

	/** Pointer to dynamic instance for manipulating shader parameters in real time. */
	UPROPERTY()
	class UMaterialInstanceDynamic* DynamicRingMat;

	/** Map associating sector IDs with their respective active HISM components. */
	UPROPERTY()
	TMap<int32, UHierarchicalInstancedStaticMeshComponent*> ActiveSectors;

	/** Repository of inactive HISM components for immediate reuse (Pooling). */
	UPROPERTY()
	TArray<UHierarchicalInstancedStaticMeshComponent*> HISMPool;

	/** Retrieves a HISM component from pool or creates new one if none available. */
	UHierarchicalInstancedStaticMeshComponent* GetOrCreateHISM();

	/**
	 * Synchronizes C++ property values with Material Instance parameters.
	 * UV radii are calculated automatically from InnerRadiusKM and OuterRadiusKM.
	 */
	void UpdateShaderParameters();

	/**
	 * Invalidates all active sectors returning them to pool to force regeneration
	 * on next tick. Called when properties affecting geometry change in editor.
	 */
	void InvalidateAllSectors();

	/**
	 * Calculates distance in centimeters from a position (in component local space)
	 * to the nearest point on ring volume (annulus + vertical thickness).
	 */
	float ComputeDistanceToRing(const FVector& LocalPosition) const;
};