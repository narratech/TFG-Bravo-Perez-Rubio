// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicOrbitComponent.generated.h"

/**
 * Component responsible for simulating orbital motion
 * and axial rotation for celestial bodies.
 *
 * The system implements:
 * - Basic elliptical orbits
 * - Local actor rotation
 * - Three-dimensional orbital inclination
 * - Debug visualization in editor
 *
 * The orbit is calculated using a Keplerian approximation
 * based on mean anomaly and eccentric anomaly.
 */
UCLASS(ClassGroup = (Cosmic), meta = (BlueprintSpawnableComponent),
	HideCategories = (Navigation, Replication, Activation, AssetUserData, Cooking, Tags))
	class COSMICARCHITECTRUNTIME_API UCosmicOrbitComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	// ============================================================
	// CONSTRUCTION
	// ============================================================

	/**
	 * Initializes orbital component with default values.
	 *
	 * Configures:
	 * - Real-time tick
	 * - Editor simulation
	 * - Initial orbital state
	 */
	UCosmicOrbitComponent();

#if WITH_EDITOR

	/**
	 * Responds to property changes from the editor.
	 *
	 * Automatically updates:
	 * - Initial orbital position
	 * - Debug visualization
	 * - Attachment relationships
	 *
	 * @param PropertyChangedEvent Information on modified property.
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

protected:

	// ============================================================
	// ENGINE LIFECYCLE
	// ============================================================

	/**
	 * Initializes orbital state at simulation start.
	 */
	virtual void BeginPlay() override;

public:

	// ============================================================
	// RUNTIME UPDATE
	// ============================================================

	/**
	 * Updates orbital simulation each frame.
	 *
	 * Responsibilities:
	 * - Orbital temporal integration
	 * - Eccentric anomaly resolution
	 * - Relative position update
	 * - Axial actor rotation
	 * - Debug visualization in editor
	 *
	 * @param DeltaTime Time elapsed since previous frame.
	 * @param TickType Current tick type.
	 * @param ThisTickFunction Information of current tick.
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================
	// ORBIT STATE
	// ============================================================

	/**
	 * Central body around which this actor orbits.
	 *
	 * Owning actor will move using relative coordinates
	 * with respect to this parent body.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit State")
	AActor* ParentBody;

	/**
	 * Current accumulated orbital time.
	 *
	 * Expressed in simulated seconds.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit State")
	float CurrentOrbitTime = 0.0f;

	// ============================================================
	// ORBIT PARAMETERS
	// ============================================================

	/**
	 * Semi-major axis of the orbit.
	 *
	 * Defines overall size of orbital trajectory.
	 *
	 * Unit: kilometers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params")
	float SemiMajorAxisKm = 1.0f;

	/**
	 * Orbital eccentricity.
	 *
	 * Values:
	 * - 0.0  -> circular orbit
	 * - 0-1  -> elliptical orbit
	 *
	 * Values close to 1 produce extremely elongated orbits.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "1"))
	float Eccentricity = 0.0f;

	/**
	 * Initial position along orbit.
	 *
	 * Represents normalized fraction of orbital period.
	 *
	 * Range:
	 * - 0.0 -> orbit start
	 * - 1.0 -> complete orbit
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "1"))
	float InitialPosition = 0.0f;

	/**
	 * Time required to complete a full orbit.
	 *
	 * Unit: seconds.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params")
	float OrbitalPeriod = 10.0f;

	/**
	 * Orbital inclination about X axis.
	 *
	 * Unit: degrees.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "360"))
	float InclinationX = 0.0f;

	/**
	 * Orbital inclination about Y axis.
	 *
	 * Unit: degrees.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "360"))
	float InclinationY = 0.0f;

	/**
	 * Orbital inclination about Z axis.
	 *
	 * Unit: degrees.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "360"))
	float InclinationZ = 0.0f;

	// ============================================================
	// ROTATION
	// ============================================================

	/**
	 * Axial rotation speed of actor.
	 *
	 * Controls local rotation applied about yaw axis.
	 *
	 * Unit: degrees per second.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	float SpinSpeed = 0.0f;

	// ============================================================
	// EDITOR STATE
	// ============================================================

	/**
	 * Indicates whether orbital system is being simulated in editor.
	 */
	UPROPERTY()
	bool bEditorSimulating = false;

	/**
	 * Global multiplier for orbital speed.
	 *
	 * Used mainly for preview
	 * and temporal control from editor tools.
	 */
	UPROPERTY()
	float EditorSpeedMultiplier = 1.0f;

	// ============================================================
	// INITIALIZATION
	// ============================================================

	/**
	 * Initializes basic visual parameters of orbit.
	 *
	 * @param color Color used for debug visualization.
	 */
	void InitOrbit(FColor color = FColor::Cyan);

protected:

	// ============================================================
	// INTERNAL STATE
	// ============================================================

	/**
	 * Indicates whether component is being previewed in editor.
	 */
	bool bIsInEditorPreview = false;

	/**
	 * Calculates and applies initial orbital position.
	 *
	 * Position obtained using:
	 * - orbital period
	 * - eccentricity
	 * - normalized initial position
	 */
	void UpdateInitialOrbitPosition();

private:

	// ============================================================
	// ORBIT VISUALIZATION
	// ============================================================

	/**
	 * Generates debug visual representation of orbit.
	 *
	 * Orbit is drawn using linear segments
	 * approximating an elliptical trajectory.
	 *
	 * @note Only available in editor.
	 */
	void UpdateOrbitVisualization();

	/**
	 * Number of segments used to approximate orbit.
	 *
	 * Higher values produce visually smoother orbits
	 * at the cost of higher debug rendering expense.
	 */
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization", meta = (ClampMin = "8", ClampMax = "360"))
	int32 OrbitSegments = 72;

	/**
	 * Color used to represent orbit in editor.
	 */
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization")
	FColor OrbitColor = FColor::White;

	/**
	 * Visual thickness of debug orbital lines.
	 */
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization", meta = (ClampMin = "0.1", ClampMax = "100000"))
	float OrbitThickness = 5000.0f;

	/**
	 * Enables or disables orbital visualization in editor.
	 */
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization")
	bool bShowOrbitInEditor = true;
};