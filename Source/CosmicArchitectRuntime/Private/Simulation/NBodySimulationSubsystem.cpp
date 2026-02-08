// Fill out your copyright notice in the Description page of Project Settings.


#include "Simulation/NBodySimulationSubsystem.h"
#include "Simulation/GravityComponent.h"

void UNBodySimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Log, TEXT("NBodySimulationSubsystem initialized"));
}

void UNBodySimulationSubsystem::Deinitialize()
{
    Bodies.Empty();
    Super::Deinitialize();
}

void UNBodySimulationSubsystem::Tick(float DeltaTime)
{
    // Aquí llamarás al GravityManager
    for (UGravityComponent* Body : Bodies)
    {
        if (!IsValid(Body)) continue;
        Body->Integrate(DeltaTime);
    }
}

void UNBodySimulationSubsystem::RegisterBody(UGravityComponent* Body)
{
    if (Body && !Bodies.Contains(Body))
    {
        Bodies.Add(Body);
    }
}

void UNBodySimulationSubsystem::UnregisterBody(UGravityComponent* Body)
{
    Bodies.Remove(Body);
}
