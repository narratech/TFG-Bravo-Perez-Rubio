// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"


class  FCosmicNoiseDefaultSettingsActions : public FAssetTypeActions_Base
{
public:
    // The name that will appear in the menu
    virtual FText GetName() const override { return FText::FromString("Default Noise Settings"); }

    // The color of the stripe at the bottom of the Asset
    virtual FColor GetTypeColor() const override { return FColor(45, 175, 255); } // Blue

    // Which class this action represents
    virtual UClass* GetSupportedClass() const override;

    // Which menu category it will appear in
    virtual uint32 GetCategories() override;

    // ID of the custom category
    EAssetTypeCategories::Type MyAssetCategory;
	 
};
