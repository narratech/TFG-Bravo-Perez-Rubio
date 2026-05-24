// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicNoiseDSettingsActions.h"
#include "CosmicDefaultNoiseSettings.h"

UClass* FCosmicNoiseDefaultSettingsActions::GetSupportedClass() const
{
    return UCosmicDefaultNoiseSettings::StaticClass();
}

uint32 FCosmicNoiseDefaultSettingsActions::GetCategories()
{
    return MyAssetCategory;
} 