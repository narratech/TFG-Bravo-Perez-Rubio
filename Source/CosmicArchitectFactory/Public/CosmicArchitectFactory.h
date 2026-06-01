// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once
 
#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"

class FCosmicArchitectFactoryModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;


	static EAssetTypeCategories::Type CosmicCategory; 
};
