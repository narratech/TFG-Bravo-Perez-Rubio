
#include "CosmicFoliageCollectionActions.h"
#include "CosmicFoliageCollection.h"

UClass* FCosmicFoliageCollectionActions::GetSupportedClass() const
{
    return UCosmicFoliageCollection::StaticClass();
}

uint32 FCosmicFoliageCollectionActions::GetCategories()
{
    return MyAssetCategory;
} 
