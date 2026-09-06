// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "TickableEditorObject.h"
#include "Templates/UniquePtr.h"

/**
 * Editor tickable object responsible for continuously
 * updating the active viewport camera data.
 *
 * This class synchronizes the editor camera's position and rotation
 * with the shared bridge used by the system.
 */
class FCameraViewportDataUpdater : public FTickableEditorObject
{
public:

    /**
     * Executes the update on each editor tick.
     *
     * @param DeltaTime Time elapsed since the last tick.
     */
    virtual void Tick(float DeltaTime) override;

    /**
     * Indicates whether this object should receive ticks.
     *
     * @return true if the object can be updated. 
     */
    virtual bool IsTickable() const override;

    /**
     * Gets the stat identifier for the profiling system.
     *
     * @return Stat identifier associated with the tick.
     */
    virtual TStatId GetStatId() const override;

private:

    /**
     * Updates active viewport camera information.
     *
     * Obtains the current position and rotation of the editor
     * camera and stores them in the shared bridge.
     */
    void UpdateCameraViewport();
};