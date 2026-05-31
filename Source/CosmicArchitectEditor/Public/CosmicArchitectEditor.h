
#pragma once

#include "Modules/ModuleManager.h"

/**
 * Módulo principal del editor para Cosmic Architect.
 *
 * Este módulo se encarga de inicializar y liberar
 * los sistemas específicos del editor, incluyendo
 * actualizadores y herramientas de integración.
 */
class FCosmicArchitectEditorModule : public IModuleInterface
{
public:

	/**
	 * Inicializa el módulo al cargarse en memoria.
	 *
	 * Aquí se registran e inicializan los sistemas
	 * necesarios para el funcionamiento del editor.
	 */
	virtual void StartupModule() override;

	/**
	 * Libera los recursos del módulo antes de descargarse.
	 * 
	 * Se utiliza para destruir objetos persistentes
	 * y realizar limpieza de memoria.
	 */
	virtual void ShutdownModule() override;
};
