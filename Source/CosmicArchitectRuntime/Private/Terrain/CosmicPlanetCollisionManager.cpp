// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Terrain/CosmicPlanetCollisionManager.h"
#include "Terrain/CosmicCollisionComponent.h"
#include "Terrain/ICosmicCollisionTarget.h"
#include "ICosmicNoiseStrategy.h"
#include "Planet/CosmicPlanet.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

UCosmicPlanetCollisionManager::UCosmicPlanetCollisionManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
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
	if (TargetActor && !CustomRegisteredTargets.Contains(TargetActor))
	{
		CustomRegisteredTargets.Add(TargetActor);
	}
}

void UCosmicPlanetCollisionManager::UnregisterCollisionTarget(AActor* TargetActor)
{
	if (!TargetActor) return;

	CustomRegisteredTargets.Remove(TargetActor);

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

UCosmicCollisionComponent* UCosmicPlanetCollisionManager::AcquirePatchFromPool(ACosmicPlanet* Planet, double PlanetRadius)
{
	while (PatchPool.Num() > 0)
	{
		UCosmicCollisionComponent* PooledPatch = PatchPool.Pop();
		if (IsValid(PooledPatch))
		{
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

	// Instantiate new collision patch component
	UCosmicCollisionComponent* NewPatch = NewObject<UCosmicCollisionComponent>(Planet, NAME_None, RF_Transient);
	if (NewPatch)
	{
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

		USceneComponent* AttachParent = Planet->GetRootComponent();
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

void UCosmicPlanetCollisionManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACosmicPlanet* Planet = Cast<ACosmicPlanet>(GetOwner());
	if (!Planet) return;

	const double PlanetRadius = Planet->RadiusKm * 100000.0;
	if (PlanetRadius <= 0.0) return;

	const FVector PlanetCenter = Planet->GetActorLocation();
	TSharedPtr<ICosmicNoiseStrategy> NoiseStrategy = Planet->GetNoiseStrategy();
	if (!NoiseStrategy.IsValid()) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Determine local observer viewpoint
	const bool bIsServer = IsRunningDedicatedServer() || (World->GetNetMode() == NM_DedicatedServer) || (World->GetNetMode() == NM_ListenServer);
	bool bHasLocalViewer = false;
	FVector LocalViewerPos = FVector::ZeroVector;
	FVector LocalViewerForward = FVector::ForwardVector;

	if (!bIsServer)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		if (PC)
		{
			FRotator CamRot;
			PC->GetPlayerViewPoint(LocalViewerPos, CamRot);
			LocalViewerForward = CamRot.Vector();
			bHasLocalViewer = !LocalViewerPos.IsZero();

			if (!bHasLocalViewer && PC->GetPawn())
			{
				LocalViewerPos = PC->GetPawn()->GetActorLocation();
				LocalViewerForward = PC->GetPawn()->GetActorForwardVector();
				bHasLocalViewer = true;
			}
		}
	}

	// 1. Collect all candidate actors
	TArray<AActor*> Candidates;
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (IsValid(Pawn))
		{
			Candidates.Add(Pawn);
		}
	}

	for (int32 i = CustomRegisteredTargets.Num() - 1; i >= 0; --i)
	{
		if (CustomRegisteredTargets[i].IsValid())
		{
			Candidates.AddUnique(CustomRegisteredTargets[i].Get());
		}
		else
		{
			CustomRegisteredTargets.RemoveAt(i);
		}
	}

	// Structure to hold evaluated candidates
	struct FCandidateEvaluation
	{
		AActor* Actor = nullptr;
		float Score = 0.0f;
		FVector SurfacePos = FVector::ZeroVector;
		FVector SurfaceNormal = FVector::UpVector;
	};

	TArray<FCandidateEvaluation> EvaluatedCandidates;
	EvaluatedCandidates.Reserve(Candidates.Num());

	for (AActor* Candidate : Candidates)
	{
		if (!IsValid(Candidate)) continue;

		const FVector ActorLoc = Candidate->GetActorLocation();
		const FVector CenterToActor = ActorLoc - PlanetCenter;
		const double DistToCenter = CenterToActor.Length();
		if (DistToCenter <= KINDA_SMALL_NUMBER) continue;

		const FVector SurfaceNormal = CenterToActor / DistToCenter;

		float SurfaceHeight = 0.0f;
		FLinearColor DummyColor;
		NoiseStrategy->EvaluatePoint(SurfaceNormal, SurfaceHeight, DummyColor);

		const double SurfaceRadius = PlanetRadius + SurfaceHeight;
		const double DistToSurface = FMath::Abs(DistToCenter - SurfaceRadius);

		if (DistToSurface > MaxCollisionDistance)
		{
			continue;
		}

		const float RelevanceScore = CalculateActorRelevance(
			Candidate,
			LocalViewerPos,
			LocalViewerForward,
			bHasLocalViewer,
			DistToSurface
		);

		if (RelevanceScore >= 0.0f)
		{
			const FVector SurfacePos = PlanetCenter + SurfaceNormal * PlanetRadius;
			EvaluatedCandidates.Add({ Candidate, RelevanceScore, SurfacePos, SurfaceNormal });
		}
	}

	// Sort candidates by descending relevance score
	EvaluatedCandidates.Sort([](const FCandidateEvaluation& A, const FCandidateEvaluation& B)
	{
		return A.Score > B.Score;
	});

	// Budget limit
	if (EvaluatedCandidates.Num() > MaxConcurrentPatches)
	{
		EvaluatedCandidates.SetNum(MaxConcurrentPatches);
	}

	// 2. Reconcile currently active patches
	for (int32 i = ActiveTrackedPatches.Num() - 1; i >= 0; --i)
	{
		FCosmicTrackedActorPatch& Entry = ActiveTrackedPatches[i];
		AActor* Tracked = Entry.TrackedActor.Get();

		bool bKeep = false;
		if (IsValid(Tracked))
		{
			for (const FCandidateEvaluation& Eval : EvaluatedCandidates)
			{
				if (Eval.Actor == Tracked)
				{
					bKeep = true;
					break;
				}
			}
		}

		if (!bKeep)
		{
			RecyclePatch(Entry.CollisionPatch);
			ActiveTrackedPatches.RemoveAt(i);
		}
	}

	// 3. Assign or update patches for top candidates
	for (const FCandidateEvaluation& Eval : EvaluatedCandidates)
	{
		FCosmicTrackedActorPatch* ExistingEntry = nullptr;
		for (FCosmicTrackedActorPatch& Entry : ActiveTrackedPatches)
		{
			if (Entry.TrackedActor.Get() == Eval.Actor)
			{
				ExistingEntry = &Entry;
				break;
			}
		}

		const FVector ActorLoc = Eval.Actor->GetActorLocation();

		if (ExistingEntry)
		{
			ExistingEntry->RelevanceScore = Eval.Score;
			if (ExistingEntry->CollisionPatch)
			{
				if (!ExistingEntry->LastActorLocation.Equals(ActorLoc, ExistingEntry->CollisionPatch->GetUpdateDistanceThreshold()))
				{
					ExistingEntry->CollisionPatch->RequestCollisionUpdate(
						Eval.SurfacePos,
						Eval.SurfaceNormal,
						PlanetRadius,
						NoiseStrategy,
						PlanetCenter
					);
					ExistingEntry->LastActorLocation = ActorLoc;
				}
			}
		}
		else
		{
			UCosmicCollisionComponent* NewPatch = AcquirePatchFromPool(Planet, PlanetRadius);
			if (NewPatch)
			{
				NewPatch->RequestCollisionUpdate(
					Eval.SurfacePos,
					Eval.SurfaceNormal,
					PlanetRadius,
					NoiseStrategy,
					PlanetCenter
				);

				FCosmicTrackedActorPatch NewEntry;
				NewEntry.TrackedActor = Eval.Actor;
				NewEntry.CollisionPatch = NewPatch;
				NewEntry.LastActorLocation = ActorLoc;
				NewEntry.RelevanceScore = Eval.Score;
				ActiveTrackedPatches.Add(NewEntry);
			}
		}
	}
}
