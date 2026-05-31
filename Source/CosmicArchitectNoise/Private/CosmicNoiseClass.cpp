
#include "CosmicNoiseClass.h"

TSharedPtr<ICosmicNoiseStrategy> UCosmicNoiseClass::CreateStrategy() const
{
    return nullptr;
}

#if WITH_EDITOR
void UCosmicNoiseClass::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent); // Siempre llama al Super primero o al inicio

    // Obtenemos la propiedad que cambió y, si es parte de un struct, su propiedad "padre"
    FProperty* Property = PropertyChangedEvent.Property;
    FProperty* MemberProperty = PropertyChangedEvent.MemberProperty;

    if (!Property || !MemberProperty) return;

    OnNoiseSettingsChanged.Broadcast();
}
#endif 
