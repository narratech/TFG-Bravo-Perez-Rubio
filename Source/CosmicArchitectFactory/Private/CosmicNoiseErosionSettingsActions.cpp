// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicNoiseErosionSettingsActions.h"
#include "CosmicErosionMultifractalNoiseSettings.h"

UClass* FCosmicNoiseErosionSettingsActions::GetSupportedClass() const
{
    return UCosmicErosionMultifractalNoiseSettings::StaticClass();
}

uint32 FCosmicNoiseErosionSettingsActions::GetCategories()
{
    return MyAssetCategory;
}
