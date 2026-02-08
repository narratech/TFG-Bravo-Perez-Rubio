// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NBodySimulationSubsystem.generated.h"

class UGravityComponent;

/**
 * 
 */
UCLASS()
class COSMICARCHITECTRUNTIME_API UNBodySimulationSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
    // UWorldSubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }

    // Registro de cuerpos
    void RegisterBody(UGravityComponent* Body);
    void UnregisterBody(UGravityComponent* Body);

private:
    UPROPERTY()
    TArray<TObjectPtr<UGravityComponent>> Bodies;
};

