// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicPlanetFactory.h"
#include "Planet/CosmicPlanet.h"
#include "CosmicArchitectFactory.h"

UCosmicPlanetFactory::UCosmicPlanetFactory()
{
    SupportedClass = UBlueprint::StaticClass();
    ParentClass = ACosmicPlanet::StaticClass();

    //Skip the class selection dialog window
    bSkipClassPicker = true;

    bCreateNew = true;
    bEditAfterNew = true;
}

FText UCosmicPlanetFactory::GetDisplayName() const
{
    return FText::FromString("Cosmic Planet (Blueprint)");
}

uint32 UCosmicPlanetFactory::GetMenuCategories() const
{
    // Here we need the category ID created in your StartupModule.
    // If you have access to it through your Module class, put it here.
    // If not, you can return "EAssetTypeCategories::Blueprint" temporarily 
    // and it will appear in the default Blueprints category. 

    // Assuming you saved your category in some global location or can access it:
    return FCosmicArchitectFactoryModule::CosmicCategory; 

    // For a quick test right now:
    //return EAssetTypeCategories::Blueprint;
}