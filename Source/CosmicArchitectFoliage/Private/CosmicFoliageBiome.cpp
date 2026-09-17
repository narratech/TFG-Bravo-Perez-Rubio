// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicFoliageBiome.h"

#if WITH_EDITOR
void UCosmicFoliageBiome::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FProperty* Property = PropertyChangedEvent.Property;
    FProperty* MemberProperty = PropertyChangedEvent.MemberProperty;

    if (!Property || !MemberProperty) return;

    OnFoliageBiomeChanged.Broadcast();
}
#endif
