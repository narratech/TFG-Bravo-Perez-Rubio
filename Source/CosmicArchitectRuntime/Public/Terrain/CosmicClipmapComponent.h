// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicClipmapComponent.generated.h"

class UClipmapMeshComponent;

USTRUCT()
struct FCosmicClipmapLevel
{
    GENERATED_BODY()

    int32 LevelIndex;
    float GridSpacing;
    FVector2D Origin;

    UClipmapMeshComponent* Mesh = nullptr;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UCosmicClipmapComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCosmicClipmapComponent();

    void CreateLevels();

    USceneComponent* ParentRoot = nullptr;

    float PlanetRadius = 1000.f; // default

    float HeightScale = 1.f; // multiplicador altura

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "32", ClampMax = "256"))
    int32 BaseResolution = 128;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    int32 NumLevels = 8;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    float BaseGridSpacing = 100.f;



protected:
    TArray<FCosmicClipmapLevel> Levels;

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void UpdatePatchTransform();
    void UpdateOrigins();
};

