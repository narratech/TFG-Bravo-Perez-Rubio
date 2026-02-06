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

    GridMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GridMesh"));
    GridMesh->SetupAttachment(Root);
    GridMesh->SetCastShadow(false);
    GridMesh->bUseAsOccluder = false;

    ClipmapComponent = CreateDefaultSubobject<UCosmicClipmapComponent>(TEXT("ClipmapComponent"));

}

// Called when the game starts or when spawned
void ACosmicPlanet::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACosmicPlanet::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

