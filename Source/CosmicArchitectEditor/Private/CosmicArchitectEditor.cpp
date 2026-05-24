// Copyright Epic Games, Inc. All Rights Reserved.

#include "CosmicArchitectEditor.h"
#include "CameraViewportDataUpdater.h"


#define LOCTEXT_NAMESPACE "FCosmicArchitectEditorModule"

TUniquePtr<FCameraViewportDataUpdater> CameraUpdater;


void FCosmicArchitectEditorModule::StartupModule()
{
	CameraUpdater = MakeUnique<FCameraViewportDataUpdater>();
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

}

void FCosmicArchitectEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	CameraUpdater.Reset();
}

#undef LOCTEXT_NAMESPACE
	 
IMPLEMENT_MODULE(FCosmicArchitectEditorModule, CosmicArchitectEditor)