// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain/CosmicClipmapComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Terrain/ClipmapMeshComponent.h"

// Sets default values for this component's properties
UCosmicClipmapComponent::UCosmicClipmapComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
    
	// ...
}


// Called when the game starts
void UCosmicClipmapComponent::BeginPlay()
{
    Super::BeginPlay();
    CreateLevels();
}


// Called every frame
void UCosmicClipmapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UCosmicClipmapComponent::CreateLevels()
{
    Levels.Empty();
    Levels.SetNum(NumLevels);

    for (int32 L = 0; L < NumLevels; ++L)
    {
        UClipmapMeshComponent* Mesh =
            NewObject<UClipmapMeshComponent>(GetOwner());

        Mesh->RegisterComponent();
        Mesh->AttachToComponent(
            GetOwner()->GetRootComponent(),
            FAttachmentTransformRules::KeepRelativeTransform
        );

        Mesh->LevelIndex = L;
        Mesh->Resolution = BaseResolution;
        Mesh->GridSpacing = BaseGridSpacing * (1 << L);
        Mesh->bIsRing = (L > 0);

        Mesh->BuildMesh();

        Levels[L].Mesh = Mesh;
    }
}

void UCosmicClipmapComponent::UpdateOrigins()
{

}


