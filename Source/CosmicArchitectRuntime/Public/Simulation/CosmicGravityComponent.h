// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "CosmicGravityComponent.generated.h"

/**
 * Defines the gravitational simulation modes available for the component.
 *
 * Each mode determines which celestial bodies influence the owning actor
 * and how the resulting forces are calculated on each simulation frame.
 *
 * Available modes:
 *   - None          -> No active gravitational effect.
 *   - NearestPlanet -> Exclusive attraction towards the nearest planet in scene.
 *   - SpecificPlanet-> Attraction towards a manually defined planet actor.
 *   - AllPlanets    -> Sum of gravitational forces from all planets in scene.
 *   - NBody         -> N-body simulation: all registered objects attract each other mutually. 
 */
UENUM(BlueprintType)
enum class ECosmicGravityMode : uint8
{
    None           UMETA(DisplayName = "None"),
    NearestPlanet  UMETA(DisplayName = "Nearest Planet"),
    SpecificPlanet UMETA(DisplayName = "Specific Planet"),
    AllPlanets     UMETA(DisplayName = "All Planets"),
    NBody          UMETA(DisplayName = "N-Body")
};

/**
 * Manages custom gravitational physics for an actor within the Cosmic Architect system.
 *
 * This component can act in two mutually exclusive roles based on the value of IsPlanet:
 *
 *   PLANET ROLE:
 *     Generates a gravitational field based on its mass, calculated from
 *     RadiusKm and SurfaceGravity. Does not apply engine physics to itself.
 *
 *   ORBITAL BODY ROLE:
 *     Reacts to gravitational fields of registered planets.
 *     Enables engine physics simulation and disables Unreal's internal gravity.
 *
 * Main responsibilities:
 *   - Register and unregister with UCosmicGravitySubsystem on start and end.
 *   - Accumulate gravitational forces per frame via AccumulatedForce.
 *   - Integrate velocity and position (or apply AddForce if using engine physics).
 *   - Expose editable settings from editor for level design.
 *
 * Restrictions and usage contracts:
 *   - Requires UCosmicGravitySubsystem to be active in the World.
 *   - Owning actor must have a UPrimitiveComponent as root component.
 *   - Must not be used alongside Unreal Engine internal gravity (SetEnableGravity = false).
 *   - SetIsPlanet() must be used at runtime to safely switch roles.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
    HideCategories = (Activation, AssetUserData, Cooking, Tags, Navigation))
    class COSMICARCHITECTRUNTIME_API UCosmicGravityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCosmicGravityComponent();

    /**
     * Returns full spatial transform of owning actor.
     * Used by the subsystem to calculate gravitational distances and directions.
     */
    FTransform getTransform() const { return GetOwner()->GetActorTransform(); }

    /**
     * Active gravity mode for this component.
     * Determines which planets are considered when computing forces on this body.
     * Always visible in the editor regardless of object role.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config")
    ECosmicGravityMode GravityMode = ECosmicGravityMode::NearestPlanet;

    /**
     * Mass of the orbital body in kilograms.
     * Used to calculate resulting acceleration (F = m*a) on each integration.
     * Only visible in editor when IsPlanet is false.
     * For planets, mass is automatically computed from RadiusKm and SurfaceGravity.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "!IsPlanet", EditConditionHides))
    double Mass = 100.0f;

    /**
     * Physical radius of planet expressed in kilometers.
     * Combined with SurfaceGravity to calculate gravitational mass of planet.
     * Only visible in editor when IsPlanet is true.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "IsPlanet", EditConditionHides, ClampMin = "0.001",
            UIMin = "0.001", UIMax = "1000000",
            ToolTip = "Radio del planeta en kilómetros"))
    float RadiusKm = 1.0f;

    /**
     * Gravitational acceleration at planet surface (m/s²).
     * Together with RadiusKm, defines planet mass upon game start.
     * Only visible in editor when IsPlanet is true.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "IsPlanet", EditConditionHides, ClampMin = "0.0"))
    float SurfaceGravity = 9.8f;

    /**
     * Indicates whether this object generates a gravitational field on other registered bodies.
     * If false, the subsystem will ignore this object as a gravity source.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config")
    bool AffectsOthers = true;

    /**
     * Indicates whether this object reacts to gravitational fields generated by others.
     * If false, the subsystem will not apply external forces onto this object.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config")
    bool IsAffectedByOthers = true;

    /**
     * Defines whether this actor acts as a planetary body within the simulation.
     *
     * true  -> Acts as planet: generates gravity, does not receive engine physics.
     * false -> Acts as orbital body: receives physics and reacts to gravitational fields.
     *
     * To change this value at runtime, use SetIsPlanet() instead of modifying directly,
     * as subsystem registration must be updated.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity Config")
    bool IsPlanet = false;

    /**
     * Actor acting as exclusive gravity source when mode is SpecificPlanet.
     * Referenced actor must also have an active UCosmicGravityComponent.
     * Only visible in editor when GravityMode == SpecificPlanet.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "GravityMode == ECosmicGravityMode::SpecificPlanet", EditConditionHides))
    AActor* SpecificGravitySource = nullptr;

    /**
     * Reference to root primitive component of owning actor.
     * Cached in BeginPlay to apply physics (AddForce, SetMass, etc.) without repeated searches.
     */
    UPROPERTY()
    UPrimitiveComponent* RootPrimitive = nullptr;

    /** Current body velocity in simulation space (cm/s in Unreal). */
    FVector Velocity = FVector::ZeroVector;

    /** Accumulator of gravitational forces received during current frame (N). Reset after each integration. */
    FVector AccumulatedForce = FVector::ZeroVector;

    /**
     * Normalized direction of dominant gravity from previous frame.
     * Can be read by other systems (character orientation, visual effects, etc.)
     * to know where gravity "points" from this object.
     */
    FVector CurrentGravityDirection = FVector::DownVector;

    /**
     * Applies accumulated forces and integrates body position or velocity.
     *
     * If RootPrimitive is simulating physics (standard orbital bodies),
     * delegates application to engine's AddForce. Otherwise, manually
     * integrates velocity and position using Euler integration.
     *
     * Must be called once per frame from subsystem, after accumulating
     * all gravitational forces of the frame.
     *
     * @param DeltaTime Time elapsed since last frame, in seconds.
     */
    void Integrate(double DeltaTime);

    /**
     * Safely switches object role between planet and orbital body at runtime.
     *
     * Automatically manages unregistration and re-registration in subsystem
     * to keep internal lists of planets and bodies consistent.
     * Does nothing if new value equals current value.
     *
     * @param bNewIsPlanet true to convert to planet, false for orbital body.
     */
    void SetIsPlanet(bool bNewIsPlanet);

protected:
    /**
     * Initializes component at game start.
     *
     * Responsibilities:
     *   - Cache RootPrimitive and configure mobility and physics according to role.
     *   - Compute planet mass if IsPlanet is true.
     *   - Register this component with UCosmicGravitySubsystem.
     */
    virtual void BeginPlay() override;

    /**
     * Cleans up component when actor is destroyed or game ends.
     * Unregisters this component from subsystem to avoid dangling references.
     *
     * @param EndPlayReason Reason EndPlay is invoked.
     */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /**
     * Calculates approximate actor radius in Unreal units (cm) from its bounding box.
     * Returns largest of the three box extents, assuming roughly spherical geometry.
     * Used for collision calculations or surface detection on non-perfectly spherical planets.
     *
     * @return Approximate radius in centimeters (Unreal units).
     */
    float GetObjectRadius() const;
};