// Fill out your copyright notice in the Description page of Project Settings.


#include "TFG-Bravo-Perez-Rubio/PlanetComponent.h"
#include "Components/PrimitiveComponent.h"

UPlanetComponent::UPlanetComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPlanetComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (!Owner) return;

    UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Owner->GetRootComponent());
    if (!Prim) return;

    // Forzar Mobility a Movable
    if (Prim->Mobility != EComponentMobility::Movable)
    {
        Prim->SetMobility(EComponentMobility::Movable);
    }

    // Activar físicas
    if (bAutoEnablePhysics)
    {
        Prim->SetSimulatePhysics(true);
    }

    // Quitar gravedad del motor
    if (bDisableEngineGravity)
    {
        Prim->SetEnableGravity(false);
    }

    // Ajustar masa al valor del componente
    Prim->SetMassOverrideInKg(NAME_None, Mass, true);

    Prim->SetUseCCD(true);
}

