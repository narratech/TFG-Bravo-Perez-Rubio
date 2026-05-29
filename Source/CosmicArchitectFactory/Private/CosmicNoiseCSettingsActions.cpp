// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.


#include "CosmicNoiseCSettingsActions.h"
#include "CosmicCraterNoiseSettings.h"

UClass* FCosmicNoiseCraterSettingsActions::GetSupportedClass() const
{
    return UCosmicCraterNoiseSettings::StaticClass();
}

uint32 FCosmicNoiseCraterSettingsActions::GetCategories()
{
    return MyAssetCategory;
} 