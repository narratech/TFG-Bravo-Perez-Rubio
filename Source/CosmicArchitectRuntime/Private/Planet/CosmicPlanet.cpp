// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet/CosmicPlanet.h"
#include "Terrain/CosmicClipmapComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ACosmicPlanet::ACosmicPlanet()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    ClipmapComponent = CreateDefaultSubobject<UCosmicClipmapComponent>(TEXT("ClipmapComponent"));
}


// Called when the game starts or when spawned
void ACosmicPlanet::BeginPlay()
{
	Super::BeginPlay();

    //InitClipmap();
    if (ClipmapComponent) {
        if (ClipmapComponent->bInitializedInEditor) {
            ClipmapComponent->ReasignLevels();
        }
        else {
            ClipmapComponent->ParentRoot = Root;
            ClipmapComponent->PlanetRadius = Radius * 100000;
            ClipmapComponent->CreatePerformanceLevel(true);
        }
    }
}

void ACosmicPlanet::InitClipmap()
{
    if (ClipmapComponent) {

        ClipmapComponent->ParentRoot = Root;
        ClipmapComponent->PlanetRadius = Radius * 100000;
        ClipmapComponent->ClearLevels();
        ClipmapComponent->CreatePerformanceLevel(true);
        ClipmapComponent->bInitializedInEditor = true;
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("No existe el clipmap"));
    }
}

// Called every frame
void ACosmicPlanet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACosmicPlanet::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

//#if WITH_EDITOR
    if (!GetWorld()->IsGameWorld())
    {
        InitClipmap();
    }
//#endif
}

void ACosmicPlanet::InitPlanet(float InRadiusKm)
{
    Radius = InRadiusKm;

    InitClipmap();
}

