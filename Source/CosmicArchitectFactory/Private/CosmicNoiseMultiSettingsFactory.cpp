// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicNoiseMultiSettingsFactory.h"
#include "CosmicMultiNoiseSettings.h"

UCosmicNoiseMultiSettingsFactory::UCosmicNoiseMultiSettingsFactory()
{
    SupportedClass = UCosmicMultiNoiseSettings::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UCosmicNoiseMultiSettingsFactory::FactoryCreateNew(
    UClass* InClass,
    UObject* InParent,
    FName InName,
    EObjectFlags Flags,
    UObject* Context,
    FFeedbackContext* Warn)
{
    UCosmicMultiNoiseSettings* NewNoiseSettings = NewObject<UCosmicMultiNoiseSettings>(InParent, InClass, InName, Flags | RF_Transactional);
    return NewNoiseSettings;
}

bool UCosmicNoiseMultiSettingsFactory::ShouldShowInNewMenu() const
{
    return true;
}
