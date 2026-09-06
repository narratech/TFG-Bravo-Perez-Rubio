// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CosmicEditorUtils.generated.h"

/**
 * Utility library related to editor functionalities.
 *
 * Provides Blueprint access to specific information
 * about the editor environment and the system.
 */
UCLASS()
class COSMICARCHITECTEDITOR_API UCosmicEditorUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Gets the current position of the active editor camera.
     *
     * Available only in editor environment.
     *
     * @return Active viewport camera position.
     */
    UFUNCTION(BlueprintCallable, Category = "Editor", meta = (DevelopmentOnly)) 
    static FVector GetEditorCameraPosition();

    /**
     * Indicates whether the current execution is within the editor.
     *
     * In runtime it will always return false.
     *
     * @return true if the code is executing within the editor.
     */
    UFUNCTION(BlueprintCallable, Category = "System")
    static bool IsInEditor();
};