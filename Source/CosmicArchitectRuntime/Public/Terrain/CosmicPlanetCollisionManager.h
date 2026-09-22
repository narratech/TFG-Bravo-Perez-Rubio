// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicPlanetCollisionManager.generated.h"

COSMICARCHITECTRUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogCosmicCollision, Log, All);

class UCosmicCollisionComponent;
class ICosmicNoiseStrategy;
class ACosmicPlanet;

/**
 * Internal tracking entry for an active collision patch assigned to an actor.
 */
USTRUCT()
struct FCosmicTrackedActorPatch
{
	GENERATED_BODY()

	/** Weak pointer to the actor receiving ground collision */
	UPROPERTY()
	TWeakObjectPtr<AActor> TrackedActor;

	/** Physical collision patch component allocated to this actor */
	UPROPERTY()
	TObjectPtr<UCosmicCollisionComponent> CollisionPatch = nullptr;

	/** Last actor location when collision update was requested */
	FVector LastActorLocation = FVector(MAX_flt);

	/** Calculated multi-factor relevance score for sorting/budgeting */
	float RelevanceScore = 0.0f;
};

/**
 * Procedural planet collision manager.
 *
 * Runs on both Clients and Dedicated Server.
 * Responsible for discovering candidate actors near the planetary surface,
 * evaluating their relevance (proximity, camera visibility, physics/movement state, priority),
 * and dynamically allocating, updating, and pooling ping-pong collision patches
 * (UCosmicCollisionComponent) underneath them.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
	HideCategories = (Rendering, Tags, Activation, AssetUserData, Navigation, Replication, ComponentReplication, Cooking, Collision, Input, Actor))
class COSMICARCHITECTRUNTIME_API UCosmicPlanetCollisionManager : public UActorComponent
{
	GENERATED_BODY()

public:

	UCosmicPlanetCollisionManager();

	/** Size of each triangle used for collision (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Collision")
	float CollisionTriangleSize = 250.f;

	/** Collision grid resolution (quads per side) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Collision")
	int32 CollisionResolution = 16;

	/** Number of grid cells the actor must travel before triggering a collision update */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Collision", meta = (ClampMin = "1"))
	int32 UpdateCellInterval = 4;

	/** Maximum distance from planet surface to generate collision (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Collision")
	double MaxCollisionDistance = 30000.f;

	/** Maximum distance from local viewer to consider remote actors relevant on clients (cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Collision")
	float MaxRemoteActorDistance = 30000.f;

	/** Maximum number of concurrent collision patches allowed simultaneously */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Collision", meta = (ClampMin = "1", ClampMax = "64"))
	int32 MaxConcurrentPatches = 16;

	/** If true, boosts relevance of actors currently visible in the local camera frustum / recently rendered */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Collision")
	bool bFilterByVisibility = true;

	/** Show collision debug mesh in viewport */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet Collision")
	bool bShowCollisionMesh = false;

	/** Color used to visualize active collision mesh */
	UPROPERTY(EditAnywhere, Category = "Planet Collision", meta = (EditCondition = "bShowCollisionMesh"))
	FColor DebugColor = FColor::Green;

	/** Color used to visualize in-flight standby collision mesh */
	UPROPERTY(EditAnywhere, Category = "Planet Collision", meta = (EditCondition = "bShowCollisionMesh"))
	FColor StandbyDebugColor = FColor(255, 165, 0);

	/** Debug line width */
	UPROPERTY(EditAnywhere, Category = "Planet Collision", meta = (EditCondition = "bShowCollisionMesh", ClampMin = "0"))
	float DebugLineWidth = 20.f;

	/** Use complex collision as simple collision */
	UPROPERTY(EditAnywhere, Category = "Planet Collision")
	bool bUseComplexAsSimpleCollision = true;

	/** Use asynchronous cooking for physics */
	UPROPERTY(EditAnywhere, Category = "Planet Collision")
	bool bUseAsyncCooking = true;

	/** Subscribes an actor to receive procedural planetary collision from this manager */
	UFUNCTION(BlueprintCallable, Category = "Planet Collision")
	void RegisterCollisionTarget(AActor* TargetActor);

	/** Unsubscribes an actor from this planet's collision */
	UFUNCTION(BlueprintCallable, Category = "Planet Collision")
	void UnregisterCollisionTarget(AActor* TargetActor);

	/** Finds the nearest planet in the world and registers the actor to its collision manager */
	static ACosmicPlanet* SubscribeTargetToNearestPlanet(AActor* TargetActor);

	/** Completely clears all active patches and returns them to pool or destroys them */
	UFUNCTION(BlueprintCallable, Category = "Planet Collision")
	void ClearAllPatches();

	/** Updates procedural collision patches for all registered actors. Returns true if any patch was requested to update or allocated this frame. */
	bool UpdateCollisions();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	/** Pool of pre-instantiated or recycled collision patch components */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCosmicCollisionComponent>> PatchPool;

	/** List of currently active tracked patches assigned to actors */
	UPROPERTY(Transient)
	TArray<FCosmicTrackedActorPatch> ActiveTrackedPatches;

	/** List of explicitly subscribed targets receiving collision updates from this manager */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> SubscribedTargets;

	/** (Temporarily bypassed) Calculates multi-factor relevance score for an actor candidate */
	float CalculateActorRelevance(
		AActor* Candidate,
		const FVector& LocalViewerPos,
		const FVector& LocalViewerForward,
		bool bHasLocalViewer,
		double DistToSurface
	) const;

	/** Acquires a collision patch component from pool or spawns a new one attached to planet */
	UCosmicCollisionComponent* AcquirePatchFromPool(ACosmicPlanet* Planet, double PlanetRadius);

	/** Recycles a patch back into the pool */
	void RecyclePatch(UCosmicCollisionComponent* Patch);
};
