// Fill out your copyright notice in the Description page of Project Settings.


#include "TFG-Bravo-Perez-Rubio/Planet.h"

#include "Components/StaticMeshComponent.h"

APlanet::APlanet()
{
    PrimaryActorTick.bCanEverTick = true;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    Mesh->SetSimulatePhysics(true);

    Mesh->SetEnableGravity(false); // Para que no caiga por la gravedad global

    Mesh->SetMassOverrideInKg(NAME_None, Mass, true);
}

void APlanet::BeginPlay()
{
    Super::BeginPlay();
}

void APlanet::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

