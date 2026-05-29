// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.

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