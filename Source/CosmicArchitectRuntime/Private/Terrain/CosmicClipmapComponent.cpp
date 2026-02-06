// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain/CosmicClipmapComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

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

    Levels.SetNum(NumLevels);

    for (int32 i = 0; i < NumLevels; ++i)
    {
        Levels[i].GridSpacing = BaseGridSpacing * FMath::Pow(2.0f, i);
        Levels[i].AlphaWidth = Levels[i].GridSpacing * 8.0f;
        Levels[i].AlphaOffset = Levels[i].AlphaWidth * 0.5f;
        Levels[i].Radius = BaseRadius * FMath::Pow(2.f, i);
    }

    AActor* Owner = GetOwner();
    if (!Owner) return;

    UStaticMeshComponent* Mesh = Owner->FindComponentByClass<UStaticMeshComponent>();
    if (!Mesh) return;

    if (!ClipmapMaterial)
    {
        UE_LOG(LogTemp, Warning, TEXT("ClipmapMaterial no asignado"));
        return;
    }

    // 1️⃣ Asignar el material base
    Mesh->SetMaterial(0, ClipmapMaterial);

    // 2️⃣ Crear la instancia dinámica
    MID = Mesh->CreateDynamicMaterialInstance(0);
}


// Called every frame
void UCosmicClipmapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateViewerPosition();      // posición del jugador
    UpdatePatchTransform();      // mover y rotar el patch
    UpdateActiveLevels();        // activar niveles según distancia
    UpdateOrigins();             // actualizar origen de cada nivel activo
    PushMaterialParameters();    // enviar ScaleFactor al material
}


void UCosmicClipmapComponent::UpdatePatchTransform()
{
    AActor* Owner = GetOwner();
    if (!Owner) return;

    UStaticMeshComponent* Mesh =
        Owner->FindComponentByClass<UStaticMeshComponent>();
    if (!Mesh) return;

    FVector PlanetCenter = Owner->GetActorLocation();

    // Posición real del viewer
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return;

    FVector ViewerPosWorld =
        PC->PlayerCameraManager->GetCameraLocation();

    // Normal esférica
    FVector N = (ViewerPosWorld - PlanetCenter).GetSafeNormal();

    // Punto de contacto
    FVector SurfacePos = PlanetCenter + N * PlanetRadius * HeightScale;

    // Marco tangente
    FVector Up = N;

    FVector Arbitrary = (FMath::Abs(Up.Z) < 0.99f)
        ? FVector::UpVector
        : FVector::ForwardVector;

    FVector Right = FVector::CrossProduct(Arbitrary, Up).GetSafeNormal();
    FVector Forward = FVector::CrossProduct(Up, Right).GetSafeNormal();

    // Rotación
    FRotator PatchRotation =
        FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();

    Mesh->SetWorldLocationAndRotation(SurfacePos, PatchRotation);
}

void UCosmicClipmapComponent::UpdateViewerPosition()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return;

    FVector CamPos = PC->PlayerCameraManager->GetCameraLocation();

    // Simplificado: proyección plana XY
    ViewerPos = FVector2D(CamPos.X, CamPos.Y);
}

void UCosmicClipmapComponent::UpdateOrigins()
{
    for (FClipmapLevel& Level : Levels)
    {
        if (!Level.bActive) continue;

        float BlockSize = Level.GridSpacing * 64.0f; // tamaño de un bloque
        Level.Origin.X = FMath::FloorToFloat(ViewerPos.X / BlockSize) * BlockSize;
        Level.Origin.Y = FMath::FloorToFloat(ViewerPos.Y / BlockSize) * BlockSize;
    }
}


void UCosmicClipmapComponent::UpdateActiveLevels() {
    if (Levels.Num() == 0) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return;

    FVector ViewerPosWorld = PC->PlayerCameraManager->GetCameraLocation();
    FVector PlanetCenter = GetOwner()->GetActorLocation();

    float Dist = (ViewerPosWorld - PlanetCenter).Size();

    for (int32 i = 0; i < Levels.Num(); ++i)
    {
        FClipmapLevel& Level = Levels[i];
        Level.bActive = (Dist < Level.Radius);
    }
}

void UCosmicClipmapComponent::PushMaterialParameters()
{
    if (!MID || Levels.Num() == 0) return;

    for (int32 i = 0; i < Levels.Num(); ++i)
    {
        const FClipmapLevel& Level = Levels[i];
        if (!Level.bActive) continue;

        FVector4 ScaleFactor(Level.GridSpacing, Level.GridSpacing, Level.Origin.X, Level.Origin.Y);
        MID->SetVectorParameterValue(FName(*FString::Printf(TEXT("ScaleFactor%d"), i)), ScaleFactor);
    }
}


