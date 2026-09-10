// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicArchitectRuntime.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FCosmicArchitectRuntimeModule"

void FCosmicArchitectRuntimeModule::StartupModule()
{
	// Register virtual shader source directory so that .ush files under
	// <PluginDir>/Shaders/ can be referenced as "/CosmicArchitect/..." 
	// inside UMaterialExpressionCustom include paths.
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CosmicArchitect"));
	if (Plugin.IsValid())
	{
		FString PluginShaderDir = FPaths::Combine(
			Plugin->GetBaseDir(),
			TEXT("Shaders")
		);
		AddShaderSourceDirectoryMapping(TEXT("/CosmicArchitect"), PluginShaderDir);
	}
}

void FCosmicArchitectRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCosmicArchitectRuntimeModule, CosmicArchitectRuntime) 