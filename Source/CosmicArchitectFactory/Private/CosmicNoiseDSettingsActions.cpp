// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.


#include "CosmicNoiseDSettingsActions.h"
#include "CosmicDefaultNoiseSettings.h"

UClass* FCosmicNoiseDefaultSettingsActions::GetSupportedClass() const
{
    return UCosmicDefaultNoiseSettings::StaticClass();
}

uint32 FCosmicNoiseDefaultSettingsActions::GetCategories()
{
    return MyAssetCategory;
} 