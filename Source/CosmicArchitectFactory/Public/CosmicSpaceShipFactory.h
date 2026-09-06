// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Factories/BlueprintFactory.h"
#include "CosmicSpaceShipFactory.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTFACTORY_API UCosmicSpaceShipFactory : public UBlueprintFactory
{
	GENERATED_BODY()

public:

	UCosmicSpaceShipFactory();

	// The name that will appear in the right-click menu
	virtual FText GetDisplayName() const override;

	virtual uint32 GetMenuCategories() const override;
};
 