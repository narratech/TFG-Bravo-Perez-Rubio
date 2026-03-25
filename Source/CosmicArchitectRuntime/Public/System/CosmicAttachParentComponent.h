// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicAttachParentComponent.generated.h"

class USphereComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COSMICARCHITECTRUNTIME_API UCosmicAttachParentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCosmicAttachParentComponent();

	UFUNCTION(CallInEditor, Category = "Debug")
	void DebugTriggerState();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	// Trigger
	UPROPERTY()
	USphereComponent* TriggerSphere;

	// Actores manuales
	UPROPERTY(EditAnywhere, Category = "Attach")
	TArray<AActor*> ActorsToAttach;

	// Config
	UPROPERTY(EditAnywhere, Category = "Attach")
	bool bAttachPlayerPawn = true;

	UPROPERTY(EditAnywhere, Category = "Attach")
	float AttachRadiusKm = 1.f;

	UPROPERTY(EditAnywhere, Category = "Attach")
	bool bAutoDetach = true;

	// Debug
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebugArea = true;

	UPROPERTY(EditAnywhere, Category = "Debug")
	int32 DebugSegments = 64;

	UPROPERTY(EditAnywhere, Category = "Debug")
	float DebugThickness = 2.f;

	UPROPERTY(EditAnywhere, Category = "Debug")
	FColor DebugColor = FColor::Green;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	

	void TryAttach(AActor* Actor);
	void TryDetach(AActor* Actor);

	void DrawDebugArea();
};
