// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#include "CosmicArchitectFactory.h"
#include "CosmicNoiseDSettingsActions.h"
#include "CosmicNoiseCSettingsActions.h"
#include "CosmicNoiseMultiSettingsActions.h"
#include "CosmicNoiseErosionSettingsActions.h"
#include "CosmicFoliageCollectionActions.h"
#include "CosmicFoliageBiomeActions.h"

#define LOCTEXT_NAMESPACE "FCosmicArchitectFactoryModule"


EAssetTypeCategories::Type FCosmicArchitectFactoryModule::CosmicCategory = EAssetTypeCategories::None;

void FCosmicArchitectFactoryModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module


    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    // New category in the right-click menu
    CosmicCategory = AssetTools.RegisterAdvancedAssetCategory( 
        FName(TEXT("CosmicArchitect")),
        FText::FromString("Cosmic Architect")
    );

    // Register our actions and pass the category
    TSharedPtr<FCosmicNoiseDefaultSettingsActions> NoiseActions = MakeShareable(new FCosmicNoiseDefaultSettingsActions());
    NoiseActions->MyAssetCategory = CosmicCategory;

    TSharedPtr<FCosmicNoiseCraterSettingsActions> CraterNoiseActions = MakeShareable(new FCosmicNoiseCraterSettingsActions());
    CraterNoiseActions->MyAssetCategory = CosmicCategory;

    TSharedPtr<FCosmicNoiseMultiSettingsActions> MultiNoiseActions = MakeShareable(new FCosmicNoiseMultiSettingsActions());
    MultiNoiseActions->MyAssetCategory = CosmicCategory;

    TSharedPtr<FCosmicNoiseErosionSettingsActions> ErosionNoiseActions = MakeShareable(new FCosmicNoiseErosionSettingsActions());
    ErosionNoiseActions->MyAssetCategory = CosmicCategory;

    TSharedPtr<FCosmicFoliageCollectionActions> FoliageActions = MakeShareable(new FCosmicFoliageCollectionActions());
    FoliageActions->MyAssetCategory = CosmicCategory;

    TSharedPtr<FCosmicFoliageBiomeActions> BiomeActions = MakeShareable(new FCosmicFoliageBiomeActions());
    BiomeActions->MyAssetCategory = CosmicCategory;

    AssetTools.RegisterAssetTypeActions(NoiseActions.ToSharedRef());
    AssetTools.RegisterAssetTypeActions(CraterNoiseActions.ToSharedRef());
    AssetTools.RegisterAssetTypeActions(MultiNoiseActions.ToSharedRef());
    AssetTools.RegisterAssetTypeActions(ErosionNoiseActions.ToSharedRef());
    AssetTools.RegisterAssetTypeActions(FoliageActions.ToSharedRef());
    AssetTools.RegisterAssetTypeActions(BiomeActions.ToSharedRef());
}

void FCosmicArchitectFactoryModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCosmicArchitectFactoryModule, CosmicArchitectFactory)