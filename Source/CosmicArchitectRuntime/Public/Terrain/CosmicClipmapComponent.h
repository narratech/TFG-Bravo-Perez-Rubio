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

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "1", ClampMax = "15"))
    int32 NumLevels = 8;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "50", ClampMax = "500"))
    float BaseGridSpacing = 100.f;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    float HeightVisibility = 2.5f;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "0", ClampMax = "60"))
    float TimeToRefresh = 0.25f;

    bool bInit = false;

protected:
    TArray<FCosmicClipmapLevel> Levels;
    FCosmicClipmapLevel IntermediateLevel;

    float ElapsedTime = 0;
    bool bIntermediateExists = false;

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    float UpdatePatchTransform();
    void UpdateOrigins();
};

