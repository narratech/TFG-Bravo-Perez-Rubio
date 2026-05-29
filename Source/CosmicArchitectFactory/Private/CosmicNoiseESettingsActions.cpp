// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.


#include "CosmicNoiseESettingsActions.h"
#include "CosmicEarthLikeNoiseSettings.h"

UClass* FCosmicNoiseEarthSettingsActions::GetSupportedClass() const
{
    return UCosmicEarthLikeNoiseSettings::StaticClass();
}

uint32 FCosmicNoiseEarthSettingsActions::GetCategories()
{
    return MyAssetCategory;
} 