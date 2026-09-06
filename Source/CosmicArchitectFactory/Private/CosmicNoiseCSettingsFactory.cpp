// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicNoiseCSettingsFactory.h"

#include "CosmicCraterNoiseSettings.h"

UCosmicNoiseCSettingsFactory::UCosmicNoiseCSettingsFactory()
{
    SupportedClass = UCosmicCraterNoiseSettings::StaticClass();

    // Allow creating new objects from scratch (not just by importing)
    bCreateNew = true;

    //Automatically opens the details panel upon creation
    bEditAfterNew = true;
}

UObject* UCosmicNoiseCSettingsFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{ 
    // Use NewObject to create the actual instance in Unreal memory
    UCosmicCraterNoiseSettings* NewNoiseSettings = NewObject<UCosmicCraterNoiseSettings>(InParent, InClass, InName, Flags | RF_Transactional);

    // Initialize default values
    // e.g.: NewNoiseSettings->Params.Seed = FMath::RandRange(1000, 9999);

    return NewNoiseSettings;
}

bool UCosmicNoiseCSettingsFactory::ShouldShowInNewMenu() const
{
    return true; // So that it appears in the "Miscellaneous" menu by default
}