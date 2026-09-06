// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "Modules/ModuleManager.h"
/**
 * Main editor module for Cosmic Architect.
 *
 * This module is responsible for initializing and releasing
 * editor-specific systems, including
 * updaters and integration tools.
 */
class FCosmicArchitectFoliageModule : public IModuleInterface
{
public:

	/**
	 * Initializes the module upon being loaded into memory.
	 *
	 * Here, systems required for editor operation
	 * are registered and initialized. 
	 */
	virtual void StartupModule() override;

	/**
	 * Releases module resources before being unloaded.
	 *
	 * Used to destroy persistent objects
	 * and perform memory cleanup.
	 */
	virtual void ShutdownModule() override;
};
