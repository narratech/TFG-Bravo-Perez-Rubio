// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Planet.generated.h"

UCLASS()
class PROJECT_API APlanet : public AActor
{
    GENERATED_BODY()

public:
    APlanet();

    virtual void Tick(float DeltaTime) override;

    /** Masa del planeta (usada para calcular la gravedad) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    float Mass = 10000.0f;

    /** Radio visual del planeta */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    float Radius = 500.0f;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;
};
