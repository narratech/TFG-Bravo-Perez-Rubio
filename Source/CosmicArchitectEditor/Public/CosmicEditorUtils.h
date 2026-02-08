// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CosmicEditorUtils.generated.h"

/**
 * 
 */
UCLASS()
class COSMICARCHITECTEDITOR_API UCosmicEditorUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Editor", meta = (DevelopmentOnly))
    static FVector GetEditorCameraPosition();

    // Disponible en editor y runtime (pero comportamiento diferente)
    UFUNCTION(BlueprintCallable, Category = "System")
    static bool IsInEditor();
	
};
