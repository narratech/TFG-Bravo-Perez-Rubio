// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Simulation/CosmicGravitySubsystem.h"
#include "Simulation/CosmicGravityComponent.h"

// Universal gravitational constant in SI units: 6.674e-11 m³ / (kg * s²).
// Used to calculate planetary masses in BeginPlay and as base for GUnreal.
static const double GravityConstant = 0.00000000006674;

// Adapted version of G for Unreal Engine unit system (centimeters).
// Multiplied by 10000 (100²) to compensate for Unreal distances being in cm
// while Newton's formula operates in meters: F = G*M*m / r², with r in meters.
static const double GUnreal = GravityConstant * 10000;


void UCosmicGravitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}
 
void UCosmicGravitySubsystem::Deinitialize()
{
    Bodies.Empty();
    Planets.Empty();
    Super::Deinitialize();
}

void UCosmicGravitySubsystem::Tick(float DeltaTime)
{

    const int32 Count = Bodies.Num();
    if (Count == 0) return;

    // Phase 1: force accumulation.
    // Each body calculates forces it receives based on mode and sums them into AccumulatedForce.
    // Integration is performed in a second pass so all frame forces
    // are accumulated before moving any body (avoids order dependencies).
    for (int32 i = 0; i < Count; ++i) {
        UCosmicGravityComponent* BodyA = Bodies[i];
        if (!BodyA) continue;

        FVector PosA = BodyA->getTransform().GetLocation();

        switch (BodyA->GravityMode)
        {
        case ECosmicGravityMode::None:
            break;

        case ECosmicGravityMode::NearestPlanet:
        {
            // Nearest planet search iterating only over Planets (optimized sublist).
            // Compares squared distance to avoid square roots in loop.
            if (!BodyA->IsAffectedByOthers) break;

            UCosmicGravityComponent* NearestPlanet = nullptr;
            double NearestDistanceSq = DBL_MAX;

            for (UCosmicGravityComponent* Planet : Planets) {
                if (!Planet || !Planet->AffectsOthers) continue;

                double DistSq = FVector::DistSquared(PosA, Planet->getTransform().GetLocation());
                if (DistSq < NearestDistanceSq) {
                    NearestDistanceSq = DistSq;
                    NearestPlanet = Planet;
                }
            }
            BodyAddForce(BodyA, NearestPlanet);

            break;
        }

        case ECosmicGravityMode::SpecificPlanet:
        {
            // Look up component of referenced actor in SpecificGravitySource
            // within Planets to guarantee target actor is indeed a registered planet.
            if (!BodyA->IsAffectedByOthers || !BodyA->SpecificGravitySource) break;

            UCosmicGravityComponent* SpecificPlanetComp = nullptr;
            for (UCosmicGravityComponent* Planet : Planets) {
                if (Planet && Planet->GetOwner() == BodyA->SpecificGravitySource) {
                    SpecificPlanetComp = Planet;
                    break;
                }
            }

            BodyAddForce(BodyA, SpecificPlanetComp);

            break;
        }

        case ECosmicGravityMode::AllPlanets:
        {
            // Accumulate force from each planet separately.
            // BodyAddForce internally filters null planets or those with AffectsOthers == false.
            if (!BodyA->IsAffectedByOthers) break;

            for (UCosmicGravityComponent* Planet : Planets) {
                BodyAddForce(BodyA, Planet);
            }
            break;
        }

        case ECosmicGravityMode::NBody:
        {
            // N-body simulation: each pair (i, j) is processed once (j > i)
            // applying mutual forces in ApplyMutualForce, implementing Newton's Third Law.
            // This reduces complexity from O(n²) calls to O(n*(n-1)/2).
            for (int32 j = i + 1; j < Count; ++j) {
                UCosmicGravityComponent* BodyB = Bodies[j];
                ApplyMutualForce(BodyA, BodyB);
            }
            break;
        }
        default:
            break;
        }
    }

    // Phase 2: integration.
    // Only bodies with active mode and IsAffectedByOthers == true are moved.
    // Static planets (IsAffectedByOthers == false) do not integrate even if having accumulated forces.
    for (UCosmicGravityComponent* Body : Bodies)
    {
        if (Body && Body->GravityMode != ECosmicGravityMode::None && Body->IsAffectedByOthers)
        {
            Body->Integrate(DeltaTime);
        }
    }
}

void UCosmicGravitySubsystem::BodyAddForce(UCosmicGravityComponent* BodyA, UCosmicGravityComponent* BodyB)
{
    if (!BodyB || !BodyB->AffectsOthers || BodyA == BodyB) return;

    FVector Difference = BodyB->getTransform().GetLocation() - BodyA->getTransform().GetLocation();

    double DistSq_cm = Difference.SizeSquared();

    double DistRadius = 0.0;

    // Clamp minimum distance to planet radius in cm (RadiusKm * 1000 m/km * 100 cm/m).
    // Prevents force diverging to infinity when a body passes through or overlaps with planet,
    // simulating that inside surface gravitational force does not keep growing.
    if (BodyA->IsPlanet) {
        DistRadius = FMath::Square(BodyA->RadiusKm * 100000);
    }
    else if (BodyB->IsPlanet) {
        DistRadius = FMath::Square(BodyB->RadiusKm * 100000);
    }

    DistSq_cm = FMath::Max(DistRadius, DistSq_cm);

    // Newton formula adapted to centimeters: F = GUnreal * M * m / r²
    // GUnreal absorbs conversion factor from m² to cm² (x10000), keeping F in Newtons.
    double ForceMagnitude = (GUnreal * BodyA->Mass * BodyB->Mass) / DistSq_cm;

    BodyA->AccumulatedForce += Difference.GetSafeNormal() * ForceMagnitude;
}

void UCosmicGravitySubsystem::ApplyMutualForce(UCosmicGravityComponent* BodyA, UCosmicGravityComponent* BodyB)
{
    if (!BodyB) return;
    if (!BodyB->IsAffectedByOthers && !BodyA->IsAffectedByOthers) return;
    if (!BodyB->AffectsOthers && !BodyA->AffectsOthers) return;

    FVector Difference = BodyB->getTransform().GetLocation() - BodyA->getTransform().GetLocation();

    double DistSq_cm = Difference.SizeSquared();

    double DistRadius = 0.0;

    // Same clamping as in BodyAddForce: minimum distance clamped to planet radius
    // to avoid singularities when bodies geometrically overlap.
    if (BodyA->IsPlanet) {
        DistRadius = FMath::Square(BodyA->RadiusKm * 100000);
    }
    else if (BodyB->IsPlanet) {
        DistRadius = FMath::Square(BodyB->RadiusKm * 100000);
    }

    DistSq_cm = FMath::Max(DistRadius, DistSq_cm);

    double ForceMagnitude = (GUnreal * BodyA->Mass * BodyB->Mass) / DistSq_cm;

    FVector Force = Difference.GetSafeNormal() * ForceMagnitude;

    // Symmetric force application: BodyA receives Force and BodyB receives -Force.
    // Individual flags respected: body can generate gravity without receiving it and vice versa.
    if (BodyA->IsAffectedByOthers && BodyB->AffectsOthers)
        BodyA->AccumulatedForce += Force;
    if (BodyB->IsAffectedByOthers && BodyA->AffectsOthers)
        BodyB->AccumulatedForce -= Force;
}

void UCosmicGravitySubsystem::RegisterBody(UCosmicGravityComponent* Body)
{
    if (!Body) return;

    // AddUnique avoids duplicates on redundant calls from SetIsPlanet or BeginPlay.
    Bodies.AddUnique(Body);

    if (Body->IsPlanet)
    {
        Planets.AddUnique(Body);
    }
}

void UCosmicGravitySubsystem::UnregisterBody(UCosmicGravityComponent* Body)
{
    if (!Body) return;

    Bodies.Remove(Body);

    if (Body->IsPlanet)
    {
        Planets.Remove(Body);
    }
}

double UCosmicGravitySubsystem::GetGravityConstant() const
{
    return GravityConstant;
}

UWorld* UCosmicGravitySubsystem::GetTickableGameObjectWorld() const
{
    return GetWorld();
}

bool UCosmicGravitySubsystem::IsTickable() const
{
    if (IsTemplate() || !GetWorld()) return false;

    return Bodies.Num() > 0;
}