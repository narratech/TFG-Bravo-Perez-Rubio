// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#include "CosmicNoiseClass.h"

TSharedPtr<ICosmicNoiseStrategy> UCosmicNoiseClass::CreateStrategy() const
{
    return nullptr;
}

#if WITH_EDITOR
void UCosmicNoiseClass::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent); // Always call Super first or at the start

    // We get the property that changed and, if it is part of a struct, its "parent" property
    FProperty* Property = PropertyChangedEvent.Property;
    FProperty* MemberProperty = PropertyChangedEvent.MemberProperty;

    if (!Property || !MemberProperty) return;

    OnNoiseSettingsChanged.Broadcast();
}
#endif 