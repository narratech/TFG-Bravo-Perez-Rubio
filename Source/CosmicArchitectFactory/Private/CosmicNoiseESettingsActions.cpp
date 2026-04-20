// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicNoiseESettingsActions.h"
#include "CosmicEarthLikeNoiseSettings.h"

UClass* FCosmicNoiseEarthSettingsActions::GetSupportedClass() const
{
    return UCosmicEarthLikeNoiseSettings::StaticClass();
}

uint32 FCosmicNoiseEarthSettingsActions::GetCategories()
{
    return MyAssetCategory;
}