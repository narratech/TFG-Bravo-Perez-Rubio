// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CosmicFoliageTypes.h"
#include "CosmicFoliageCollection.generated.h"

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "0", ClampMax = "1"))
    float GlobalDensity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement", meta = (ClampMin = "1", ClampMax = "100"))
    int32 SeedsPerSquareKm = 50;

    /** Obtiene una entrada aleatoria basada en pesos */
    const FCosmicFoliageCollectionEntry* GetRandomEntry(FRandomStream& Random) const;
};
