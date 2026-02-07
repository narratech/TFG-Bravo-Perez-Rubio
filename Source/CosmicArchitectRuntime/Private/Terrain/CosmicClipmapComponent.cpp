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
        float DistanceToSurface = UpdatePatchTransform();

        if (bInit) {

           // UE_LOG(LogTemp, Warning, TEXT("Distancia de cambio: %.4f, Distancia a la superficie:  %.4f"),
             //   IntermediateLevel.Mesh->GridSpacing * BaseResolution * HeightVisibility, DistanceToSurface);

            if (bIntermediateExists) {

                //Activar/desactivar malla intermedia para mejorar rendimiento
                if (DistanceToSurface > IntermediateLevel.Mesh->GridSpacing * BaseResolution * HeightVisibility) {

                    if (!IntermediateLevel.Mesh->bActiveMesh)
                    {
                        IntermediateLevel.Mesh->SetMeshActive(true);
                        for (size_t i = 0; i < IntermediateLevel.Mesh->LevelIndex; i++)
                        {
                            Levels[i].Mesh->SetMeshActive(false);
                        }
                    }
                }
                else {
                    
                    if (IntermediateLevel.Mesh->bActiveMesh) 
                    {
                        IntermediateLevel.Mesh->SetMeshActive(false);
                        for (size_t i = 0; i < IntermediateLevel.Mesh->LevelIndex; i++)
                        {
                            Levels[i].Mesh->SetMeshActive(true);
                        }
                    }
                }
            }

            if (bIntermediateExists && IntermediateLevel.Mesh->bActiveMesh) {

                for (size_t i = 0; i < IntermediateLevel.Mesh->LevelIndex; i++)
                {
                    //UE_LOG(LogTemp, Warning, TEXT("Actualizando: %d"), IntermediateLevel.Mesh->LevelIndex + 1);
                    Levels[i].Mesh->UpdateMesh();
                }
            }
            else {

                for (size_t i = 0; i < NumLevels; i++)
                {
                    //UE_LOG(LogTemp, Warning, TEXT("Actualizando: %d"), NumLevels);
                    Levels[i].Mesh->UpdateMesh();
                }
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
        Mesh->bActiveMesh = true;

        Mesh->BuildBaseMesh();

        if (BaseMaterial)
            Mesh->SetMaterial(0, BaseMaterial);

        Levels[L].Mesh = Mesh;
    }

    // Nivel intermedio para mejorar rendimiendo si te encuentras a distancia lejana

    if (NumLevels > 4) {

        UClipmapMeshComponent* Mesh = NewObject<UClipmapMeshComponent>(GetOwner());
        Mesh->RegisterComponent();

        // Adjuntamos al root que nos pasó el actor
        if (ParentRoot)
        {
            Mesh->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);
        }

        Mesh->LevelIndex = NumLevels / 2;
        Mesh->Resolution = BaseResolution;
        Mesh->GridSpacing = BaseGridSpacing * (1 << NumLevels / 2);
        Mesh->bIsRing = false;
        Mesh->PlanetRadius = PlanetRadius;

        Mesh->BuildBaseMesh();
        Mesh->SetMeshActive(false);

        IntermediateLevel.Mesh = Mesh;

        bIntermediateExists = true;
    }

    bInit = true;
}

float UCosmicClipmapComponent::UpdatePatchTransform()
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    if (Levels.Num() == 0) return 0.f;

    // posición del jugador
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return 0.f;

    FVector ViewerPosWorld = PC->PlayerCameraManager->GetCameraLocation();
    FVector PlanetCenter = Owner->GetActorLocation();

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

    for (FCosmicClipmapLevel& Level : Levels)
    {
        UClipmapMeshComponent* Mesh = Level.Mesh;
        if (!Mesh) continue;

        // Mover la malla procedimental
        Mesh->SetWorldLocationAndRotation(SurfacePos, PatchRotation);
    }

    if (bIntermediateExists) {
        IntermediateLevel.Mesh->SetWorldLocationAndRotation(SurfacePos, PatchRotation);
    }

    return FVector::Distance(ViewerPosWorld, SurfacePos);
}



void UCosmicClipmapComponent::UpdateOrigins()
{

}


