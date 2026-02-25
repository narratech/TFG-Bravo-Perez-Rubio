// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicClipmapComponent.generated.h"

class UCosmicMeshComponent;

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

    float PlanetRadius = 100.f;

    bool bInitializedInEditor = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* BaseMaterial;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "32", ClampMax = "256"))
    int32 BaseResolution = 64;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "1", ClampMax = "20"))
    int32 NumLevels = 5;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "50"))
    float MinTriangleSize = 200.f;

    UPROPERTY(VisibleAnywhere, Category = "Clipmap")
    float BaseGridSpacing = 200.f;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    float HeightVisibility = 2.5f;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "0", ClampMax = "60"))
    float TimeToRefresh = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    bool FreezeGeneration = false;

protected:
    TArray<UCosmicMeshComponent*> Levels;
    UCosmicMeshComponent* FarLevel;

    float ElapsedTime = 0;
    float TimeToRefreshActive;
    bool bPerformaceMode = false;
    bool bInit = false;

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void UpdatePatchTransform(const FVector& SurfacePos, const FVector& N);
    float GetDistanceToSurface(FVector& SurfacePos, FVector& N);
    bool IsClipmapRingVisible(const int32 LevelIndex, const float DistanceToSurface);
    bool IsClipmapRingVisible(const float GridSpacing, const int32 Resolution, const float DistanceToSurface);
    void ReduceClimapLevel();
    void IncreaseClipmapLevel();
    void UpdateOrigins();
};

