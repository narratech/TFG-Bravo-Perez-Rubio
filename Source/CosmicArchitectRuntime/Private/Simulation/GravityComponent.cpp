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


    if (UGravitySubsystem* Subsystem =
        GetWorld()->GetSubsystem<UGravitySubsystem>())
    {
        Subsystem->RegisterBody(this);
    }
    
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
    FVector Acceleration = AccumulatedForce / Mass;

    if (GravityMode == EGravityMode::SpecificPlanet) {
        FVector Direction = (SpecificGravitySource->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal();
        double desiredAcc = SurfaceGravity * 100;
        Acceleration = Direction * desiredAcc;
    }

    Velocity += Acceleration * DeltaTime;

    double Damping = 0.02;

    double DampingFactor = FMath::Clamp(1.0 - (Damping * DeltaTime), 0.0, 1.0);
    Velocity *= DampingFactor;

    if (AActor* Owner = GetOwner())
    {
        FVector NewLocation = Owner->GetActorLocation() + (Velocity * DeltaTime);
        Owner->SetActorLocation(NewLocation);
    }

    AccumulatedForce = FVector::ZeroVector;
}

float UGravityComponent::GetObjectRadius() const
{
    // Obtenemos el radio aproximado del StaticMesh (o del Actor entero)
    FVector Origin, BoxExtent;
    GetOwner()->GetActorBounds(true, Origin, BoxExtent);

    // Devolvemos la dimensión más grande (por si no es una esfera perfecta)
    return BoxExtent.GetMax();
}