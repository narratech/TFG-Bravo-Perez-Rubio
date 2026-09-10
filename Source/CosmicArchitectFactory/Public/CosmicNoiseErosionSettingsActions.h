// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FCosmicNoiseErosionSettingsActions : public FAssetTypeActions_Base
{
public:
    virtual FText GetName() const override { return FText::FromString("Erosion Multifractal Settings"); }
    virtual FColor GetTypeColor() const override { return FColor(200, 130, 60); } // Warm terracotta / sedimentary rock
    virtual UClass* GetSupportedClass() const override;
    virtual uint32 GetCategories() override;

    EAssetTypeCategories::Type MyAssetCategory;
};
