// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ICosmicCollisionTarget.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCosmicCollisionTarget : public UInterface
{
	GENERATED_BODY()
};

class ACosmicPlanet;

/**
 * Interface implemented by actors that require procedural collision patches on planets.
 * Allows actors (players, spaceships, vehicles, AI, physics props) to declare their
 * collision relevance and priority to UCosmicPlanetCollisionManager.
 */
class COSMICARCHITECTRUNTIME_API ICosmicCollisionTarget
{
	GENERATED_BODY()

public:
	/**
	 * Indicates whether this actor currently needs planetary collision.
	 * (Temporarily bypassed while relevance culling is disabled).
	 */
	virtual bool IsCollisionRelevant() const { return true; }

	/**
	 * Returns the base priority score of this actor for collision allocation (0.0 to 1.0).
	 * (Temporarily bypassed while relevance culling is disabled).
	 */
	virtual float GetCollisionPriority() const { return 0.5f; }

	/**
	 * Finds all ACosmicPlanet actors in the given world.
	 */
	static void FindCosmicPlanets(const UWorld* World, TArray<ACosmicPlanet*>& OutPlanets);

	/**
	 * Finds the nearest ACosmicPlanet to the specified target actor.
	 */
	static ACosmicPlanet* FindNearestPlanet(const AActor* TargetActor, const TArray<ACosmicPlanet*>& Planets);

	/**
	 * Discovers all planets in the world and registers TargetActor to the nearest planet's collision manager.
	 * Optionally outputs the list of discovered planets.
	 *
	 * @return The planet subscribed to, or nullptr if none was found.
	 */
	static ACosmicPlanet* RegisterAndSubscribeToNearestPlanet(
		AActor* TargetActor,
		TArray<TWeakObjectPtr<ACosmicPlanet>>* OutRegisteredPlanets = nullptr
	);

	/**
	 * Unsubscribes TargetActor from a planet's collision manager.
	 */
	static void UnsubscribeFromPlanet(AActor* TargetActor, ACosmicPlanet* Planet);
};
