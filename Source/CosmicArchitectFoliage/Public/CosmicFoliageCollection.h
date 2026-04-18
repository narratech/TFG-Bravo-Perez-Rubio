// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CosmicFoliageTypes.h"
#include "CosmicFoliageCollection.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnFoliageCollectionChanged);

/**
 * DataAsset que define un conjunto de foliage para un bioma
 */
UCLASS(BlueprintType)
class COSMICARCHITECTFOLIAGE_API UCosmicFoliageCollection : public UDataAsset
{
	GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Foliage")
    TArray<FCosmicFoliageCollectionEntry> FoliageEntries;

    FOnFoliageCollectionChanged OnFoliageCollectionChanged;

protected:

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
