// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/GravitySubsystem.h"
#include "Simulation/GravityComponent.h"

static const double G = 10000.0; //Constante gravitacional adaptada

// Factor de suavizado para evitar que la fuerza sea infinita si dos cuerpos se tocan
static const double Softening = 100000.0;

void UGravitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

}

void UGravitySubsystem::Deinitialize()
{
    Bodies.Empty();
    Super::Deinitialize();
}

void UGravitySubsystem::Tick(float DeltaTime)
{
    UE_LOG(LogTemp, Warning, TEXT("Actualizando"));

    const int32 Count = Bodies.Num();
    if (Count == 0) return;

    // Primero, acumulamos las fuerzas según el modo de gravedad de cada cuerpo
    for (int32 i = 0; i < Count; ++i) {
        UGravityComponent* BodyA = Bodies[i];
        if (!BodyA || !BodyA->IsActive()) continue;

        FVector PosA = BodyA->getTransform().GetLocation();
        double MassA = BodyA->Mass;

        // Switch para manejar los diferentes modos de gravedad
        switch (BodyA->GravityMode)
        {
        case EGravityMode::None:
            // No hace nada, sin gravedad
            break;

        case EGravityMode::NearestPlanet:
        {
            // Encontrar el planeta más cercano que sea planeta y afecte a otros
            UGravityComponent* NearestPlanet = nullptr;
            float NearestDistanceSq = FLT_MAX;

            for (int32 j = 0; j < Count; ++j) {
                if (i == j) continue;

                UGravityComponent* BodyB = Bodies[j];
                if (!BodyB || !BodyB->IsActive() || !BodyB->IsPlanet || !BodyB->AffectsOthers) continue;

                float DistSq = FVector::DistSquared(PosA, BodyB->getTransform().GetLocation());
                if (DistSq < NearestDistanceSq) {
                    NearestDistanceSq = DistSq;
                    NearestPlanet = BodyB;
                }
            }

            // Aplicar fuerza del planeta más cercano
            if (NearestPlanet && BodyA->IsAffectedByOthers) {
                FVector Difference = NearestPlanet->getTransform().GetLocation() - PosA;
                double DistSq = Difference.SizeSquared();
                double DistanceFactor = DistSq + Softening;
                FVector Direction = Difference.GetSafeNormal();
                double ForceMagnitude = (G * MassA * NearestPlanet->Mass) / DistanceFactor;
                BodyA->AccumulatedForce += Direction * ForceMagnitude;
            }
            break;
        }

        case EGravityMode::SpecificPlanet:
        {
            // Usar un planeta específico como fuente de gravedad
            if (BodyA->IsAffectedByOthers && BodyA->SpecificGravitySource) {
                // Buscar el componente de gravedad del planeta específico
                UGravityComponent* SpecificPlanetComp = nullptr;
                for (UGravityComponent* BodyB : Bodies) {
                    if (BodyB && BodyB->GetOwner() == BodyA->SpecificGravitySource) {
                        SpecificPlanetComp = BodyB;
                        break;
                    }
                }

                if (SpecificPlanetComp && SpecificPlanetComp->AffectsOthers) {
                    FVector Difference = SpecificPlanetComp->getTransform().GetLocation() - PosA;
                    double DistSq = Difference.SizeSquared();
                    double DistanceFactor = DistSq + Softening;
                    FVector Direction = Difference.GetSafeNormal();
                    double ForceMagnitude = (G * MassA * SpecificPlanetComp->Mass) / DistanceFactor;
                    BodyA->AccumulatedForce += Direction * ForceMagnitude;
                }
            }
            break;
        }

        case EGravityMode::AllPlanets:
        {
            // Acumular fuerzas de todos los planetas
            if (!BodyA->IsAffectedByOthers) break;

            for (int32 j = 0; j < Count; ++j) {
                if (i == j) continue;

                UGravityComponent* BodyB = Bodies[j];
                if (!BodyB || !BodyB->IsActive() || !BodyB->IsPlanet || !BodyB->AffectsOthers) continue;

                FVector Difference = BodyB->getTransform().GetLocation() - PosA;
                double DistSq = Difference.SizeSquared();
                double DistanceFactor = DistSq + Softening;
                FVector Direction = Difference.GetSafeNormal();
                double ForceMagnitude = (G * MassA * BodyB->Mass) / DistanceFactor;
                BodyA->AccumulatedForce += Direction * ForceMagnitude;
            }
            break;
        }

        case EGravityMode::NBody:
        {
            // Modo N-Body: todos los cuerpos se afectan entre sí (código original)
            if (!BodyA->IsAffectedByOthers) break;

            for (int32 j = i + 1; j < Count; ++j) {
                UGravityComponent* BodyB = Bodies[j];
                if (!BodyB || !BodyB->IsActive()) continue;

                // Verificar si BodyB afecta a otros y si BodyA es afectado por otros
                if (!BodyB->AffectsOthers || !BodyA->IsAffectedByOthers) continue;

                FVector Difference = BodyB->getTransform().GetLocation() - PosA;
                double DistSq = Difference.SizeSquared();
                double DistanceFactor = DistSq + Softening;
                FVector Direction = Difference.GetSafeNormal();
                double ForceMagnitude = (G * MassA * BodyB->Mass) / DistanceFactor;
                FVector ForceVector = Direction * ForceMagnitude;

                if (BodyB->AffectsOthers && BodyA->IsAffectedByOthers)
                    BodyA->AccumulatedForce += ForceVector;

                if (BodyA->AffectsOthers && BodyB->IsAffectedByOthers)
                    BodyB->AccumulatedForce -= ForceVector;
            }
            break;
        }

        case EGravityMode::Hybrid:
        {
            // Modo híbrido: combinación de N-Body con planetas especiales
            // Primero, aplicar N-Body normal
            if (BodyA->IsAffectedByOthers) {
                for (int32 j = 0; j < Count; ++j) {
                    if (i == j) continue;

                    UGravityComponent* BodyB = Bodies[j];
                    if (!BodyB || !BodyB->IsActive() || !BodyB->AffectsOthers) continue;

                    FVector Difference = BodyB->getTransform().GetLocation() - PosA;
                    double DistSq = Difference.SizeSquared();
                    double DistanceFactor = DistSq + Softening;
                    FVector Direction = Difference.GetSafeNormal();
                    double ForceMagnitude = (G * MassA * BodyB->Mass) / DistanceFactor;
                    BodyA->AccumulatedForce += Direction * ForceMagnitude;
                }
            }

            // Luego, aplicar fuerza adicional si es un planeta específico (efecto de superficie)
            if (BodyA->IsPlanet && BodyA->SpecificGravitySource) {
                // Aquí podrías añadir lógica especial para planetas en modo híbrido
                // Por ejemplo, mantenerlos en órbita alrededor de algo
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

void UGravitySubsystem::RegisterBody(UGravityComponent* Body)
{
    if (Body)
    {
        UE_LOG(LogTemp, Warning, TEXT("Anadiendo cuerpo %d"), Bodies.Num());
        // Añadimos solo si no está ya (evita duplicados)
        Bodies.AddUnique(Body);
    }
}

void UGravitySubsystem::UnregisterBody(UGravityComponent* Body)
{
    Bodies.Remove(Body);

    UE_LOG(LogTemp, Warning, TEXT("Eliminando cuerpo %d"), Bodies.Num());
}

UWorld* UGravitySubsystem::GetTickableGameObjectWorld() const
{
    return GetWorld();
}

bool UGravitySubsystem::IsTickable() const
{
    // 1. Si soy una "plantilla" (CDO), no tickear.
    if (IsTemplate()) return false;

    // 2. Si no tengo mundo, adiós.
    if (!GetWorld()) return false;

    // 3. (OPCIONAL PERO RECOMENDADO) 
    // Solo tickear si hay cuerpos registrados. 
    // Esto optimiza el juego cuando no hay planetas cerca.
    // Si quieres ver logs de debug aunque esté vacío, comenta esta línea.
    return Bodies.Num() > 0;
}