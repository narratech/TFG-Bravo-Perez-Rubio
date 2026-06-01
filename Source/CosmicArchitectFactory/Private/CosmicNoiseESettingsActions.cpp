// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

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