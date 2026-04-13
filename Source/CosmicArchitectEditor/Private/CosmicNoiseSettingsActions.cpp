// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicNoiseSettingsActions.h"
#include "CosmicNoiseSettings.h"

UClass* FCosmicNoiseSettingsActions::GetSupportedClass() const
{
    return UCosmicNoiseSettings::StaticClass();
}

uint32 FCosmicNoiseSettingsActions::GetCategories()
{
    return MyAssetCategory;
}