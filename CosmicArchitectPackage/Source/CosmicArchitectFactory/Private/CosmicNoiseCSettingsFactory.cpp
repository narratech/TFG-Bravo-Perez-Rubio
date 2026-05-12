// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicNoiseCSettingsFactory.h"

#include "CosmicCraterNoiseSettings.h"

UCosmicNoiseCSettingsFactory::UCosmicNoiseCSettingsFactory()
{
    SupportedClass = UCosmicCraterNoiseSettings::StaticClass();

    // Permitimos crear objetos nuevos desde cero (no solo importando)
    bCreateNew = true;

    //Abre automáticamente el panel de detalles al crearlo
    bEditAfterNew = true;
}

UObject* UCosmicNoiseCSettingsFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    // Usamos NewObject para crear la instancia real en la memoria de Unreal
    UCosmicCraterNoiseSettings* NewNoiseSettings = NewObject<UCosmicCraterNoiseSettings>(InParent, InClass, InName, Flags | RF_Transactional);

    // Inicializar valores por defecto
    // ej: NewNoiseSettings->Params.Seed = FMath::RandRange(1000, 9999);

    return NewNoiseSettings;
}

bool UCosmicNoiseCSettingsFactory::ShouldShowInNewMenu() const
{
    return true; // Para que aparezca en el menú de "Miscellaneous" por defecto
}