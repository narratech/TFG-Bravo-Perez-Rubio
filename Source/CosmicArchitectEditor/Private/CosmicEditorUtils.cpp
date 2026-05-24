// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicEditorUtils.h"
#include "ModulesBridge/CosmicCameraBridge.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Editor/UnrealEd/Public/EditorViewportClient.h"
#include "LevelEditorViewport.h"
#endif

FVector UCosmicEditorUtils::GetEditorCameraPosition()
{
#if WITH_EDITOR
    // Obtiene la posición de la cámara del viewport activo del editor.
    if (GEditor && GEditor->GetActiveViewport())
    {
        FEditorViewportClient* ViewportClient =
            static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());

        if (ViewportClient)
        {
            return ViewportClient->GetViewLocation();
        }
    }
#endif
    return FVector::ZeroVector;
}
 
bool UCosmicEditorUtils::IsInEditor()
{
#if WITH_EDITOR
    return GIsEditor;
#else
    return false;
#endif
}


