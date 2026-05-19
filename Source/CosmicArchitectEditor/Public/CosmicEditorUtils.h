// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CosmicEditorUtils.generated.h"

/**
 * Librería de utilidades relacionadas con funcionalidades del editor.
 *
 * Proporciona acceso desde Blueprints a información
 * específica del entorno de edición y del sistema.
 */
UCLASS()
class COSMICARCHITECTEDITOR_API UCosmicEditorUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Obtiene la posición actual de la cámara activa del editor.
     *
     * Disponible únicamente en entorno editor.
     *
     * @return Posición de la cámara del viewport activo.
     */
    UFUNCTION(BlueprintCallable, Category = "Editor", meta = (DevelopmentOnly))
    static FVector GetEditorCameraPosition();

    /**
     * Indica si la ejecución actual se encuentra en el editor.
     *
     * En runtime siempre devolverá false.
     *
     * @return true si el código se está ejecutando dentro del editor.
     */
    UFUNCTION(BlueprintCallable, Category = "System")
    static bool IsInEditor();
};