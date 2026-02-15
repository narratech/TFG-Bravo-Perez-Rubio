// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/GravityComponent.h"
#include "Simulation/GravitySubsystem.h"

// Sets default values for this component's properties
UGravityComponent::UGravityComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
}


void UGravityComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(Owner->GetRootComponent()))
        {
            // Si no es movible, lo hacemos movible
            if (GravityMode != EGravityMode::None && Root->Mobility != EComponentMobility::Movable)
            {
                Root->SetMobility(EComponentMobility::Movable);
            }
        }
    }

    UGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UGravitySubsystem>();

    if (!Subsystem) return;

    if (IsPlanet) {
        Mass = FMath::Square(RadiusKm * 1000) * SurfaceGravity / Subsystem->GetGravityConstant();
        UE_LOG(LogTemp, Warning, TEXT("Masa %.4f"), Mass);
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

void UGravityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
}

void UGravityComponent::Integrate(double DeltaTime)
{
    // La aceleración ya está acumulada por el subsistema
    // Solo aplicamos la gravedad específica si es necesario
    //if (GravityMode == EGravityMode::SpecificPlanet && SpecificGravitySource) {
    //    // Podemos mantener la lógica de superficie aquí si queremos
    //    // Pero la fuerza principal ya vino del subsistema
    //    FVector Direction = (SpecificGravitySource->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal();
    //    double desiredAcc = SurfaceGravity * 100; // Convertir a cm/s^2
    //    // Añadimos esta aceleración a la acumulada
    //    AccumulatedForce += Direction * (desiredAcc * Mass);

    //    
    //}

    

    FVector Acceleration = AccumulatedForce * 100 / Mass;
    Velocity += Acceleration * DeltaTime;

    UE_LOG(LogTemp, Warning, TEXT("Fuerza %.4f N/m"), Acceleration.Length());

    //double Damping = 0.02;
    //double DampingFactor = FMath::Clamp(1.0 - (Damping * DeltaTime), 0.0, 1.0);
    //Velocity *= DampingFactor;

    //UE_LOG(LogTemp, Warning, TEXT("Velocidad %.4f m/s"), Velocity.Length() / 100);

    if (AActor* Owner = GetOwner())
    {
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