// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CosmicFoliageTypes.h"
#include "CosmicFoliageBiome.generated.h"

/**
 * Delegate executed when any property of the foliage biome data asset changes.
 */
DECLARE_MULTICAST_DELEGATE(FOnFoliageBiomeChanged);

/**
 * DataAsset that defines the collection of foliage meshes for a specific biome.
 */
UCLASS(BlueprintType)
class COSMICARCHITECTFOLIAGE_API UCosmicFoliageBiome : public UDataAsset
{
    GENERATED_BODY()

public:
    /**
     * List of foliage meshes configured for this biome.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    TArray<FCosmicFoliageMesh> Foliage;

    /**
     * Event fired when the biome foliage configuration changes.
     */
    FOnFoliageBiomeChanged OnFoliageBiomeChanged;

protected:
#if WITH_EDITOR
    /**
     * Editor callback when an asset property is modified.
     */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
