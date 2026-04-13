// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "CosmicNoiseSettingsFactory.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTEDITOR_API UCosmicNoiseSettingsFactory : public UFactory
{
	GENERATED_BODY()
	
public:
    UCosmicNoiseSettingsFactory();

    //Para crear asset del tipo que quieras
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

    virtual bool ShouldShowInNewMenu() const override;
};
