// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/NBodySimulationSubsystem.h"
#include "Simulation/GravityComponent.h"

static const double G = 1000.0; //Constante gravitacional adaptada

// Factor de suavizado para evitar que la fuerza sea infinita si dos cuerpos se tocan
static const double Softening = 100000.0;

void UNBodySimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

}

void UNBodySimulationSubsystem::Deinitialize()
{
    Bodies.Empty();
    Super::Deinitialize();
}

void UNBodySimulationSubsystem::Tick(float DeltaTime)
{

    const int32 Count = Bodies.Num();
    if (Count == 0)return;

    for (int32 i = 0; i < Count; ++i) {
        UGravityComponent* BodyA = Bodies[i];
        if (!BodyA || !BodyA->IsActive()) continue;


        FVector PosA = BodyA->getTransform().GetLocation();
        double MassA = BodyA->Mass;

        for (int32 j = i + 1; j < Count; ++j) {
            UGravityComponent* BodyB = Bodies[j];
            if (!BodyB || !BodyB->IsActive()) continue;

            FVector Difference = BodyB->getTransform().GetLocation() - PosA; //Vector de A a B

            double DistSq = Difference.SizeSquared();

            double DistanceFactor = DistSq + Softening; //Evitar division por cero y fuerzas infinitas

            FVector Direction = Difference.GetSafeNormal();
            double ForceMagnitude = (G * MassA * BodyB->Mass) / DistanceFactor;
            FVector ForceVector = Direction * ForceMagnitude;

            BodyA->AccumulatedForce += ForceVector;
            BodyB->AccumulatedForce -= ForceVector;
        }
    }

    // Aquí llamarás al GravityManager
    for (UGravityComponent* Body : Bodies)
    {
        if (Body && Body->IsActive())
        {
            Body->Integrate(DeltaTime);
        }
    }
}

void UNBodySimulationSubsystem::RegisterBody(UGravityComponent* Body)
{
    if (Body)
    {
        // Añadimos solo si no está ya (evita duplicados)
        Bodies.AddUnique(Body);
    }
}

void UNBodySimulationSubsystem::UnregisterBody(UGravityComponent* Body)
{
    Bodies.Remove(Body);
}

UWorld* UNBodySimulationSubsystem::GetTickableGameObjectWorld() const
{
    return GetWorld();
}

bool UNBodySimulationSubsystem::IsTickable() const
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