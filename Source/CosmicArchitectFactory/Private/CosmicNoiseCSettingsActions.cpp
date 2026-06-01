// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

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