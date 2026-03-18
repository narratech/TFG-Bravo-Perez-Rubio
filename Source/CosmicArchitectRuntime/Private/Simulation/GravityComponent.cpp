// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/GravityComponent.h"
#include "Simulation/GravitySubsystem.h"
#include "Components/PrimitiveComponent.h"

// Sets default values for this component's properties
UGravityComponent::UGravityComponent()
{
	// ...
}


void UGravityComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        RootPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());

        if (RootPrimitive)
        {
            // Asegurar que sea movible
            if (GravityMode != ECosmicGravityMode::None && RootPrimitive->Mobility != EComponentMobility::Movable)
            {
                RootPrimitive->SetMobility(EComponentMobility::Movable);
            }

            // Si quieres que siempre use físicas:
            // 
            if (!IsPlanet) {
                RootPrimitive->SetSimulatePhysics(true);
                RootPrimitive->SetEnableGravity(false);
                RootPrimitive->SetMassOverrideInKg(NAME_None, Mass, true);
            }
                
        }
    }

    UGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UGravitySubsystem>();

    if (!Subsystem) return;

    if (IsPlanet) {
        Mass = FMath::Square(RadiusKm * 1000) * SurfaceGravity / Subsystem->GetGravityConstant();
    }

    Subsystem->RegisterBody(this);
}

void UGravityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UGravitySubsystem* Subsystem =
            World->GetSubsystem<UGravitySubsystem>())
        {
            Subsystem->UnregisterBody(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void UGravityComponent::Integrate(double DeltaTime)
{

    FVector Acceleration = AccumulatedForce * 100 / Mass; //Calcular aceleracion a partir de fuerza y masa, de m a cm

    if (!IsPlanet && RootPrimitive && RootPrimitive->IsSimulatingPhysics())
    {
        RootPrimitive->AddForce(Acceleration, NAME_None, true); //Si simula fisicas, aplicar fuerza
    }
    else //Si no, calcular velocidad y actualizar posicion
    {
        AActor* Owner = GetOwner();

        if (!Owner) return;

        Velocity += Acceleration * DeltaTime;

        FVector NewLocation = Owner->GetActorLocation() + (Velocity * DeltaTime);

        Owner->SetActorLocation(NewLocation);
    }

    AccumulatedForce = FVector::ZeroVector;
}

void UGravityComponent::SetIsPlanet(bool bNewIsPlanet)
{
    if (bNewIsPlanet == IsPlanet) return;

    IsPlanet = bNewIsPlanet;

    // Notificar al subsistema del cambio
    if (UGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UGravitySubsystem>())
    {
        // Primero removemos y luego volvemos a registrar para actualizar las listas
        Subsystem->UnregisterBody(this);
        Subsystem->RegisterBody(this);
    }
}

float UGravityComponent::GetObjectRadius() const
{
    // Obtenemos el radio aproximado del StaticMesh (o del Actor entero)
    FVector Origin, BoxExtent;
    GetOwner()->GetActorBounds(true, Origin, BoxExtent);

    // Devolvemos la dimensión más grande (por si no es una esfera perfecta)
    return BoxExtent.GetMax();
}