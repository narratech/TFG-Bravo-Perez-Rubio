// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GravityComponent.generated.h"

UENUM(BlueprintType)
enum class EGravityMode : uint8
{
	None            UMETA(DisplayName = "None"),
	NearestPlanet  UMETA(DisplayName = "Nearest Planet"),
	SpecificPlanet UMETA(DisplayName = "Specific Planet"),
	AllPlanets     UMETA(DisplayName = "All Planets"),
	NBody          UMETA(DisplayName = "N-Body"),
	Hybrid         UMETA(DisplayName = "Hybrid")
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COSMICARCHITECTRUNTIME_API UGravityComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGravityComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FTransform getTransform() const { return GetOwner()->GetActorTransform(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGravityMode GravityMode = EGravityMode::NearestPlanet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* SpecificGravitySource = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	double Mass = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool AffectsOthers = true;

	FVector Velocity = FVector::ZeroVector;
	FVector AccumulatedForce = FVector::ZeroVector;

	void Integrate(double DeltaTime);
};
