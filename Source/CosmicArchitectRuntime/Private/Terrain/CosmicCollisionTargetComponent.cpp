// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Terrain/CosmicCollisionTargetComponent.h"
#include "Planet/CosmicPlanet.h"
#include "Terrain/CosmicPlanetCollisionManager.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"

UCosmicCollisionTargetComponent::UCosmicCollisionTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCosmicCollisionTargetComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Discover planets and subscribe to the nearest planet's collision manager
	ACosmicPlanet* NearestPlanet = ICosmicCollisionTarget::RegisterAndSubscribeToNearestPlanet(Owner, &RegisteredPlanets);
	if (NearestPlanet)
	{
		CurrentPlanetCollisionManager = NearestPlanet->CollisionManager;
	}

	// Start periodic planet verification timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PlanetCheckTimerHandle,
			this,
			&UCosmicCollisionTargetComponent::UpdateNearestPlanetSubscription,
			FMath::Max(0.1f, PlanetCheckInterval),
			true
		);
	}
}

void UCosmicCollisionTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlanetCheckTimerHandle);
	}

	if (CurrentPlanetCollisionManager.IsValid())
	{
		if (AActor* Owner = GetOwner())
		{
			CurrentPlanetCollisionManager->UnregisterCollisionTarget(Owner);
		}
		CurrentPlanetCollisionManager = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

bool UCosmicCollisionTargetComponent::IsCollisionRelevant() const
{
	if (!bIsCollisionRelevant)
	{
		return false;
	}

	const AActor* Owner = GetOwner();
	if (!Owner || Owner->IsPendingKillPending())
	{
		return false;
	}

	return true;
}

float UCosmicCollisionTargetComponent::GetCollisionPriority() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return Priority;
	}

	if (bAutoElevatePlayerPriority)
	{
		if (const APawn* Pawn = Cast<APawn>(Owner))
		{
			if (Pawn->IsLocallyControlled())
			{
				return 1.0f;
			}
		}
	}

	return FMath::Clamp(Priority, 0.0f, 1.0f);
}

ACosmicPlanet* UCosmicCollisionTargetComponent::GetCurrentPlanet() const
{
	if (CurrentPlanetCollisionManager.IsValid())
	{
		return Cast<ACosmicPlanet>(CurrentPlanetCollisionManager->GetOwner());
	}
	return nullptr;
}

void UCosmicCollisionTargetComponent::UpdateNearestPlanetSubscription()
{
	AActor* Owner = GetOwner();
	if (!Owner || Owner->IsPendingKillPending())
	{
		return;
	}

	TArray<ACosmicPlanet*> ValidPlanets;
	for (const TWeakObjectPtr<ACosmicPlanet>& PlanetPtr : RegisteredPlanets)
	{
		if (PlanetPtr.IsValid())
		{
			ValidPlanets.Add(PlanetPtr.Get());
		}
	}

	if (ValidPlanets.Num() == 0)
	{
		ACosmicPlanet* Nearest = ICosmicCollisionTarget::RegisterAndSubscribeToNearestPlanet(Owner, &RegisteredPlanets);
		if (Nearest)
		{
			CurrentPlanetCollisionManager = Nearest->CollisionManager;
		}
		return;
	}

	ACosmicPlanet* NearestPlanet = ICosmicCollisionTarget::FindNearestPlanet(Owner, ValidPlanets);
	if (NearestPlanet && NearestPlanet->CollisionManager != CurrentPlanetCollisionManager.Get())
	{
		if (CurrentPlanetCollisionManager.IsValid())
		{
			CurrentPlanetCollisionManager->UnregisterCollisionTarget(Owner);
		}

		if (NearestPlanet->CollisionManager)
		{
			NearestPlanet->CollisionManager->RegisterCollisionTarget(Owner);
			CurrentPlanetCollisionManager = NearestPlanet->CollisionManager;
		}
		else
		{
			CurrentPlanetCollisionManager = nullptr;
		}
	}
}
