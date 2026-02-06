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
}


// Called every frame
void UCosmicClipmapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ElapsedTime += DeltaTime;

    if (ElapsedTime > TimeToRefresh) {
        UpdatePatchTransform();

        if (bInit) {
            for (size_t i = 0; i < NumLevels; i++)
            {
                Levels[i].Mesh->UpdateMesh();
            }
        }

        ElapsedTime = 0;
    }
     
}

void UCosmicClipmapComponent::CreateLevels()
{
    Levels.Empty();
    Levels.SetNum(NumLevels);

    for (int32 L = 0; L < NumLevels; ++L)
    {
        UClipmapMeshComponent* Mesh = NewObject<UClipmapMeshComponent>(GetOwner());
        Mesh->RegisterComponent();

        // Adjuntamos al root que nos pasó el actor
        if (ParentRoot)
        {
            Mesh->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);
        }

        Mesh->LevelIndex = L;
        Mesh->Resolution = BaseResolution;
        Mesh->GridSpacing = BaseGridSpacing * (1 << L);
        Mesh->bIsRing = (L > 0);
        Mesh->PlanetRadius = PlanetRadius;

        Mesh->BuildBaseMesh();

        Levels[L].Mesh = Mesh;
    }

    bInit = true;
}

void UCosmicClipmapComponent::UpdatePatchTransform()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    if (Levels.Num() == 0) return;

    // posición del jugador
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return;

    FVector ViewerPosWorld = PC->PlayerCameraManager->GetCameraLocation();
    FVector PlanetCenter = Owner->GetActorLocation();

    for (FCosmicClipmapLevel& Level : Levels)
    {
        UClipmapMeshComponent* Mesh = Level.Mesh;
        if (!Mesh) continue;

        // Normal esférica
        FVector N = (ViewerPosWorld - PlanetCenter).GetSafeNormal();

        // Punto sobre la superficie
        FVector SurfacePos = PlanetCenter + N * PlanetRadius * HeightScale;

        // Orientación
        FVector Up = N;
        FVector Arbitrary = (FMath::Abs(Up.Z) < 0.99f) ? FVector::UpVector : FVector::ForwardVector;
        FVector Right = FVector::CrossProduct(Arbitrary, Up).GetSafeNormal();
        FVector Forward = FVector::CrossProduct(Up, Right).GetSafeNormal();
        FRotator PatchRotation = FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();

        // Mover la malla procedimental
        Mesh->SetWorldLocationAndRotation(SurfacePos, PatchRotation);
    }
}



void UCosmicClipmapComponent::UpdateOrigins()
{

}


