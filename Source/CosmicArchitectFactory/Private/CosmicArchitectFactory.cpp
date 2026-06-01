// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#include "CosmicArchitectFactory.h"
#include "CosmicNoiseDSettingsActions.h"
#include "CosmicNoiseESettingsActions.h"
#include "CosmicNoiseCSettingsActions.h"
#include "CosmicFoliageCollectionActions.h"

#define LOCTEXT_NAMESPACE "FCosmicArchitectFactoryModule"


EAssetTypeCategories::Type FCosmicArchitectFactoryModule::CosmicCategory = EAssetTypeCategories::None;

void FCosmicArchitectFactoryModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module


    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

    // Categoría nueva en el menú de clic derecho
    CosmicCategory = AssetTools.RegisterAdvancedAssetCategory( 
        FName(TEXT("CosmicArchitect")),
        FText::FromString("Cosmic Architect")
    );

    // Registramos nuestras acciones y le pasamos la categoría
    TSharedPtr<FCosmicNoiseDefaultSettingsActions> NoiseActions = MakeShareable(new FCosmicNoiseDefaultSettingsActions());
    NoiseActions->MyAssetCategory = CosmicCategory;

    TSharedPtr<FCosmicNoiseEarthSettingsActions> EarthNoiseActions = MakeShareable(new FCosmicNoiseEarthSettingsActions());
    EarthNoiseActions->MyAssetCategory = CosmicCategory;

    TSharedPtr<FCosmicNoiseCraterSettingsActions> CraterNoiseActions = MakeShareable(new FCosmicNoiseCraterSettingsActions());
    CraterNoiseActions->MyAssetCategory = CosmicCategory;

    TSharedPtr<FCosmicFoliageCollectionActions> FoliageActions = MakeShareable(new FCosmicFoliageCollectionActions());
    FoliageActions->MyAssetCategory = CosmicCategory;

    AssetTools.RegisterAssetTypeActions(NoiseActions.ToSharedRef());
    AssetTools.RegisterAssetTypeActions(EarthNoiseActions.ToSharedRef());
    AssetTools.RegisterAssetTypeActions(CraterNoiseActions.ToSharedRef());
    AssetTools.RegisterAssetTypeActions(FoliageActions.ToSharedRef());
}

void FCosmicArchitectFactoryModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCosmicArchitectFactoryModule, CosmicArchitectFactory)