// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Terrain/CosmicPlanetCollisionManager.h"
#include "Terrain/CosmicCollisionComponent.h"
#include "Terrain/ICosmicCollisionTarget.h"
#include "ICosmicNoiseStrategy.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY(LogCosmicCollision);

UCosmicPlanetCollisionManager::UCosmicPlanetCollisionManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCosmicPlanetCollisionManager::BeginPlay()
{
	Super::BeginPlay();
}

void UCosmicPlanetCollisionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearAllPatches();
	Super::EndPlay(EndPlayReason);
}

void UCosmicPlanetCollisionManager::RegisterCollisionTarget(AActor* TargetActor)
{
	if (TargetActor && !SubscribedTargets.Contains(TargetActor))
	{
		SubscribedTargets.Add(TargetActor);
		UE_LOG(LogCosmicCollision, Log, TEXT("[CollisionManager] Subscribed target '%s' to planet '%s'"),
			*TargetActor->GetName(), *GetNameSafe(GetOwner()));
	}
}

void UCosmicPlanetCollisionManager::UnregisterCollisionTarget(AActor* TargetActor)
{
	if (!TargetActor) return;

	SubscribedTargets.Remove(TargetActor);

	for (int32 i = ActiveTrackedPatches.Num() - 1; i >= 0; --i)
	{
		if (ActiveTrackedPatches[i].TrackedActor.Get() == TargetActor)
		{
			RecyclePatch(ActiveTrackedPatches[i].CollisionPatch);
			ActiveTrackedPatches.RemoveAt(i);
			break;
		}
	}
}

ACosmicPlanet* UCosmicPlanetCollisionManager::SubscribeTargetToNearestPlanet(AActor* TargetActor)
{
	return ICosmicCollisionTarget::RegisterAndSubscribeToNearestPlanet(TargetActor);
}

void UCosmicPlanetCollisionManager::ClearAllPatches()
{
	for (FCosmicTrackedActorPatch& Entry : ActiveTrackedPatches)
	{
		if (Entry.CollisionPatch)
		{
			RecyclePatch(Entry.CollisionPatch);
		}
	}
	ActiveTrackedPatches.Empty();

	for (UCosmicCollisionComponent* Patch : PatchPool)
	{
		if (Patch)
		{
			Patch->ClearCollision();
			Patch->DestroyComponent();
		}
	}
	PatchPool.Empty();
}

float UCosmicPlanetCollisionManager::CalculateActorRelevance(
	AActor* Candidate,
	const FVector& LocalViewerPos,
	const FVector& LocalViewerForward,
	bool bHasLocalViewer,
	double DistToSurface) const
{
	if (!Candidate || Candidate->IsPendingKillPending())
	{
		return -1.0f;
	}

	// Honor explicit ignore tag
	if (Candidate->ActorHasTag(TEXT("CosmicCollisionIgnore")))
	{
		return -1.0f;
	}

	// Check interface if implemented
	float PriorityMultiplier = 0.5f;
	if (Candidate->GetClass()->ImplementsInterface(UCosmicCollisionTarget::StaticClass()))
	{
		ICosmicCollisionTarget* TargetInterface = Cast<ICosmicCollisionTarget>(Candidate);
		if (TargetInterface)
		{
			if (!TargetInterface->IsCollisionRelevant())
			{
				return -1.0f;
			}
			PriorityMultiplier = FMath::Clamp(TargetInterface->GetCollisionPriority(), 0.0f, 1.0f);
		}
	}

	// Check explicit priority tag
	if (Candidate->ActorHasTag(TEXT("CosmicCollisionTarget")))
	{
		PriorityMultiplier = FMath::Max(PriorityMultiplier, 0.8f);
	}

	const FVector ActorLoc = Candidate->GetActorLocation();

	// Dedicated server or no local viewer available: prioritize all active pawns/physics bodies
	if (!bHasLocalViewer)
	{
		float Score = 100.0f * PriorityMultiplier;

		if (APawn* Pawn = Cast<APawn>(Candidate))
		{
			if (Pawn->IsPlayerControlled())
			{
				Score += 400.0f;
			}
			else
			{
				Score += 200.0f;
			}
		}

		if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Candidate->GetRootComponent()))
		{
			if (RootPrim->IsSimulatingPhysics())
			{
				Score += 150.0f;
			}
		}

		const float ProximityFactor = 1.0f - static_cast<float>(FMath::Clamp(DistToSurface / FMath::Max(1.0, MaxCollisionDistance), 0.0, 1.0));
		Score += ProximityFactor * 50.0f;

		return Score;
	}

	// Client context with local viewer:
	APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	APawn* LocalPawn = LocalPC ? LocalPC->GetPawn() : nullptr;

	// Local player is always maximum priority
	if (Candidate == LocalPawn)
	{
		return 10000.0f;
	}

	const float DistToViewer = FVector::Dist(ActorLoc, LocalViewerPos);

	// Distance culling for remote actors on clients
	if (DistToViewer > MaxRemoteActorDistance && !Candidate->ActorHasTag(TEXT("CosmicCollisionAlwaysRelevant")))
	{
		return -1.0f;
	}

	float Score = (1.0f - (DistToViewer / FMath::Max(1.0f, MaxRemoteActorDistance))) * 100.0f * PriorityMultiplier;

	// Visibility and camera frustum boost
	if (bFilterByVisibility)
	{
		if (Candidate->WasRecentlyRendered(0.25f))
		{
			Score += 100.0f;
		}

		const FVector DirToCandidate = (ActorLoc - LocalViewerPos).GetSafeNormal();
		if ((DirToCandidate | LocalViewerForward) > 0.15f)
		{
			Score += 50.0f; // Inside forward hemisphere/frustum
		}
	}

	// Active movement and physical ground interaction boost
	if (ACharacter* Char = Cast<ACharacter>(Candidate))
	{
		if (UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement())
		{
			if (MoveComp->IsMovingOnGround() || MoveComp->IsFalling())
			{
				Score += 75.0f;
			}
		}
	}
	else if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Candidate->GetRootComponent()))
	{
		if (RootPrim->IsSimulatingPhysics())
		{
			Score += 75.0f;
		}
	}

	return Score;
}

UCosmicCollisionComponent* UCosmicPlanetCollisionManager::AcquirePatchFromPool(double PlanetRadius)
{
	while (PatchPool.Num() > 0)
	{
		UCosmicCollisionComponent* PooledPatch = PatchPool.Pop();
		if (IsValid(PooledPatch))
		{
			PooledPatch->Mobility = EComponentMobility::Stationary;
			PooledPatch->CollisionTriangleSize = CollisionTriangleSize;
			PooledPatch->CollisionResolution = CollisionResolution;
			PooledPatch->UpdateCellInterval = UpdateCellInterval;
			PooledPatch->MaxCollisionDistance = MaxCollisionDistance;
			PooledPatch->bUseComplexAsSimpleCollision = bUseComplexAsSimpleCollision;
			PooledPatch->bUseAsyncCooking = bUseAsyncCooking;
			PooledPatch->bShowCollisionMesh = bShowCollisionMesh;
			PooledPatch->DebugColor = DebugColor;
			PooledPatch->StandbyDebugColor = StandbyDebugColor;
			PooledPatch->DebugLineWidth = DebugLineWidth;
			PooledPatch->GenerateCollisionMesh(PlanetRadius);
			return PooledPatch;
		}
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;

	// Instantiate new collision patch component
	UCosmicCollisionComponent* NewPatch = NewObject<UCosmicCollisionComponent>(OwnerActor, NAME_None, RF_Transient);
	if (NewPatch)
	{
		NewPatch->Mobility = EComponentMobility::Stationary;
		NewPatch->CollisionTriangleSize = CollisionTriangleSize;
		NewPatch->CollisionResolution = CollisionResolution;
		NewPatch->UpdateCellInterval = UpdateCellInterval;
		NewPatch->MaxCollisionDistance = MaxCollisionDistance;
		NewPatch->bUseComplexAsSimpleCollision = bUseComplexAsSimpleCollision;
		NewPatch->bUseAsyncCooking = bUseAsyncCooking;
		NewPatch->bShowCollisionMesh = bShowCollisionMesh;
		NewPatch->DebugColor = DebugColor;
		NewPatch->StandbyDebugColor = StandbyDebugColor;
		NewPatch->DebugLineWidth = DebugLineWidth;
		NewPatch->RegisterComponent();

		USceneComponent* AttachParent = OwnerActor->GetRootComponent();
		if (AttachParent)
		{
			NewPatch->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepRelativeTransform);
		}

		NewPatch->GenerateCollisionMesh(PlanetRadius);
	}

	return NewPatch;
}

void UCosmicPlanetCollisionManager::RecyclePatch(UCosmicCollisionComponent* Patch)
{
	if (!Patch) return;

	Patch->ClearCollision();
	PatchPool.Add(Patch);
}

bool UCosmicPlanetCollisionManager::UpdateCollisions(
	const FVector& PlanetCenter,
	double PlanetRadius,
	TSharedPtr<ICosmicNoiseStrategy> NoiseStrategy)
{
	if (PlanetRadius <= 0.0 || !NoiseStrategy.IsValid())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World) return false;

	bool bAnyCollisionUpdated = false;

	// Iterate through all explicitly subscribed targets
	for (int32 i = SubscribedTargets.Num() - 1; i >= 0; --i)
	{
		AActor* TargetActor = SubscribedTargets[i].Get();
		if (!IsValid(TargetActor) || TargetActor->IsPendingKillPending())
		{
			// Clean up any patch if the actor was destroyed or is invalid
			for (int32 PatchIdx = ActiveTrackedPatches.Num() - 1; PatchIdx >= 0; --PatchIdx)
			{
				if (ActiveTrackedPatches[PatchIdx].TrackedActor.Get() == TargetActor || !ActiveTrackedPatches[PatchIdx].TrackedActor.IsValid())
				{
					RecyclePatch(ActiveTrackedPatches[PatchIdx].CollisionPatch);
					ActiveTrackedPatches.RemoveAt(PatchIdx);
				}
			}
			SubscribedTargets.RemoveAt(i);
			continue;
		}

		const FVector ActorLoc = TargetActor->GetActorLocation();
		const FVector CenterToActor = ActorLoc - PlanetCenter;
		const double DistToCenter = CenterToActor.Length();
		if (DistToCenter <= KINDA_SMALL_NUMBER) continue;

		const FVector SurfaceNormal = CenterToActor / DistToCenter;

		// Evaluate noise strategy to obtain procedural terrain surface height
		float SurfaceHeight = 0.0f;
		FLinearColor DummyColor;
		NoiseStrategy->EvaluatePoint(SurfaceNormal, SurfaceHeight, DummyColor);

		const double SurfaceRadius = PlanetRadius + SurfaceHeight;
		const double DistToSurface = FMath::Abs(DistToCenter - SurfaceRadius);

		// Find if this target already has an active patch
		FCosmicTrackedActorPatch* ExistingEntry = nullptr;
		for (FCosmicTrackedActorPatch& Entry : ActiveTrackedPatches)
		{
			if (Entry.TrackedActor.Get() == TargetActor)
			{
				ExistingEntry = &Entry;
				break;
			}
		}

		if (DistToSurface <= MaxCollisionDistance)
		{
			// Target is near surface: dual collision system active
			const FVector SurfacePos = PlanetCenter + SurfaceNormal * PlanetRadius;

			if (ExistingEntry)
			{
				if (ExistingEntry->CollisionPatch)
				{
					const float Threshold = ExistingEntry->CollisionPatch->GetUpdateDistanceThreshold();
					if (!ExistingEntry->CollisionPatch->IsBuilt() ||
						!ExistingEntry->LastActorLocation.Equals(ActorLoc, Threshold))
					{
						ExistingEntry->CollisionPatch->RequestCollisionUpdate(
							SurfacePos,
							SurfaceNormal,
							PlanetRadius,
							NoiseStrategy,
							PlanetCenter
						);
						ExistingEntry->LastActorLocation = ActorLoc;
						bAnyCollisionUpdated = true;
					}
				}
			}
			else
			{
				UCosmicCollisionComponent* NewPatch = AcquirePatchFromPool(PlanetRadius);
				if (NewPatch)
				{
					NewPatch->RequestCollisionUpdate(
						SurfacePos,
						SurfaceNormal,
						PlanetRadius,
						NoiseStrategy,
						PlanetCenter
					);

					FCosmicTrackedActorPatch NewEntry;
					NewEntry.TrackedActor = TargetActor;
					NewEntry.CollisionPatch = NewPatch;
					NewEntry.LastActorLocation = ActorLoc;
					ActiveTrackedPatches.Add(NewEntry);
					bAnyCollisionUpdated = true;
				}
			}
		}
		else
		{
			// Target is far from surface: only remove body when moving away past surface distance
			if (ExistingEntry)
			{
				RecyclePatch(ExistingEntry->CollisionPatch);
				ActiveTrackedPatches.RemoveAll([TargetActor](const FCosmicTrackedActorPatch& P) {
					return P.TrackedActor.Get() == TargetActor;
				});
			}
		}
	}

	// Clean up any tracked patches whose actor was unsubscribed or destroyed
	for (int32 PatchIdx = ActiveTrackedPatches.Num() - 1; PatchIdx >= 0; --PatchIdx)
	{
		AActor* Tracked = ActiveTrackedPatches[PatchIdx].TrackedActor.Get();
		if (!IsValid(Tracked) || !SubscribedTargets.Contains(Tracked))
		{
			RecyclePatch(ActiveTrackedPatches[PatchIdx].CollisionPatch);
			ActiveTrackedPatches.RemoveAt(PatchIdx);
		}
	}

	return bAnyCollisionUpdated;
}
