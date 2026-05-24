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
            // Si el modo activo requiere movimiento, forzar movilidad Movable para
            // permitir que SetActorLocation y AddForce funcionen correctamente en runtime.
            if (GravityMode != ECosmicGravityMode::None && RootPrimitive->Mobility != EComponentMobility::Movable)
            {
                RootPrimitive->SetMobility(EComponentMobility::Movable);
            }

            // Los cuerpos orbitales usan el motor de físicas de Unreal para resolver colisiones
            // y respuesta a fuerzas. La gravedad interna del motor se desactiva porque la gravedad
            // la gestiona exclusivamente CosmicGravitySubsystem.
            if (!IsPlanet) {
                RootPrimitive->SetSimulatePhysics(true);
                RootPrimitive->SetEnableGravity(false);
                RootPrimitive->SetMassOverrideInKg(NAME_None, Mass, true);
            }

        }
    }

    UCosmicGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UCosmicGravitySubsystem>();

    if (!Subsystem) return;

    // La masa de un planeta se deriva de sus propiedades físicas observables (radio y gravedad superficial)
    // usando la fórmula inversa de la gravedad newtoniana: M = (g * R²) / G
    // Esto permite que el diseñador configure planetas con parámetros intuitivos sin calcular masas a mano.
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
    // AccumulatedForce está en Newtons (unidades SI). Se convierte a cm/s² multiplicando por 100
    // para pasar de metros a centímetros (sistema de unidades interno de Unreal Engine).
    // Fórmula: a = F / m  →  a_ue = (F / m) * 100
    FVector Acceleration = AccumulatedForce * 100 / Mass;

    if (!IsPlanet && RootPrimitive && RootPrimitive->IsSimulatingPhysics())
    {
        // Cuando el motor de físicas está activo, se delega en él la integración completa.
        // bAccelChange = true indica que el vector es una aceleración, no una fuerza,
        // evitando que el motor vuelva a dividir por masa internamente.
        RootPrimitive->AddForce(Acceleration, NAME_None, true);
    }
    else
    {
        // Integración de Euler semi-implícita para objetos sin físicas del motor (planetas y cuerpos cinemáticos).
        // Se actualiza primero la velocidad y después la posición para mayor estabilidad numérica.
        AActor* Owner = GetOwner();

        if (!Owner) return;

        Velocity += Acceleration * DeltaTime;

        FVector NewLocation = Owner->GetActorLocation() + (Velocity * DeltaTime);

        Owner->SetActorLocation(NewLocation);
    }

    // Se guarda la dirección de la aceleración neta antes de limpiar el acumulador.
    // CurrentGravityDirection permite que sistemas externos (orientación del personaje,
    // partículas, cámaras, etc.) conozcan el "abajo" gravitacional de este frame sin recalcularlo.
    if (!AccumulatedForce.IsNearlyZero())
    {
        CurrentGravityDirection = Acceleration;
    }

    AccumulatedForce = FVector::ZeroVector;
}

void UCosmicGravityComponent::SetIsPlanet(bool bNewIsPlanet)
{
    if (bNewIsPlanet == IsPlanet) return;

    IsPlanet = bNewIsPlanet;

    // El subsistema mantiene listas separadas para planetas y cuerpos orbitales.
    // Al cambiar de rol es necesario desregistrar y volver a registrar para que
    // el objeto quede correctamente clasificado en la lista correspondiente.
    if (UCosmicGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UCosmicGravitySubsystem>())
    {
        Subsystem->UnregisterBody(this);
        Subsystem->RegisterBody(this);
    }
}

float UCosmicGravityComponent::GetObjectRadius() const
{
    // GetActorBounds devuelve el AABB (bounding box alineado con ejes) del actor completo,
    // incluyendo componentes hijos. BoxExtent contiene las semi-extensiones en cada eje (X, Y, Z).
    // Se toma el valor máximo de los tres ejes para aproximar el radio de una esfera envolvente,
    // tolerando geometrías no perfectamente esféricas sin subestimarlas.
    FVector Origin, BoxExtent;
    GetOwner()->GetActorBounds(true, Origin, BoxExtent);

    return BoxExtent.GetMax();
}