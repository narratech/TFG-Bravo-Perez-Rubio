// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GravityComponent.generated.h"

UENUM(BlueprintType)
enum class ECosmicGravityMode : uint8
{
	None           UMETA(DisplayName = "None"),
	NearestPlanet  UMETA(DisplayName = "Nearest Planet"),
	SpecificPlanet UMETA(DisplayName = "Specific Planet"),
	AllPlanets     UMETA(DisplayName = "All Planets"),
	NBody          UMETA(DisplayName = "N-Body")
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COSMICARCHITECTRUNTIME_API UGravityComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGravityComponent();

	FTransform getTransform() const { return GetOwner()->GetActorTransform(); }

    // Modo de gravedad - siempre visible
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config")
    ECosmicGravityMode GravityMode = ECosmicGravityMode::NearestPlanet;

    // Masa - visible SOLO cuando NO es planeta
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "!IsPlanet", EditConditionHides))
    double Mass = 100.0f;

    // Radio - visible SOLO cuando es planeta
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "IsPlanet", EditConditionHides, ClampMin = "0.001",
            UIMin = "0.001", UIMax = "1000000",
            ToolTip = "Radio del planeta en kilómetros"))
    float RadiusKm = 1.0f;

    // Gravedad en superficie - visible SOLO cuando es planeta
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "IsPlanet", EditConditionHides, ClampMin = "0.0"))
    float SurfaceGravity = 9.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config")
    bool AffectsOthers = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config")
    bool IsAffectedByOthers = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity Config")
    bool IsPlanet = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "GravityMode == EGravityMode::SpecificPlanet", EditConditionHides))
    AActor* SpecificGravitySource = nullptr;

    UPROPERTY()
    UPrimitiveComponent* RootPrimitive = nullptr;

	FVector Velocity = FVector::ZeroVector;
	FVector AccumulatedForce = FVector::ZeroVector;

	void Integrate(double DeltaTime);
	void SetIsPlanet(bool bNewIsPlanet);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	float GetObjectRadius() const;
};
