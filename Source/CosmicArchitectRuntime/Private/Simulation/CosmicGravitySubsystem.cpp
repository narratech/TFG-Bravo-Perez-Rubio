// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/CosmicGravitySubsystem.h"
#include "Simulation/CosmicGravityComponent.h"

// E: Constante gravitacional
// I: Gravitational constant
static const double G = 0.00000000006674;

// E: Constante gravitacional
// I: Gravitational constant
static const double GUnreal = G * 10000;


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

    // E: Primero, acumulamos las fuerzas según el modo de gravedad de cada cuerpo
    // I: First, we accumulate forces according to the gravity mode of each body
    for (int32 i = 0; i < Count; ++i) {
        UCosmicGravityComponent* BodyA = Bodies[i];
        if (!BodyA) continue;

        FVector PosA = BodyA->getTransform().GetLocation();

        // E: Switch para manejar los diferentes modos de gravedad
        // I: Switch to handle the different gravity modes
        switch (BodyA->GravityMode)
        {
        case ECosmicGravityMode::None:
            // E: No hace nada, sin gravedad
            // I: Does nothing, without gravity
            break;

        case ECosmicGravityMode::NearestPlanet:
        {
            // E: Usar la lista de planetas para mejor rendimiento
            // I: Use the list of planets for better performance
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
            if (!BodyA->IsAffectedByOthers || !BodyA->SpecificGravitySource) break;

            // E: Buscar el planeta específico en la lista de planetas
            // I: Search for the specific planet in the list of planets
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
            if (!BodyA->IsAffectedByOthers) break;

            // E: Usar la lista de planetas para mejor rendimiento
            // I: Use the list of planets for better performance
            for (UCosmicGravityComponent* Planet : Planets) {
                BodyAddForce(BodyA, Planet);
            }
            break;
        }

        case ECosmicGravityMode::NBody:
        {
            // E: Modo N-Body: todos los cuerpos se afectan entre sí
            // I: N-Body mode: all bodies affect each other
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

    // E: Integramos todos los cuerpos activos
    // I: We integrate all active bodies
    for (UCosmicGravityComponent* Body : Bodies)
    {
        if (Body)
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

    // E: Limitamos distancia si atraviesas el planeta para que no aplique mas fuerza de la que debe
    // I: We limit distance if you pass through the planet so it doesn't apply more force than it should
    if (BodyA->IsPlanet) {
        DistRadius = FMath::Square(BodyA->RadiusKm * 100000);
    }
    else if (BodyB->IsPlanet) {
        DistRadius = FMath::Square(BodyB->RadiusKm * 100000);
    }

    DistSq_cm = FMath::Max(DistRadius, DistSq_cm);

    // E: La GUnreal esta multiplicada por 10000 para contrarestar la distancia en cm
    // I: GUnreal is multiplied by 10000 to counteract the distance in cm
    double ForceMagnitude = (GUnreal * BodyA->Mass * BodyB->Mass) / DistSq_cm;

    BodyA->AccumulatedForce += Difference.GetSafeNormal() * ForceMagnitude;
}

void UCosmicGravitySubsystem::ApplyMutualForce(UCosmicGravityComponent* BodyA, UCosmicGravityComponent* BodyB)
{
    if (!BodyB) return;
    if (!BodyB->IsAffectedByOthers && !BodyA->IsAffectedByOthers) return;
    if (!BodyB->AffectsOthers && !BodyA->AffectsOthers) return;

    FVector Difference = BodyB->getTransform().GetLocation() - BodyA->getTransform().GetLocation();

    // E: La GUnreal esta multiplicada por 10000 para contrarestar la distancia en cm
    // I: GUnreal is multiplied by 10000 to counteract the distance in cm
    double DistSq_cm = Difference.SizeSquared();

    double DistRadius = 0.0;

    // E: Limitamos distancia si atraviesas el planeta para que no aplique mas fuerza de la que debe
    // I: We limit distance if you pass through the planet so it doesn't apply more force than it should
    if (BodyA->IsPlanet) {
        DistRadius = FMath::Square(BodyA->RadiusKm * 100000);
    }
    else if (BodyB->IsPlanet) {
        DistRadius = FMath::Square(BodyB->RadiusKm * 100000);
    }

    DistSq_cm = FMath::Max(DistRadius, DistSq_cm);

    double ForceMagnitude = (GUnreal * BodyA->Mass * BodyB->Mass) / DistSq_cm;

    FVector Force = Difference.GetSafeNormal() * ForceMagnitude;

    if (BodyA->IsAffectedByOthers && BodyB->AffectsOthers)
        BodyA->AccumulatedForce += Force;
    if (BodyB->IsAffectedByOthers && BodyA->AffectsOthers)
        BodyB->AccumulatedForce -= Force;
}

void UCosmicGravitySubsystem::RegisterBody(UCosmicGravityComponent* Body)
{
    if (!Body) return;

    // E: Añadimos solo si no está ya (evita duplicados)
    // I: Add only if it isn't already there (prevents duplicates)
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
    return G;
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