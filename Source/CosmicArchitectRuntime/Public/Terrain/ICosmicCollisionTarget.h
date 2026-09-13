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
	 * Can return false if the actor is dead, invisible, or in deep space.
	 */
	virtual bool IsCollisionRelevant() const { return true; }

	/**
	 * Returns the base priority score of this actor for collision allocation (0.0 to 1.0).
	 * Higher values are prioritized when allocating from the patch pool.
	 */
	virtual float GetCollisionPriority() const { return 0.5f; }
};
