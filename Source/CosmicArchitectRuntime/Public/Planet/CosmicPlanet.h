// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CosmicPlanet.generated.h"

class UCosmicClipmapComponent;

UCLASS()
class COSMICARCHITECTRUNTIME_API ACosmicPlanet : public AActor
{
	GENERATED_BODY()
	
public:	

	UPROPERTY(EditAnywhere)
	float Radius;

	// Sets default values for this actor's properties
	ACosmicPlanet();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* GridMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCosmicClipmapComponent* ClipmapComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
