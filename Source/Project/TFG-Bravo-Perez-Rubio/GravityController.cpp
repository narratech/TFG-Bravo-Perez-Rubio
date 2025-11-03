// Fill out your copyright notice in the Description page of Project Settings.


#include "TFG-Bravo-Perez-Rubio/GravityController.h"

#include "Planet.h"
#include "Kismet/GameplayStatics.h"
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

    // Obtener todos los planetas
    TArray<AActor*> PlanetActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlanet::StaticClass(), PlanetActors);

    /*if (GEngine)
    {
        FString DebugText = FString::Printf(TEXT("Planetas encontrados: %d"), PlanetActors.Num());
        GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, DebugText);
    }*/

    // Calcular fuerzas entre cada par de planetas
    for (int32 i = 0; i < PlanetActors.Num(); i++)
    {
        APlanet* PlanetA = Cast<APlanet>(PlanetActors[i]);
        if (!PlanetA) continue;

        UPrimitiveComponent* CompA = Cast<UPrimitiveComponent>(PlanetA->GetRootComponent());
        if (!CompA || !CompA->IsSimulatingPhysics()) continue;

        FVector ForceSum = FVector::ZeroVector;

        for (int32 j = 0; j < PlanetActors.Num(); j++)
        {
            if (i == j) continue;

            APlanet* PlanetB = Cast<APlanet>(PlanetActors[j]);
            if (!PlanetB) continue;

            UPrimitiveComponent* CompB = Cast<UPrimitiveComponent>(PlanetB->GetRootComponent());
            if (!CompB) continue;

            FVector Dir = (PlanetB->GetActorLocation() - PlanetA->GetActorLocation());
            float Distance = Dir.Size();
            if (Distance <= KINDA_SMALL_NUMBER) continue;

            Dir.Normalize();

            // Ley de gravitación: F = G * (m1 * m2) / r^2
            float ForceMag = (GravityConstant * PlanetA->Mass * PlanetB->Mass) / (Distance * Distance);

            // Sumar la fuerza total
            ForceSum += Dir * ForceMag;
        }

        // Aplicar la fuerza resultante a este planeta
        CompA->AddForce(ForceSum);
    }
}

