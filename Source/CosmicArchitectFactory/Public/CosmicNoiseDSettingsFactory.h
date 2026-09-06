// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "CosmicNoiseDSettingsFactory.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTFACTORY_API UCosmicNoiseDSettingsFactory : public UFactory
{
	GENERATED_BODY()
public:
    UCosmicNoiseDSettingsFactory();

    //To create an asset of the desired type
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

    virtual bool ShouldShowInNewMenu() const override;
};
 