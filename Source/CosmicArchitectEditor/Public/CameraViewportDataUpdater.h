
#pragma once

#include "TickableEditorObject.h"
#include "Templates/UniquePtr.h"

/**
 * Objeto tickable del editor encargado de actualizar
 * continuamente los datos de la cámara activa del viewport.
 *
 * Esta clase sincroniza la posición y rotación de la cámara
 * del editor con el bridge compartido utilizado por el sistema.
 */
class FCameraViewportDataUpdater : public FTickableEditorObject
{
public:

    /**
     * Ejecuta la actualización en cada tick del editor.
     *
     * @param DeltaTime Tiempo transcurrido desde el último tick.
     */
    virtual void Tick(float DeltaTime) override;

    /**
     * Indica si este objeto debe recibir ticks.
     *
     * @return true si el objeto puede actualizarse. 
     */
    virtual bool IsTickable() const override;

    /**
     * Obtiene el identificador de estadísticas del sistema de profiling.
     *
     * @return Identificador de estadísticas asociado al tick.
     */
    virtual TStatId GetStatId() const override;

private:

    /**
     * Actualiza la información de la cámara activa del viewport.
     *
     * Obtiene la posición y rotación actuales de la cámara
     * del editor y las almacena en el bridge compartido.
     */
    void UpdateCameraViewport();
};
