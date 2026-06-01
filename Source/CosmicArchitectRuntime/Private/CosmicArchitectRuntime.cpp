// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicArchitectRuntime.h"

#define LOCTEXT_NAMESPACE "FCosmicArchitectRuntimeModule"

void FCosmicArchitectRuntimeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FCosmicArchitectRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCosmicArchitectRuntimeModule, CosmicArchitectRuntime) 