// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/GravityComponent.h"
#include "Simulation/NBodySimulationSubsystem.h"

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

    if (GravityMode == EGravityMode::NBody || GravityMode == EGravityMode::Hybrid)
    {
        if (UNBodySimulationSubsystem* Subsystem =
            GetWorld()->GetSubsystem<UNBodySimulationSubsystem>())
        {
            Subsystem->RegisterBody(this);
        }
    }
}

void UGravityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UNBodySimulationSubsystem* Subsystem =
            World->GetSubsystem<UNBodySimulationSubsystem>())
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
    if (Mass <= 0.f) return;

    FVector Acceleration = AccumulatedForce / Mass;
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
