// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ICosmicNoiseStrategy.h"
#include "CosmicNoiseClass.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnNoiseSettingsChanged);

/**
 * 
 */
UCLASS(Abstract, BlueprintType)
class COSMICARCHITECTNOISE_API UCosmicNoiseClass : public UDataAsset
{
	GENERATED_BODY()
	
public: 
	FOnNoiseSettingsChanged OnNoiseSettingsChanged;

	virtual TSharedPtr<ICosmicNoiseStrategy> CreateStrategy() const;
protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
