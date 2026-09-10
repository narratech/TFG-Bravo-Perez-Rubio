// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicNoiseErosionSettingsFactory.h"
#include "CosmicErosionMultifractalNoiseSettings.h"

UCosmicNoiseErosionSettingsFactory::UCosmicNoiseErosionSettingsFactory()
{
    SupportedClass = UCosmicErosionMultifractalNoiseSettings::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UCosmicNoiseErosionSettingsFactory::FactoryCreateNew(
    UClass* InClass,
    UObject* InParent,
    FName InName,
    EObjectFlags Flags,
    UObject* Context,
    FFeedbackContext* Warn)
{
    UCosmicErosionMultifractalNoiseSettings* NewNoiseSettings = NewObject<UCosmicErosionMultifractalNoiseSettings>(InParent, InClass, InName, Flags | RF_Transactional);
    return NewNoiseSettings;
}

bool UCosmicNoiseErosionSettingsFactory::ShouldShowInNewMenu() const
{
    return true;
}
