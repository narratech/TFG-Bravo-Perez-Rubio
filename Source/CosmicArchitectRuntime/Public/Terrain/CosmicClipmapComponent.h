// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CosmicClipmapComponent.generated.h"

class UCosmicMeshComponent;
class UCosmicNoiseSettings;
class UCosmicCollisionComponent;

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
    void SetMaterialData(FColor color1, FColor color2, FColor colorHeight, float scale);

    USceneComponent* ParentRoot = nullptr;
    UCosmicNoiseSettings* NoiseSettings;

    float PlanetRadius = 100.f;

    bool bInitializedInEditor = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* BaseMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UTexture2D* DefaultTexture;


    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "8", ClampMax = "256"))
    int32 BaseResolution = 128;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "1", ClampMax = "20"))
    int32 NumLevels = 4;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "50"))
    int32 MinTriangleSize = 200;

    UPROPERTY(VisibleAnywhere, Category = "Clipmap")
    int64 BaseGridSpacing = 200;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    float HeightVisibility = 1.5f;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "0", ClampMax = "60"))
    float TimeToRefresh = 0.033f;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    bool IsPlanet = true;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    bool FreezeGeneration = false;

    UCosmicCollisionComponent* CollisionComponent;

protected:
    TArray<UCosmicMeshComponent*> Levels;
    UCosmicMeshComponent* FarLevel;
    
    float ElapsedTime = 0;
    float TimeToRefreshActive;
    bool bPerformaceMode = false;
    bool bInit = false;
    bool bPerformanceBuild = false;
    int64 BaseSpacing = 200;

    FVector CachedPlanetNormal;
    FVector CachedTangentX;
    FVector CachedTangentY;
    FVector CachedPlayerPos;

    FColor PlanetMainColor1 = FColor::Green;
    FColor PlanetMainColor2 = FColor::Red;
    FColor PlanetAltitudeColor = FColor::Yellow;
    float MaterialNoiseScale = 1.f;
    FVector LastPlayerPos;
    FVector CurrentActorPosition;
    FVector AccumulatedDelta = FVector::ZeroVector;
    FIntPoint TotalShift = FIntPoint::ZeroValue;

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Actualizar colisión cerca del jugador */
    void UpdateCollisionNearPlayer(const FVector& SurfacePos, const FVector& SurfaceNormal, const float DistanceToSurface);
    void UpdatePatchTransform(const FVector& SurfacePos, const FVector& N);
    float GetDistanceToSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);
    float GetDistanceToPlainSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);
    FVector GetPlayerLocation();
    FIntPoint ComputeGridShift(const FVector& PlayerPos, float GridSpacing);
    uint32 UpdateLevels(const FIntPoint& Shift);
    bool IsClipmapRingVisible(const int32 LevelIndex, const float DistanceToSurface);
    bool IsClipmapRingVisible(const int64 GridSpacing, const int64 Resolution, const float DistanceToSurface);
    void UpdatePlanetBasis(const FVector& SurfacePos, const FVector& N);
    void DecreaseClipmapLevelFull();
    void IncreaseClipmapLevelFull();

private:
    UPROPERTY(EditAnywhere, Category = "Materials")
    UMaterialInstanceDynamic* DynamicPlanetMat;
};

