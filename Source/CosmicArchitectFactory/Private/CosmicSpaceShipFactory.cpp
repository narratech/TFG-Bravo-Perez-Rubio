// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.


#include "CosmicSpaceShipFactory.h"
#include "System/CosmicSpaceShip.h"
#include "CosmicArchitectFactory.h"

UCosmicSpaceShipFactory::UCosmicSpaceShipFactory()
{
    SupportedClass = UBlueprint::StaticClass();
    ParentClass = ACosmicSpaceShip::StaticClass();

    //Omitimos la ventana de diálogo de selección de clase
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
    // Aquí necesitamos el ID de la categoría que creaste en tu StartupModule.
    // Si tienes acceso a ella a través de tu clase de Módulo, ponla aquí.
    // Si no, puedes devolver "EAssetTypeCategories::Blueprint" temporalmente 
    // y saldrá en la categoría por defecto de Blueprints.

    // Suponiendo que guardaste tu categoría en algún sitio global o puedes acceder a ella: 
    return FCosmicArchitectFactoryModule::CosmicCategory; 

    // Para una prueba rápida ahora mismo:
    //return EAssetTypeCategories::Blueprint;
}