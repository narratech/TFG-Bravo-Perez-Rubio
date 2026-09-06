// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "CosmicNoiseRSettingsFactory.h"
#include "CosmicRealisticNoiseSettings.h"

UCosmicNoiseRSettingsFactory::UCosmicNoiseRSettingsFactory()
{
    SupportedClass = UCosmicRealisticNoiseSettings::StaticClass();

    // Allow creating new objects from scratch (not just by importing)
    bCreateNew = true;

    //Automatically opens the details panel upon creation
    bEditAfterNew = true;
}

UObject* UCosmicNoiseRSettingsFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{

    UCosmicRealisticNoiseSettings* NewNoiseSettings = NewObject<UCosmicRealisticNoiseSettings>(InParent, InClass, InName, Flags | RF_Transactional);

    return NewNoiseSettings;
}

bool UCosmicNoiseRSettingsFactory::ShouldShowInNewMenu() const
{
	return true;
}

