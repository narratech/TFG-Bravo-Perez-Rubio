// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CosmicClipmapComponent.generated.h"

class ICosmicNoiseStrategy;
class UCosmicMeshComponent;
class UCosmicFoliageSpawner;
class UCosmicCollisionComponent;
class UCosmicNoiseClass;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
    HideCategories = (Activation, Tags, AssetUserData, Navigation, Rendering, Replication, Input, Actor, Collision, Cooking))
class UCosmicClipmapComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCosmicClipmapComponent();

    void CreateLevels();
    void CreatePerformanceLevel(bool bActive);

    void ClearLevels();
    void ReasignLevels();
    void SetMaterialData(FColor Color1, FColor Color2, FColor ColorCold, FColor ColorHot,
        FColor ColorSlope, float ScaleL, float ScaleM, float ScaleS);
    void RequestCompleteMeshUpdate();
    void UpdateNoiseEvaluator();

    USceneComponent* ParentRoot;
    UCosmicNoiseClass* NoiseClass;

    double PlanetRadius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* BaseMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UTexture2D* DefaultTexture;


    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "8", ClampMax = "256"))
    int32 BaseResolution = 128;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "1", ClampMax = "10"))
    int32 NumLevels = 4;

    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "10"))
    int32 MinTriangleSize = 100;

    UPROPERTY(VisibleAnywhere, Category = "Clipmap")
    int64 BaseGridSpacing = 200;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    float HeightVisibility = 5.0f;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    bool UseClipmap = true;

    UPROPERTY(EditAnywhere, Category = "Clipmap")
    bool FreezeGeneration = false;

    UCosmicCollisionComponent* CollisionComponent;
    UCosmicFoliageSpawner* FoliageSpawnerComponent;

protected:
    TArray<UCosmicMeshComponent*> Levels;
    UCosmicMeshComponent* FarLevel;

    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

    enum class EUpdatePhase : uint8
    {
        Foliage,
        Collision,
        Mesh
    };
    
    float ElapsedTime = 0;
    float TimeToRefreshActive;
    bool bPerformaceMode = false;
    bool bInit = false;
    bool bPerformanceBuild = false;
    bool bPendingTasksRemaining = false;
    bool bWaitingForFirstUpdateAfterPerformance = false;
    bool bBuildingLevels = false;
    bool IsPlanet = true;
    int64 BaseSpacing = 200;

    FColor PlanetMainColor1 = FColor::Green;
    FColor PlanetMainColor2 = FColor::Red;
    FColor PlanetColdColor = FColor::Yellow;
    FColor PlanetHotColor = FColor::Yellow;
    FColor PlanetSlopeColor = FColor::Yellow;
    float NoiseScaleSmall = 1.f;
    float NoiseScaleMedium = 1.f;
    float NoiseScaleLarge = 1.f;
    float TimeToRefresh = 0.01f;
    FVector LastPlayerPos;
    FVector LastMeshPlayerPos;
    FVector CurrentActorPosition;
    FVector AccumulatedDelta = FVector::ZeroVector;
    FIntPoint TotalShift = FIntPoint::ZeroValue;

    EUpdatePhase CurrentPhase = EUpdatePhase::Mesh;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
    // Se llama automáticamente cuando cambias algo en el panel de Detalles
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    /** Actualizar colisión cerca del jugador */
    void UpdateFoliagePhase(float DeltaTime, const FVector& ViewerPos, float DistanceToSurface);
    void UpdateCollisionPhase(const FVector& ViewerPos, const FVector& SurfacePos,
        const FVector& N, float DistanceToSurface);
    void UpdateMeshPhase(const FVector& ViewerPos, const FVector& SurfacePos,
        const FVector& N, float DistanceToSurface);
    void UpdateCollisionNearPlayer(const FVector& SurfacePos, const FVector& SurfaceNormal, const FRotator& PatchRotation, const double DistanceToSurface);
    void UpdatePatchTransform(const FVector& SurfacePos, const FVector& N);
    void BuildDynamicMaterial();
    FRotator GetPatchRotation(const FVector& SurfacePos) const;
    double GetDistanceToSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);
    double GetFastDistanceToSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);
    float GetDistanceToPlainSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);
    FVector GetPlayerLocation();
    FIntPoint ComputeGridShiftPlanar(const FVector& PlayerPos, float GridSpacing);
    FIntPoint ComputeGridShiftSpherical(const FVector& PlayerPos, const FVector& CurrentSurfacePos, int64 GridSpacing);
    FIntPoint ComputeGridShift(const FVector& PlayerPos, const FVector& CurrentSurfacePos, float GridSpacing);
    FVector2D GetSurfaceAngles(const FVector& SurfacePos);
    int32 CalculateDecreaseSteps(const double DistanceToSurface) const;
    int32 CalculateIncreaseSteps(const double DistanceToSurface) const;
    bool IsClipmapRingVisible(const int32 LevelIndex, const double DistanceToSurface) const;
    bool IsClipmapRingVisible(const int64 GridSpacing, const int64 Resolution, const double DistanceToSurface) const;
    void DecreaseClipmapLevelFull(int32 Steps = 1);
    void IncreaseClipmapLevelFull(int32 Steps = 1);

private:
    UPROPERTY()
    UMaterialInstanceDynamic* DynamicPlanetMat;

    FVector PreviousSurfacePos = FVector::ZeroVector;
    FVector2D LastSurfaceAngles;        // Ultimos angulos del jugador
    FVector2D AccumulatedLinearDelta;   // Desplazamiento lineal acumulado (en unidades)
};

