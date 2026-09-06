// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "CosmicNoiseCSettingsFactory.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTFACTORY_API UCosmicNoiseCSettingsFactory : public UFactory
{
	GENERATED_BODY()
public:
    UCosmicNoiseCSettingsFactory();

    //To create an asset of the desired type
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

    virtual bool ShouldShowInNewMenu() const override;
};
 