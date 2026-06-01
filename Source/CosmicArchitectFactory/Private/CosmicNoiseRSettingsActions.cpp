// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicNoiseRSettingsActions.h"
#include "CosmicRealisticNoiseSettings.h"


UClass* FCosmicNoiseRealisticSettingsActions::GetSupportedClass() const
{
	return UCosmicRealisticNoiseSettings::StaticClass();
}

uint32 FCosmicNoiseRealisticSettingsActions::GetCategories()
{
	return MyAssetCategory;
}
