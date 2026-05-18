// Copyright Epic Games, Inc. All Rights Reserved.

#include "CosmicArchitectBenchmark.h"
#include "CosmicBenchmarkRecorder.h"
#include "CosmicArchitectRuntime/Public/ModulesBridge/CosmicBenchmarkBridge.h"

#define LOCTEXT_NAMESPACE "FCosmicArchitectBenchmarkModule"

void FCosmicArchitectBenchmarkModule::StartupModule()
{
	FCosmicBenchmarkBridge::OnRecordEvent = [](const FString& Name, const FString& Desc, float Value)
		{
			FCosmicBenchmarkRecorder::RecordEvent(Name, Desc, Value);
		};
}

void FCosmicArchitectBenchmarkModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCosmicArchitectBenchmarkModule, CosmicArchitectBenchmark)