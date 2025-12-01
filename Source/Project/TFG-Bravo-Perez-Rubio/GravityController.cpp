// Fill out your copyright notice in the Description page of Project Settings.


#include "TFG-Bravo-Perez-Rubio/GravityController.h"

#include "PlanetComponent.h"
#include "EngineUtils.h"          
#include "Components/PrimitiveComponent.h"

AGravityController::AGravityController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGravityController::BeginPlay()
{
    Super::BeginPlay();
}

void AGravityController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 1. Recolectar TODOS los PlanetComponent del mundo
    TArray<UPlanetComponent*> Planets;

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor) continue;

        UPlanetComponent* PlanetComp = Actor->FindComponentByClass<UPlanetComponent>();
        if (PlanetComp)
        {
            Planets.Add(PlanetComp);
        }
    }

    // 2. Aplicar gravedad entre ellos
    for (int32 i = 0; i < Planets.Num(); i++)
    {
        UPlanetComponent* PlanetA = Planets[i];
        AActor* OwnerA = PlanetA->GetOwner();
        if (!OwnerA) continue;

        UPrimitiveComponent* BodyA =
            Cast<UPrimitiveComponent>(OwnerA->GetRootComponent());

        if (!BodyA || !BodyA->IsSimulatingPhysics()) continue;

        FVector ForceSum = FVector::ZeroVector;

        for (int32 j = 0; j < Planets.Num(); j++)
        {
            if (i == j) continue;

            UPlanetComponent* PlanetB = Planets[j];
            AActor* OwnerB = PlanetB->GetOwner();
            if (!OwnerB) continue;

            FVector Dir = OwnerB->GetActorLocation() - OwnerA->GetActorLocation();
            float Distance = Dir.Size();

            if (Distance <= KINDA_SMALL_NUMBER) continue;

            Dir.Normalize();

            float ForceMag =
                (GravityConstant * PlanetA->Mass * PlanetB->Mass)
                / (Distance * Distance);

            ForceSum += Dir * ForceMag;
        }

        BodyA->AddForce(ForceSum);
    }
}


