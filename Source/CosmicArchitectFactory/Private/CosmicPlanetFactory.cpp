// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicPlanetFactory.h"
#include "Planet/CosmicPlanet.h"
#include "CosmicArchitectFactory.h"

UCosmicPlanetFactory::UCosmicPlanetFactory()
{
    SupportedClass = UBlueprint::StaticClass();
    ParentClass = ACosmicPlanet::StaticClass();

    //Omitimos la ventana de diálogo de selección de clase
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
    // Aquí necesitamos el ID de la categoría que creaste en tu StartupModule.
    // Si tienes acceso a ella a través de tu clase de Módulo, ponla aquí.
    // Si no, puedes devolver "EAssetTypeCategories::Blueprint" temporalmente 
    // y saldrá en la categoría por defecto de Blueprints. 

    // Suponiendo que guardaste tu categoría en algún sitio global o puedes acceder a ella:
    return FCosmicArchitectFactoryModule::CosmicCategory; 

    // Para una prueba rápida ahora mismo:
    //return EAssetTypeCategories::Blueprint;
}