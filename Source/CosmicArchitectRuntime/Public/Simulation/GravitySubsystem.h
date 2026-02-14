// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GravitySubsystem.generated.h"

class UGravityComponent;

/**
 *
 */
UCLASS()
class COSMICARCHITECTRUNTIME_API UGravitySubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    // UWorldSubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UGravitySubsystem, STATGROUP_Tickables);
    }
    // Registro de cuerpos
    void RegisterBody(UGravityComponent* Body);
    void UnregisterBody(UGravityComponent* Body);
    double GetGravityConstant() const;
    virtual UWorld* GetTickableGameObjectWorld() const override;

private:
    UPROPERTY()
    TArray<UGravityComponent*> Bodies;
    TArray<UGravityComponent*> Planets;
};