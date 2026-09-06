// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ICosmicNoiseStrategy.h"
#include "CosmicNoiseClass.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnNoiseSettingsChanged);

/**
 * Abstract base for procedural noise configuration assets.
 *
 * Allows creating noise strategies from editable data in Unreal Engine
 * and notifying changes at edit time.
 */
UCLASS(Abstract, BlueprintType)
class COSMICARCHITECTNOISE_API UCosmicNoiseClass : public UDataAsset
{
	GENERATED_BODY()
	
public: 
     /**
     * Event fired when asset properties are modified in the editor.
     */
	FOnNoiseSettingsChanged OnNoiseSettingsChanged;

    /**
     * Creates an instance of the noise strategy associated with this asset.
     */
	virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const;
protected:
#if WITH_EDITOR

    /**
     * Executes when an asset property is modified in the editor.
     */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
