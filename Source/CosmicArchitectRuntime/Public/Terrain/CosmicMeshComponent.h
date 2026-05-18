// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "CosmicNoiseGenerationTask.h"
#include "CosmicMeshComponent.generated.h"

class ICosmicNoiseStrategy;

/**
 * Define el cuadrante lógico utilizado por el clipmap
 * para gestionar desplazamientos y reconstrucciones.
 */
UENUM(BlueprintType)
enum class EClipmapQuadrant : uint8
{
    /** Posición base inicial */
    TopLeft = 0,

    /** Desplazamiento hacia la derecha */
    TopRight = 1,

    /** Desplazamiento hacia abajo */
    BottomLeft = 2,

    /** Desplazamiento hacia derecha y abajo */
    BottomRight = 3
};

/**
 * Componente procedural encargado de representar
 * un nivel individual del sistema clipmap.
 *
 * Funcionalidades principales:
 * - Generación de malla procedural proyectada sobre esfera.
 * - Generación de mallas esféricas simplificadas.
 * - Actualización asíncrona mediante tareas de ruido.
 * - Reescalado dinámico del nivel.
 * - Gestión de visibilidad y transformaciones.
 */
UCLASS()
class UCosmicMeshComponent : public UProceduralMeshComponent
{
    GENERATED_BODY()

public:

    /** Índice del nivel dentro del clipmap */
    int32 LevelIndex;

    /** Resolución de la cuadrícula */
    int32 Resolution;

    /** Espaciado entre vértices */
    int64 GridSpacing;

    /** Radio del planeta */
    double PlanetRadius;

    /** Indica si el nivel es un anillo exterior */
    bool bIsRing;

    /** Indica si la malla representa un planeta */
    bool bIsPlanet;

    /** Indica si la malla procedural ya fue creada */
    bool bMeshCreated = false;

    /** Indica si la malla es una esfera completa */
    bool bIsSphereMesh = false;

    /** Indica si la malla está visible actualmente */
    bool bActiveMesh;

    /** Transformación actual del patch */
    FTransform PatchTransform;

    /**
     * Vértices base deformados únicamente sobre la esfera
     * sin aplicar alturas de ruido adicionales.
     */
    TArray<FVector> BaseVertices;

    /** Normales base de la geometría */
    TArray<FVector> BaseNormals;

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

    virtual void OnComponentDestroyed(bool bDestroyingHierarchy);

    /**
     * Construye la malla procedural base proyectada
     * sobre la superficie del planeta.
     */
    void BuildBaseProjectedMesh();

    /**
     * Construye una esfera completa simplificada.
     */
    void BuildSphereMesh();

    /**
     * Reescala el nivel actual utilizando un nuevo spacing.
     *
     * @param GridSpacing Nuevo espaciado entre vértices.
     */
    void ReScaleLevel(int64 GridSpacing);

    /**
     * Actualiza la posición y rotación del patch.
     *
     * @param SurfacePos Posición sobre la superficie.
     * @param PatchRotation Rotación alineada con la normal.
     */
    void SetPositionAndRotation(const FVector& SurfacePos, const FRotator& PatchRotation);

    /**
     * Activa o desactiva visualmente la malla.
     *
     * @param active Estado de visibilidad.
     */
    void SetMeshActive(bool active);

    /**
     * Solicita una actualización asíncrona de ruido procedural.
     *
     * @param NoiseGenerationStrategy Estrategia de ruido activa.
     */
    void RequestMeshUpdate(TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy);

    /**
     * Comprueba si la tarea de generación terminó
     * y aplica la nueva geometría generada.
     *
     * @return True si la malla ya está actualizada.
     */
    bool CheckAndApplyMeshUpdate();

    /**
     * Comprueba si existe una tarea asíncrona activa.
     *
     * @return True si la tarea sigue ejecutándose.
     */
    bool IsTaskActive();

    /**
     * Cancela cualquier tarea asíncrona activa.
     */
    void CancelAsyncWork();

protected:

    /** Tarea asíncrona utilizada para generar ruido procedural */
    FAsyncTask<FCosmicNoiseGenerationTask>* NoiseTask = nullptr;

    /** Indica si actualmente se está generando ruido */
    bool bIsGeneratingNoise = false;
};