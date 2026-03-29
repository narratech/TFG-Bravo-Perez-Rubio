// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/CosmicGravityComponent.h"
#include "Simulation/CosmicGravitySubsystem.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"

UCosmicGravityComponent::UCosmicGravityComponent()
{
	// ...
}


void UCosmicGravityComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        RootPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());

        if (RootPrimitive)
        {
            // E: Asegurar que sea movible
            // I: Make sure it's movable
            if (GravityMode != ECosmicGravityMode::None && RootPrimitive->Mobility != EComponentMobility::Movable)
            {
                RootPrimitive->SetMobility(EComponentMobility::Movable);
            }

            // E: Si quieres que siempre use físicas:
            // I: If you want to use physics always
            if (!IsPlanet) {
                RootPrimitive->SetSimulatePhysics(true);
                RootPrimitive->SetEnableGravity(false);
                RootPrimitive->SetMassOverrideInKg(NAME_None, Mass, true);
            }
                
        }
    }

    UCosmicGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UCosmicGravitySubsystem>();

    if (!Subsystem) return;

    if (IsPlanet) {
        Mass = FMath::Square(RadiusKm * 1000) * SurfaceGravity / Subsystem->GetGravityConstant();
    }

    Subsystem->RegisterBody(this);
}

void UCosmicGravityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UCosmicGravitySubsystem* Subsystem =
            World->GetSubsystem<UCosmicGravitySubsystem>())
        {
            Subsystem->UnregisterBody(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void UCosmicGravityComponent::Integrate(double DeltaTime)
{

    FVector Acceleration = AccumulatedForce * 100 / Mass; // E: Calcular aceleracion a partir de fuerza y masa, de m a cm
                                                          // I: Calculate acceleration from force and mass, from m to cm

    if (!IsPlanet && RootPrimitive && RootPrimitive->IsSimulatingPhysics())
    {
        RootPrimitive->AddForce(Acceleration, NAME_None, true); // E: Si simula fisicas, aplicar fuerza 
                                                                // I: If it uses phyisics, apply force
    }
    else // E: Si no, calcular velocidad y actualizar posicion 
        //  I: If not, calculate velocity and update position
    {
        AActor* Owner = GetOwner();

        if (!Owner) return;

        Velocity += Acceleration * DeltaTime;

        FVector NewLocation = Owner->GetActorLocation() + (Velocity * DeltaTime);

        Owner->SetActorLocation(NewLocation);
    }

    // E: Guardamos la dirección normalizada antes de limpiar la fuerza
    if (!AccumulatedForce.IsNearlyZero())
    {
        CurrentGravityDirection = AccumulatedForce.GetSafeNormal();
    }

    AccumulatedForce = FVector::ZeroVector;
}

void UCosmicGravityComponent::SetIsPlanet(bool bNewIsPlanet)
{
    if (bNewIsPlanet == IsPlanet) return;

    IsPlanet = bNewIsPlanet;

    // E: Notificar al subsistema del cambio
    // I: Notify the subsystem of the change
    if (UCosmicGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UCosmicGravitySubsystem>())
    {
        // E: Primero desregistramos y luego volvemos a registrar para actualizar las listas
        // I: Unregister first and then re-register to update lists
        Subsystem->UnregisterBody(this);
        Subsystem->RegisterBody(this);
    }
}

float UCosmicGravityComponent::GetObjectRadius() const
{
    // E: Obtenemos el radio aproximado del StaticMesh (o del Actor entero)
    // I: Obtain the approximated radius for the StaticMesh (or the whole Actor)
    FVector Origin, BoxExtent;
    GetOwner()->GetActorBounds(true, Origin, BoxExtent);
    
    // E: Devolvemos la dimensión más grande (por si no es una esfera perfecta)
    // I: We return the biggest dimension (in case it's not a perfect sphere)
    return BoxExtent.GetMax();
}