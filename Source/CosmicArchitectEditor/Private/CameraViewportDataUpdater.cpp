// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraViewportDataUpdater.h"
#include "CosmicCameraBridge.h"
#include "Editor.h"
#include "EditorViewportClient.h"

void FCameraViewportDataUpdater::Tick(float DeltaTime)
{
    // Opcional: Control de frecuencia de actualización
    TimeSinceLastUpdate += DeltaTime;
    if (TimeSinceLastUpdate >= UpdateInterval)
    {
        UpdateCameraViewport();
        TimeSinceLastUpdate = 0.0f;
    }
}

bool FCameraViewportDataUpdater::IsTickable() const
{
    return true;
}

TStatId FCameraViewportDataUpdater::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FCameraViewportDataUpdater, STATGROUP_Tickables);
}

void FCameraViewportDataUpdater::UpdateCameraViewport()
{
    if (!GEditor) return;

    FViewport* Viewport = GEditor->GetActiveViewport();
    if (!Viewport) return;

    FEditorViewportClient* Client =
        static_cast<FEditorViewportClient*>(Viewport->GetClient());

    if (!Client) return;

    

    FCosmicCameraBridge::CameraLocation = Client->GetViewLocation();
    FCosmicCameraBridge::CameraRotation = Client->GetViewRotation();

    //UE_LOG(LogTemp, Warning, TEXT("Camara X: %.4f, Y: %.4f, Z: %.4f"),
    // FCosmicCameraBridge::CameraLocation.X, FCosmicCameraBridge::CameraLocation.Y, FCosmicCameraBridge::CameraLocation.Z);
}
