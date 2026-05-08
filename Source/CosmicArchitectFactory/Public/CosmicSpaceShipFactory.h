// Fill out your copyright notice in the Description page of Project Settings.

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

	// El nombre que aparecerá en el menú del clic derecho
	virtual FText GetDisplayName() const override;

	virtual uint32 GetMenuCategories() const override;
};
