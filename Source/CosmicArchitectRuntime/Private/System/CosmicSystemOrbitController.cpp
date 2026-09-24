// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "System/CosmicSystemOrbitController.h"
#include "GameFramework/Actor.h"
#include "Simulation/CosmicOrbitComponent.h"

void FCosmicSystemOrbitController::StartOrbitSimulation(const TArray<AActor*>& Bodies, float OrbitSpeedMultiplier)
{
    for (AActor* Actor : Bodies)
    {
        if (!Actor)
        {
            continue;
        }

        TArray<UCosmicOrbitComponent*> Orbits;
        Actor->GetComponents<UCosmicOrbitComponent>(Orbits);
        for (UCosmicOrbitComponent* Orbit : Orbits)
        {
            if (Orbit)
            {
                Orbit->EditorSpeedMultiplier = OrbitSpeedMultiplier;
                Orbit->bEditorSimulating = true;
            }
        }
    }
}

void FCosmicSystemOrbitController::StopOrbitSimulation(const TArray<AActor*>& Bodies)
{
    for (AActor* Actor : Bodies)
    {
        if (!Actor)
        {
            continue;
        }

        TArray<UCosmicOrbitComponent*> Orbits;
        Actor->GetComponents<UCosmicOrbitComponent>(Orbits);
        for (UCosmicOrbitComponent* Orbit : Orbits)
        {
            if (Orbit)
            {
                Orbit->bEditorSimulating = false;
            }
        }
    }
}

void FCosmicSystemOrbitController::UpdateBodiesOrbitalPeriod(const TArray<AActor*>& Bodies, float OrbitSpeedMultiplier)
{
    for (AActor* Actor : Bodies)
    {
        if (!Actor)
        {
            continue;
        }

        TArray<UCosmicOrbitComponent*> Orbits;
        Actor->GetComponents<UCosmicOrbitComponent>(Orbits);
        for (UCosmicOrbitComponent* Orbit : Orbits)
        {
            if (Orbit)
            {
                Orbit->EditorSpeedMultiplier = OrbitSpeedMultiplier;
            }
        }
    }
}

void FCosmicSystemOrbitController::TickSimulation(const TArray<AActor*>& Bodies, float OrbitSpeedMultiplier)
{
    for (AActor* Actor : Bodies)
    {
        if (!Actor)
        {
            continue;
        }

        TArray<UCosmicOrbitComponent*> Orbits;
        Actor->GetComponents<UCosmicOrbitComponent>(Orbits);
        for (UCosmicOrbitComponent* Orbit : Orbits)
        {
            if (Orbit)
            {
                Orbit->EditorSpeedMultiplier = OrbitSpeedMultiplier;
            }
        }
    }
}
