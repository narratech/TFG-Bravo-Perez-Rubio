// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicFoliageBiomeFactory.h"
#include "CosmicFoliageBiome.h"

UCosmicFoliageBiomeFactory::UCosmicFoliageBiomeFactory()
{
    SupportedClass = UCosmicFoliageBiome::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UCosmicFoliageBiomeFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UCosmicFoliageBiome>(InParent, InClass, InName, Flags | RF_Transactional);
}

bool UCosmicFoliageBiomeFactory::ShouldShowInNewMenu() const
{
    return true;
}
