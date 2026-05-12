// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/CosmicGravitySubsystem.h"
#include "Simulation/CosmicGravityComponent.h"

// Constante gravitacional universal en unidades SI: 6.674e-11 m3 / (kg * s2).
// Se usa para calcular masas planetarias en BeginPlay y como base de GUnreal.
static const double G = 0.00000000006674;

// Version adaptada de G para el sistema de unidades de Unreal Engine (centimetros).
// Se multiplica por 10000 (100^2) para compensar que las distancias en Unreal estan en cm
// mientras que la formula de Newton opera en metros: F = G*M*m / r^2, con r en metros.
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

    // Fase 1: acumulacion de fuerzas.
    // Cada cuerpo calcula las fuerzas que recibe segun su modo y las suma en AccumulatedForce.
    // La integracion se realiza en una segunda pasada para que todas las fuerzas del frame
    // esten acumuladas antes de mover ningun cuerpo (evita dependencias de orden).
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
            // Busqueda del planeta mas cercano iterando unicamente sobre Planets (sublista optimizada).
            // Se compara distancia al cuadrado para evitar raices cuadradas en el bucle.
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
            // Se busca el componente del actor referenciado en SpecificGravitySource
            // dentro de Planets para garantizar que el actor destino es realmente un planeta registrado.
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
            // Se acumula la fuerza de cada planeta por separado.
            // BodyAddForce filtra internamente planetas nulos o con AffectsOthers == false.
            if (!BodyA->IsAffectedByOthers) break;

            for (UCosmicGravityComponent* Planet : Planets) {
                BodyAddForce(BodyA, Planet);
            }
            break;
        }

        case ECosmicGravityMode::NBody:
        {
            // Simulacion N-cuerpos: cada par (i, j) se procesa una sola vez (j > i)
            // aplicando fuerzas mutuas en ApplyMutualForce, que implementa la Tercera Ley de Newton.
            // Esto reduce la complejidad de O(n^2) llamadas a O(n*(n-1)/2).
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

    // Fase 2: integracion.
    // Se mueven unicamente los cuerpos con modo activo e IsAffectedByOthers == true.
    // Los planetas estaticos (IsAffectedByOthers == false) no integran aunque tengan fuerzas acumuladas.
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

    // Clampeo de distancia minima al radio del planeta en cm (RadiusKm * 1000 m/km * 100 cm/m).
    // Evita que la fuerza diverja a infinito cuando un cuerpo atraviesa o se solapa con el planeta,
    // simulando que dentro de la superficie la fuerza gravitacional no sigue creciendo.
    if (BodyA->IsPlanet) {
        DistRadius = FMath::Square(BodyA->RadiusKm * 100000);
    }
    else if (BodyB->IsPlanet) {
        DistRadius = FMath::Square(BodyB->RadiusKm * 100000);
    }

    DistSq_cm = FMath::Max(DistRadius, DistSq_cm);

    // Formula de Newton adaptada a centimetros: F = GUnreal * M * m / r^2
    // GUnreal absorbe el factor de conversion de m^2 a cm^2 (x10000), manteniendo F en Newtons.
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

    // Mismo clampeo que en BodyAddForce: la distancia minima se fija al radio del planeta
    // para evitar singularidades cuando los cuerpos se superponen geometricamente.
    if (BodyA->IsPlanet) {
        DistRadius = FMath::Square(BodyA->RadiusKm * 100000);
    }
    else if (BodyB->IsPlanet) {
        DistRadius = FMath::Square(BodyB->RadiusKm * 100000);
    }

    DistSq_cm = FMath::Max(DistRadius, DistSq_cm);

    double ForceMagnitude = (GUnreal * BodyA->Mass * BodyB->Mass) / DistSq_cm;

    FVector Force = Difference.GetSafeNormal() * ForceMagnitude;

    // Aplicacion simetrica de fuerzas: BodyA recibe Force y BodyB recibe -Force.
    // Se respetan los flags individuales: un cuerpo puede generar gravedad sin recibirla y viceversa.
    if (BodyA->IsAffectedByOthers && BodyB->AffectsOthers)
        BodyA->AccumulatedForce += Force;
    if (BodyB->IsAffectedByOthers && BodyA->AffectsOthers)
        BodyB->AccumulatedForce -= Force;
}

void UCosmicGravitySubsystem::RegisterBody(UCosmicGravityComponent* Body)
{
    if (!Body) return;

    // AddUnique evita duplicados en caso de llamadas redundantes desde SetIsPlanet o BeginPlay.
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