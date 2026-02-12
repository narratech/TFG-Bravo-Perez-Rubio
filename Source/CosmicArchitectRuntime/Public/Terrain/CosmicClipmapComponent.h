// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicClipmapComponent.generated.h"

class UClipmapMeshComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UCosmicClipmapComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCosmicClipmapComponent();

    void CreateLevels();
    void CreatePerformanceLevel(bool bActive);

    void ClearLevels();
    void ReasignLevels();

    USceneComponent* ParentRoot = nullptr;

    float PlanetRadius = 1000.f; // default

    float HeightScale = 1.f; // multiplicador altura

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* BaseMaterial;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "32", ClampMax = "256"))
    int32 BaseResolution = 64;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "1", ClampMax = "15"))
    int32 NumLevels = 5;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "50", ClampMax = "500"))
    float BaseGridSpacing = 200.f;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    float HeightVisibility = 2.5f;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "0", ClampMax = "60"))
    float TimeToRefresh = 0.25f;

    bool bInit = false;

protected:
    TArray<UClipmapMeshComponent*> Levels;
    UClipmapMeshComponent* FarLevel;

    float ElapsedTime = 0;
    bool bPerformaceMode = false;
    float TimeToRefreshActive;

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    float UpdatePatchTransform();
    bool IsClipmapRingVisible(const int32 LevelIndex, const float DistanceToSurface);
    void UpdateOrigins();
};

