// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.


#include "CosmicFoliageCollection.h"

#if WITH_EDITOR
void UCosmicFoliageCollection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent); 

    // Obtenemos la propiedad que cambio y, si es parte de un struct, su propiedad "padre"
    FProperty* Property = PropertyChangedEvent.Property;
    FProperty* MemberProperty = PropertyChangedEvent.MemberProperty;

    if (!Property || !MemberProperty) return;

    OnFoliageCollectionChanged.Broadcast();
}
#endif 