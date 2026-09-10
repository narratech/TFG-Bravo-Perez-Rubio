// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FCosmicNoiseMultiSettingsActions : public FAssetTypeActions_Base
{
public:
    virtual FText GetName() const override { return FText::FromString("Multi Noise Settings"); }
    virtual FColor GetTypeColor() const override { return FColor(120, 220, 100); } // Emerald green
    virtual UClass* GetSupportedClass() const override;
    virtual uint32 GetCategories() override;

    EAssetTypeCategories::Type MyAssetCategory;
};
