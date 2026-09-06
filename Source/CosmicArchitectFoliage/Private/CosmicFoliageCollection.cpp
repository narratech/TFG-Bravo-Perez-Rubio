// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicFoliageCollection.h"

#if WITH_EDITOR
void UCosmicFoliageCollection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent); 

    // We get the property that changed and, if it is part of a struct, its "parent" property
    FProperty* Property = PropertyChangedEvent.Property;
    FProperty* MemberProperty = PropertyChangedEvent.MemberProperty;

    if (!Property || !MemberProperty) return;

    OnFoliageCollectionChanged.Broadcast();
}
#endif 