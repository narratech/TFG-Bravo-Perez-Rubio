// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "CosmicNoiseRSettingsFactory.h"
#include "CosmicRealisticNoiseSettings.h"

UCosmicNoiseRSettingsFactory::UCosmicNoiseRSettingsFactory()
{
    SupportedClass = UCosmicRealisticNoiseSettings::StaticClass();

    // Permitimos crear objetos nuevos desde cero (no solo importando)
    bCreateNew = true;

    //Abre automáticamente el panel de detalles al crearlo
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

