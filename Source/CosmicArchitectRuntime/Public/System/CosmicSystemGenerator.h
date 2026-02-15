// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "CosmicSystemGenerator.generated.h"

UCLASS(HideCategories = (
    Rendering,          // Oculta mallas, sombras, visibilidad
    Replication,        // Oculta red
    Input,              // Oculta input
    Collision,          // Oculta colisiones
    Actor,              // Oculta Tick, Spawn
    LOD,                // Oculta Level of Detail
    Cooking, 
    Networking,
    Physics,            // Oculta físicas (Gravity, Mass)
    Navigation,         // Oculta NavMesh
    Tags,               // Oculta etiquetas de actor
    DataLayers,         // Oculta capas de datos
    LevelInstance       // Oculta instancias de nivel
), AutoExpandCategories = ("Configuracion", "Acciones")) // Abre tus categorías automáticamente
class COSMICARCHITECTRUNTIME_API ACosmicSystemGenerator : public AActor
{
    GENERATED_BODY()

public:
    ACosmicSystemGenerator();

protected:
    // El volumen visual donde se generarán los cuerpos
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generacion")
    UBoxComponent* VolumenGeneracion;

    // Clase del actor a generar (una esfera)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion")
    TSubclassOf<AActor> ClaseAGenerar;

    // Número de cuerpos a crear
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion", meta = (ClampMin = "1"))
    int32 CantidadCuerpos;

    // La Semilla mágica: Si este número es igual, la generación será idéntica siempre
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuracion")
    int32 Semilla;

    // Almacenamos los actores creados para poder borrarlos luego
    UPROPERTY()
    TArray<AActor*> CuerposGenerados;

public:
    // Función marcada como CallInEditor para que aparezca un botón en el editor
    UFUNCTION(CallInEditor, Category = "Acciones")
    void GenerarCuerpos();

    // Función para limpiar la escena
    UFUNCTION(CallInEditor, Category = "Acciones")
    void LimpiarCuerpos();
};
