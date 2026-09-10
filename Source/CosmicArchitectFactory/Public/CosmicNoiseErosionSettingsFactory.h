// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "CosmicNoiseErosionSettingsFactory.generated.h"

/**
 * Factory for creating UCosmicErosionMultifractalNoiseSettings DataAssets in the Unreal Editor.
 */
UCLASS()
class COSMICARCHITECTFACTORY_API UCosmicNoiseErosionSettingsFactory : public UFactory
{
    GENERATED_BODY()
public:
    UCosmicNoiseErosionSettingsFactory();

    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

    virtual bool ShouldShowInNewMenu() const override;
};
