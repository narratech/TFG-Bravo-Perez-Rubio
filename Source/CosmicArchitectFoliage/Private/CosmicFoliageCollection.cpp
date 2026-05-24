// Fill out your copyright notice in the Description page of Project Settings.


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