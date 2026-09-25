// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Terrain/ICosmicCollisionTarget.h"
#include "CosmicCollisionTargetComponent.generated.h"

class ACosmicPlanet;
class UCosmicPlanetCollisionManager;

/**
 * Procedural planetary collision target component.
 *
 * Attach this component to any Actor (Player, Spaceship, Vehicles, NPCs, Physics Props)
 * to automatically request and manage procedural collision patches from the nearest planet's
 * UCosmicPlanetCollisionManager.
 */
UCLASS(ClassGroup = (CosmicArchitect), meta = (BlueprintSpawnableComponent))
class COSMICARCHITECTRUNTIME_API UCosmicCollisionTargetComponent : public UActorComponent, public ICosmicCollisionTarget
{
	GENERATED_BODY()

public:
	UCosmicCollisionTargetComponent();

	// ~ICosmicCollisionTarget interface
	virtual bool IsCollisionRelevant() const override;
	virtual float GetCollisionPriority() const override;
	// ~End ICosmicCollisionTarget interface

	/** Base priority score for collision patch allocation (0.0 to 1.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Collision", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Priority = 0.8f;

	/** Whether this actor currently requires procedural collision */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Collision")
	bool bIsCollisionRelevant = true;

	/** If true, automatically elevates priority to 1.0 for locally controlled player pawns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Collision")
	bool bAutoElevatePlayerPriority = true;

	/** Interval in seconds between checks to see if the actor has moved closer to another planet */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Collision", meta = (ClampMin = "0.1", UIMin = "0.1"))
	float PlanetCheckInterval = 1.5f;

	/** Periodically checks all discovered planets and updates subscription to the nearest one */
	UFUNCTION(BlueprintCallable, Category = "Cosmic Collision")
	void UpdateNearestPlanetSubscription();

	/** Manually sets collision priority score */
	UFUNCTION(BlueprintCallable, Category = "Cosmic Collision")
	void SetCollisionPriority(float NewPriority) { Priority = FMath::Clamp(NewPriority, 0.0f, 1.0f); }

	/** Manually enables or disables collision relevance */
	UFUNCTION(BlueprintCallable, Category = "Cosmic Collision")
	void SetCollisionRelevant(bool bNewRelevant) { bIsCollisionRelevant = bNewRelevant; }

	/** Returns the currently subscribed planet, if any */
	UFUNCTION(BlueprintPure, Category = "Cosmic Collision")
	ACosmicPlanet* GetCurrentPlanet() const;

	/** Returns the currently subscribed collision manager, if any */
	UFUNCTION(BlueprintPure, Category = "Cosmic Collision")
	UCosmicPlanetCollisionManager* GetCurrentCollisionManager() const { return CurrentPlanetCollisionManager.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Discovered planets in the world */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ACosmicPlanet>> RegisteredPlanets;

	/** Currently subscribed planetary collision manager */
	UPROPERTY(Transient)
	TWeakObjectPtr<UCosmicPlanetCollisionManager> CurrentPlanetCollisionManager;

	/** Timer handle for periodic nearest planet verification */
	FTimerHandle PlanetCheckTimerHandle;
};
