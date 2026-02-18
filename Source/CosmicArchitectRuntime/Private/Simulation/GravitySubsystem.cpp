// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/GravitySubsystem.h"
#include "Simulation/GravityComponent.h"

static const double G = 0.00000000006674; //Constante gravitacional 
static const double GUnreal = G * 10000; //Constante gravitacional 

// Factor de suavizado para evitar que la fuerza sea infinita si dos cuerpos se tocan
static const double Softening = 100000.0;

void UGravitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UGravitySubsystem::Deinitialize()
{
    Bodies.Empty();
    Planets.Empty();
    Super::Deinitialize();
}

void UGravitySubsystem::Tick(float DeltaTime)
{

    const int32 Count = Bodies.Num();
    if (Count == 0) return;

    // Primero, acumulamos las fuerzas según el modo de gravedad de cada cuerpo
    for (int32 i = 0; i < Count; ++i) {
        UGravityComponent* BodyA = Bodies[i];
        if (!BodyA) continue;

        FVector PosA = BodyA->getTransform().GetLocation();

        // Switch para manejar los diferentes modos de gravedad
        switch (BodyA->GravityMode)
        {
        case EGravityMode::None:
            // No hace nada, sin gravedad
            break;

        case EGravityMode::NearestPlanet:
        {
            // Usar la lista de planetas para mejor rendimiento
            if (!BodyA->IsAffectedByOthers) break;

            UGravityComponent* NearestPlanet = nullptr;
            double NearestDistanceSq = DBL_MAX;

            for (UGravityComponent* Planet : Planets) {
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

        case EGravityMode::SpecificPlanet:
        {
            if (!BodyA->IsAffectedByOthers || !BodyA->SpecificGravitySource) break;

            // Buscar el planeta específico en la lista de planetas
            UGravityComponent* SpecificPlanetComp = nullptr;
            for (UGravityComponent* Planet : Planets) {
                if (Planet && Planet->GetOwner() == BodyA->SpecificGravitySource) {
                    SpecificPlanetComp = Planet;
                    break;
                }
            }

            BodyAddForce(BodyA, SpecificPlanetComp);

            break;
        }

        case EGravityMode::AllPlanets:
        {
            if (!BodyA->IsAffectedByOthers) break;

            // Usar la lista de planetas para mejor rendimiento
            for (UGravityComponent* Planet : Planets) {
                BodyAddForce(BodyA, Planet);
            }
            break;
        }

        case EGravityMode::NBody:
        {
            // Modo N-Body: todos los cuerpos se afectan entre sí

            for (int32 j = i + 1; j < Count; ++j) {
                UGravityComponent* BodyB = Bodies[j];
                ApplyMutualForce(BodyA, BodyB);
            }
            break;
        }
        default:
            break;
        }
    }

    // Integramos todos los cuerpos activos
    for (UGravityComponent* Body : Bodies)
    {
        if (Body)
        {
            Body->Integrate(DeltaTime);
        }
    }
}

void UGravitySubsystem::BodyAddForce(UGravityComponent* BodyA, UGravityComponent* BodyB)
{
    if (!BodyB || !BodyB->AffectsOthers || BodyA == BodyB) return;

    FVector Difference = BodyB->getTransform().GetLocation() - BodyA->getTransform().GetLocation();

    double DistSq_cm = Difference.SizeSquared();

    double DistRadius = 0.0;

    //Limitamos distancia si atraviesas el planeta para que no aplique mas fuerza de la que debe
    if (BodyA->IsPlanet) {
        DistRadius = FMath::Square(BodyA->RadiusKm * 100000);
    }
    else if (BodyB->IsPlanet) {
        DistRadius = FMath::Square(BodyB->RadiusKm * 100000);
    }

    DistSq_cm = FMath::Max(DistRadius, DistSq_cm);

    //La GUnreal esta multiplicada por 10000 para contrarestar la distancia en cm
    double ForceMagnitude = (GUnreal * BodyA->Mass * BodyB->Mass) / DistSq_cm;

    BodyA->AccumulatedForce += Difference.GetSafeNormal() * ForceMagnitude;
}

void UGravitySubsystem::ApplyMutualForce(UGravityComponent* BodyA, UGravityComponent* BodyB)
{
    if (!BodyB) return;
    if (!BodyB->IsAffectedByOthers && !BodyA->IsAffectedByOthers) return;
    if (!BodyB->AffectsOthers && !BodyA->AffectsOthers) return;

    FVector Difference = BodyB->getTransform().GetLocation() - BodyA->getTransform().GetLocation();

    //La GUnreal esta multiplicada por 10000 para contrarestar la distancia en cm
    double DistSq_cm = Difference.SizeSquared();

    double DistRadius = 0.0;
    //Limitamos distancia si atraviesas el planeta para que no aplique mas fuerza de la que debe
    if (BodyA->IsPlanet) {
        DistRadius = FMath::Square(BodyA->RadiusKm * 100000);
    }
    else if (BodyB->IsPlanet) {
        DistRadius = FMath::Square(BodyB->RadiusKm * 100000);
    }

    DistSq_cm = FMath::Max(DistRadius, DistSq_cm);

    double ForceMagnitude = (GUnreal * BodyA->Mass * BodyB->Mass) / DistSq_cm;

    FVector Force = Difference.GetSafeNormal() * ForceMagnitude;

    if(BodyA->IsAffectedByOthers && BodyB->AffectsOthers)
        BodyA->AccumulatedForce += Force;
    if(BodyB->IsAffectedByOthers && BodyA->AffectsOthers)
        BodyB->AccumulatedForce -= Force;
}

void UGravitySubsystem::RegisterBody(UGravityComponent* Body)
{
    if (!Body) return;

    // Añadimos solo si no está ya (evita duplicados)
    Bodies.AddUnique(Body);

    if (Body->IsPlanet)
    {
        Planets.AddUnique(Body);
    }
}

void UGravitySubsystem::UnregisterBody(UGravityComponent* Body)
{
    if (!Body) return;

    Bodies.Remove(Body);

    if (Body->IsPlanet)
    {
        Planets.Remove(Body);
    }
}

double UGravitySubsystem::GetGravityConstant() const
{
    return G;
}

UWorld* UGravitySubsystem::GetTickableGameObjectWorld() const
{
    return GetWorld();
}



bool UGravitySubsystem::IsTickable() const
{
    if (IsTemplate() || !GetWorld()) return false;

    return Bodies.Num() > 0;
}