// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "CosmicFoliageCollectionFactory.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTFACTORY_API UCosmicFoliageCollectionFactory : public UFactory
{
	GENERATED_BODY()
public:
    UCosmicFoliageCollectionFactory();

    //To create an asset of the desired type
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

    virtual bool ShouldShowInNewMenu() const override;
};
 