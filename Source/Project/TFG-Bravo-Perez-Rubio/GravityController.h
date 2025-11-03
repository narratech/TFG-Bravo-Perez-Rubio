// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GravityController.generated.h"

UCLASS()
class PROJECT_API AGravityController : public AActor
{
    GENERATED_BODY()

public:
    AGravityController();

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    /** Constante gravitacional (ajustable) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
    float GravityConstant = 0.1f;

    /** Rango máximo en el que se aplica la gravedad */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity")
    float InfluenceRadius = 100000.0f;

};
