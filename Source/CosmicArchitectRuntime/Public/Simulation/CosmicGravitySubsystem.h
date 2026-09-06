// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CosmicGravitySubsystem.generated.h"

class UCosmicGravityComponent;

/**
 * World subsystem responsible for managing full gravitational simulation of the level.
 *
 * Acts as central coordinator for all registered bodies, calculating and distributing
 * gravitational forces each frame according to the mode configured on each UCosmicGravityComponent.
 *
 * Main responsibilities:
 *   - Maintain two internal lists: Bodies (all bodies) and Planets (planets only).
 *   - Calculate gravitational forces in Tick() and accumulate them in each component.
 *   - Invoke Integrate() on each active body after accumulating all frame forces.
 *   - Provide RegisterBody() and UnregisterBody() as entry points for components. 
 *
 * Restrictions and usage contracts:
 *   - Tick only activates when bodies are registered (automatic optimization).
 *   - Bodies and Planets must not be modified directly from outside subsystem.
 *   - Components must register in BeginPlay and unregister in EndPlay.
 */
UCLASS()
class COSMICARCHITECTRUNTIME_API UCosmicGravitySubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    /**
     * Initializes subsystem when level is created or loaded.
     * Entry point for future initializations depending on the World.
     */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /**
     * Empties internal lists and releases references when level is destroyed.
     * Guarantees no dangling references remain to destroyed components.
     */
    virtual void Deinitialize() override;

    /**
     * Core of the simulation. Executes once per frame while bodies are registered.
     *
     * Phase 1: Force accumulation.
     *   Traverses all bodies and calculates gravitational forces according to GravityMode,
     *   accumulating them into AccumulatedForce on each affected component.
     *
     * Phase 2: Integration.
     *   Invokes Integrate(DeltaTime) on each active body to apply resulting movement.
     *
     * @param DeltaTime Time in seconds elapsed since last frame.
     */
    virtual void Tick(float DeltaTime) override;

    /**
     * Indicates whether subsystem should execute Tick this frame.
     * Returns false if no bodies registered or World invalid,
     * avoiding CPU cost when simulation is idle.
     */
    virtual bool IsTickable() const override;

    /**
     * Stat identifier required by FTickableGameObject.
     * Allows Unreal profiler to measure subsystem cost separately.
     */
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UGravitySubsystem, STATGROUP_Tickables);
    }

    /**
     * Returns registered planets list for external reading.
     * Allows other systems to query active planets without accessing full Bodies list.
     */
    const TArray<UCosmicGravityComponent*>& GetPlanets() const { return Planets; }

    /**
     * Registers a component into gravitational simulation.
     * If component is a planet, also added to Planets sublist.
     * Uses AddUnique to avoid duplicates on multiple calls.
     *
     * @param Body Component to register. Ignored if nullptr.
     */
    void RegisterBody(UCosmicGravityComponent* Body);

    /**
     * Removes a component from gravitational simulation.
     * If component was a planet, also removed from Planets sublist.
     *
     * @param Body Component to unregister. Ignored if nullptr.
     */
    void UnregisterBody(UCosmicGravityComponent* Body);

    /**
     * Returns gravitational constant G in SI units (m³ / (kg * s²)).
     * Used by UCosmicGravityComponent to calculate planet masses in BeginPlay.
     */
    double GetGravityConstant() const;

    /**
     * Returns World in which this tickable object operates.
     * Required by FTickableGameObject interface to validate execution context.
     */
    virtual UWorld* GetTickableGameObjectWorld() const override;

private:
    /**
     * Calculates gravitational force BodyB exerts on BodyA and accumulates it in BodyA.
     * Unidirectional operation: only BodyA receives resulting force.
     * Used by NearestPlanet, SpecificPlanet, and AllPlanets modes.
     *
     * @param BodyA Body receiving gravitational force.
     * @param BodyB Body acting as gravity source.
     */
    void BodyAddForce(UCosmicGravityComponent* BodyA, UCosmicGravityComponent* BodyB);

    /**
     * Calculates and applies mutual gravitational forces between two bodies (Newton's Third Law).
     * BodyA receives force towards BodyB and BodyB receives force towards BodyA in same calculation.
     * Used exclusively by NBody mode to avoid redundant calculations (j > i).
     *
     * @param BodyA First body of pair.
     * @param BodyB Second body of pair.
     */
    void ApplyMutualForce(UCosmicGravityComponent* BodyA, UCosmicGravityComponent* BodyB);

    /**
     * List of all active bodies registered in current level simulation.
     * Includes both planets and orbital bodies. Marked UPROPERTY so
     * Unreal garbage collector does not invalidate references.
     */
    UPROPERTY()
    TArray<UCosmicGravityComponent*> Bodies;

    /**
     * Optimized sublist containing only bodies with IsPlanet == true.
     * Allows NearestPlanet, SpecificPlanet, and AllPlanets modes to iterate only
     * over planets without filtering full Bodies list every frame.
     */
    TArray<UCosmicGravityComponent*> Planets;
};