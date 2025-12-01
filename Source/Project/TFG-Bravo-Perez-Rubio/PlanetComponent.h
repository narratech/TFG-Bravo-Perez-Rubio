// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlanetComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_API UPlanetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UPlanetComponent();

    virtual void BeginPlay() override;

    /** Masa personalizada del planeta (gravedad personalizada) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    float Mass = 10000.0f;

    /** Radio visual o efectivo del planeta */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    float Radius = 100.0f;

    /** ¿Activar automáticamente físicas del mesh dueño? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    bool bAutoEnablePhysics = true;

    /** ¿Desactivar gravedad estándar del motor? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet")
    bool bDisableEngineGravity = true;
};
