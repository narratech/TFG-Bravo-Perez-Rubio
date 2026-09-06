// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicSpaceShipFactory.h"
#include "System/CosmicSpaceShip.h"
#include "CosmicArchitectFactory.h"

UCosmicSpaceShipFactory::UCosmicSpaceShipFactory()
{
    SupportedClass = UBlueprint::StaticClass();
    ParentClass = ACosmicSpaceShip::StaticClass();

    //Skip the class selection dialog window
    bSkipClassPicker = true;

    bCreateNew = true;
    bEditAfterNew = true;
}

FText UCosmicSpaceShipFactory::GetDisplayName() const
{
    return FText::FromString("Cosmic SpaceShip (Blueprint)");
}

uint32 UCosmicSpaceShipFactory::GetMenuCategories() const
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