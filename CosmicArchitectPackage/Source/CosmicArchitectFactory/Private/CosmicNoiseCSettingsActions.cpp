// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicNoiseCSettingsActions.h"
#include "CosmicCraterNoiseSettings.h"

UClass* FCosmicNoiseCraterSettingsActions::GetSupportedClass() const
{
    return UCosmicCraterNoiseSettings::StaticClass();
}

uint32 FCosmicNoiseCraterSettingsActions::GetCategories()
{
    return MyAssetCategory;
}