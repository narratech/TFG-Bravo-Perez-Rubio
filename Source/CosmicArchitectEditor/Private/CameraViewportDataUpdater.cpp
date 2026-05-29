// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.


#include "CameraViewportDataUpdater.h"
#include "ModulesBridge/CosmicCameraBridge.h"
#include "Editor.h"
#include "EditorViewportClient.h"

void FCameraViewportDataUpdater::Tick(float DeltaTime)
{
    UpdateCameraViewport();
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
