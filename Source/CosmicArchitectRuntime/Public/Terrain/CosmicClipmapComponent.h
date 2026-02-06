// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicClipmapComponent.generated.h"

USTRUCT(BlueprintType)
struct FClipmapLevel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GridSpacing = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector2D Origin = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AlphaOffset = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AlphaWidth = 100.0f;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COSMICARCHITECTRUNTIME_API UCosmicClipmapComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
    UCosmicClipmapComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;

    void UpdateViewerPosition();
    void UpdatePatchTransform();
    void UpdateLevels();
    void UpdateOrigins();
    void PushMaterialParameters();

public:
    // -------- General settings --------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clipmap")
    int32 NumLevels = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clipmap")
    float BaseGridSpacing = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clipmap")
    float HeightScale = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clipmap")
    float PlanetRadius = 6371000.0f;

    // -------- Runtime --------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Clipmap")
    FVector2D ViewerPos;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Clipmap")
    TArray<FClipmapLevel> Levels;

private:
    UMaterialInstanceDynamic* MID = nullptr;

		
};
