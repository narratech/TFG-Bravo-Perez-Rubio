// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicNoiseMultiSettingsActions.h"
#include "CosmicMultiNoiseSettings.h"

UClass* FCosmicNoiseMultiSettingsActions::GetSupportedClass() const
{
    return UCosmicMultiNoiseSettings::StaticClass();
}

uint32 FCosmicNoiseMultiSettingsActions::GetCategories()
{
    return MyAssetCategory;
}
