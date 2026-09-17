// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "CosmicFoliageBiomeFactory.generated.h"

/**
 * Factory for creating UCosmicFoliageBiome assets in the editor.
 */
UCLASS()
class COSMICARCHITECTFACTORY_API UCosmicFoliageBiomeFactory : public UFactory
{
	GENERATED_BODY()

public:
    UCosmicFoliageBiomeFactory();

    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

    virtual bool ShouldShowInNewMenu() const override;
};
