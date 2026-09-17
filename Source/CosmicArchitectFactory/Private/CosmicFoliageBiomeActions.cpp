// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicFoliageBiomeActions.h"
#include "CosmicFoliageBiome.h"

UClass* FCosmicFoliageBiomeActions::GetSupportedClass() const
{
    return UCosmicFoliageBiome::StaticClass();
}

uint32 FCosmicFoliageBiomeActions::GetCategories()
{
    return MyAssetCategory;
}
