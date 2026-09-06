// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CosmicFoliageTypes.h"
#include "CosmicFoliageCollection.generated.h"

/**
 * Delegate executed when the foliage collection is modified.
 */
DECLARE_MULTICAST_DELEGATE(FOnFoliageCollectionChanged);

/**
 * DataAsset that defines a foliage collection used in planet generation.
 *
 * Allows grouping multiple foliage entries and notifying changes in editor.
 */
UCLASS(BlueprintType)
class COSMICARCHITECTFOLIAGE_API UCosmicFoliageCollection : public UDataAsset
{ 
	GENERATED_BODY()

public:

	/**
	 * List of foliage entries that compose this collection.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foliage")
	TArray<FCosmicFoliageCollectionEntry> FoliageEntries;

	/**
	 * Event fired when the collection changes.
	 */
	FOnFoliageCollectionChanged OnFoliageCollectionChanged;

protected:

#if WITH_EDITOR

	/**
	 * Editor callback when an asset property is modified.
	 *
	 * Used to detect changes in the collection and notify dependent systems.
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif
};