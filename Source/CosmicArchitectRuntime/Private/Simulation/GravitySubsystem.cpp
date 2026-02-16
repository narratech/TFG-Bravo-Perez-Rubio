// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/GravitySubsystem.h"
#include "Simulation/GravityComponent.h"

static const double G = 0.00000000006674; //Constante gravitacional adaptada

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
   // UE_LOG(LogTemp, Warning, TEXT("Actualizando"));

    double UpdateMeshStartTime = FPlatformTime::Seconds();

    const int32 Count = Bodies.Num();
    if (Count == 0) return;

    // Primero, acumulamos las fuerzas según el modo de gravedad de cada cuerpo
    for (int32 i = 0; i < Count; ++i) {
        UGravityComponent* BodyA = Bodies[i];
        if (!BodyA) continue;

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
            // Usar la lista de planetas para mejor rendimiento
            if (!BodyA->IsAffectedByOthers) break;

            UGravityComponent* NearestPlanet = nullptr;
            float NearestDistanceSq = FLT_MAX;

            for (UGravityComponent* Planet : Planets) {
                if (!Planet || !Planet->AffectsOthers) continue;

                float DistSq = FVector::DistSquared(PosA, Planet->getTransform().GetLocation());
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
            if (!BodyA->IsAffectedByOthers) break;

            for (int32 j = 0; j < Count; ++j) {
                UGravityComponent* BodyB = Bodies[j];
                BodyAddForce(BodyA, BodyB);
            }
            break;
        }
        default:
            break;
        }
    }

    double UpdateMeshEndTime = FPlatformTime::Seconds();
    double UpdateMeshTime = UpdateMeshEndTime - UpdateMeshStartTime;

    UE_LOG(LogTemp, Warning, TEXT("Calculo gravitatorio hecho en %.4f ms"), UpdateMeshTime * 1000.0);

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

    // Convertimos a metros²
    double DistSq_m = Difference.SizeSquared() / 10000.0;

    double ForceMagnitude = (G * BodyA->Mass * BodyB->Mass) / DistSq_m;

    BodyA->AccumulatedForce += Difference.GetSafeNormal() * ForceMagnitude;
}

void UGravitySubsystem::RegisterBody(UGravityComponent* Body)
{
    if (!Body) return;
    
    UE_LOG(LogTemp, Warning, TEXT("Anadiendo cuerpo %d"), Bodies.Num());
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

    UE_LOG(LogTemp, Warning, TEXT("Eliminando cuerpo %d"), Bodies.Num());
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