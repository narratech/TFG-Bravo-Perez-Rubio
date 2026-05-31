

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
