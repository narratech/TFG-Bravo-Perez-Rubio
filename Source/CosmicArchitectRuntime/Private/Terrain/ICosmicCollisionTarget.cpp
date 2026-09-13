// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#include "Terrain/ICosmicCollisionTarget.h"
#include "Planet/CosmicPlanet.h"
#include "Terrain/CosmicPlanetCollisionManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"

void ICosmicCollisionTarget::FindCosmicPlanets(const UWorld* World, TArray<ACosmicPlanet*>& OutPlanets)
{
	OutPlanets.Reset();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ACosmicPlanet> It(World); It; ++It)
	{
		ACosmicPlanet* Planet = *It;
		if (IsValid(Planet))
		{
			OutPlanets.Add(Planet);
		}
	}
}

ACosmicPlanet* ICosmicCollisionTarget::FindNearestPlanet(const AActor* TargetActor, const TArray<ACosmicPlanet*>& Planets)
{
	if (!TargetActor || Planets.Num() == 0)
	{
		return nullptr;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();
	ACosmicPlanet* NearestPlanet = nullptr;
	double NearestDistanceSq = MAX_dbl;

	for (ACosmicPlanet* Planet : Planets)
	{
		if (!IsValid(Planet))
		{
			continue;
		}

		const double DistSq = FVector::DistSquared(TargetLocation, Planet->GetActorLocation());
		if (DistSq < NearestDistanceSq)
		{
			NearestDistanceSq = DistSq;
			NearestPlanet = Planet;
		}
	}

	return NearestPlanet;
}

ACosmicPlanet* ICosmicCollisionTarget::RegisterAndSubscribeToNearestPlanet(
	AActor* TargetActor,
	TArray<TWeakObjectPtr<ACosmicPlanet>>* OutRegisteredPlanets)
{
	if (!TargetActor)
	{
		return nullptr;
	}

	UWorld* World = TargetActor->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<ACosmicPlanet*> FoundPlanets;
	FindCosmicPlanets(World, FoundPlanets);

	if (OutRegisteredPlanets)
	{
		OutRegisteredPlanets->Reset();
		OutRegisteredPlanets->Reserve(FoundPlanets.Num());
		for (ACosmicPlanet* P : FoundPlanets)
		{
			OutRegisteredPlanets->Add(P);
		}
	}

	ACosmicPlanet* NearestPlanet = FindNearestPlanet(TargetActor, FoundPlanets);
	if (NearestPlanet && NearestPlanet->CollisionManager)
	{
		NearestPlanet->CollisionManager->RegisterCollisionTarget(TargetActor);
	}

	return NearestPlanet;
}

void ICosmicCollisionTarget::UnsubscribeFromPlanet(AActor* TargetActor, ACosmicPlanet* Planet)
{
	if (!TargetActor || !Planet || !Planet->CollisionManager)
	{
		return;
	}

	Planet->CollisionManager->UnregisterCollisionTarget(TargetActor);
}
