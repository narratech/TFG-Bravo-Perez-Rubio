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
    }

    // Crear MID
    if (AActor* Owner = GetOwner())
    {
        if (UStaticMeshComponent* Mesh = Owner->FindComponentByClass<UStaticMeshComponent>())
        {
            MID = Mesh->CreateDynamicMaterialInstance(0);
        }
    }
}


// Called every frame
void UCosmicClipmapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UpdateViewerPosition();
    UpdatePatchTransform();
    UpdateOrigins();
    PushMaterialParameters();
	// ...
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
        float BlockSize = Level.GridSpacing * 64.0f; // tamaño del tile
        Level.Origin.X = FMath::FloorToFloat(ViewerPos.X / BlockSize) * BlockSize;
        Level.Origin.Y = FMath::FloorToFloat(ViewerPos.Y / BlockSize) * BlockSize;
    }
}

void UCosmicClipmapComponent::UpdateLevels() {

}

void UCosmicClipmapComponent::PushMaterialParameters()
{
    if (!MID || Levels.Num() == 0) return;

    // Ejemplo: primer nivel (extiende a arrays luego)
    const FClipmapLevel& L0 = Levels[0];

    FVector4 ScaleFactor(
        L0.GridSpacing,
        L0.GridSpacing,
        L0.Origin.X,
        L0.Origin.Y
    );

    MID->SetVectorParameterValue(TEXT("ScaleFactor"), ScaleFactor);
    MID->SetVectorParameterValue(TEXT("ViewerPos"), FVector(ViewerPos, 0.0f));
    MID->SetScalarParameterValue(TEXT("ZScaleFactor"), HeightScale);
    MID->SetScalarParameterValue(TEXT("OneOverWidth"), 1.0f / L0.AlphaWidth);
    MID->SetScalarParameterValue(TEXT("AlphaOffset"), L0.AlphaOffset);
}

