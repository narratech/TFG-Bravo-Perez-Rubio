// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CosmicCubeMapCell.h"

/**
 * Gestor encargado de administrar la subdivisión espacial esférica
 * mediante una proyección CubeMap.
 *
 * Esta clase permite:
 * - Obtener nodos visibles dentro de una distancia.
 * - Calcular relaciones jerárquicas entre celdas.
 * - Convertir posiciones entre espacio cúbico y esférico.
 * - Determinar celdas a partir de posiciones del mundo.
 * - Obtener información geométrica y de depuración.
 */
class COSMICARCHITECTCOMMON_API FCosmicOctree
{
public:

    /**
     * Constructor por defecto.
     */
    FCosmicOctree();

    /**
     * Destructor por defecto.
     */
    ~FCosmicOctree();

    /**
     * Inicializa el octree esférico.
     *
     * @param InPlanetRadius Radio del planeta en centímetros.
     * @param InMaxDepth Profundidad máxima permitida para subdivisión.
     */
    void Initialize(float InPlanetRadius, int32 InMaxDepth = 8);

    /**
     * Obtiene todos los nodos visibles dentro de un radio sobre la superficie.
     *
     * @param ViewerLocation Posición del observador en el mundo.
     * @param PlanetCenter Centro del planeta.
     * @param ViewDistanceKm Distancia de visión en kilómetros.
     * @param OutNodes Array de salida con las celdas visibles.
     */
    void GetNodesInRadius(
        const FVector& ViewerLocation,
        const FVector& PlanetCenter,
        float ViewDistanceKm,
        TArray<FCubeMapCell>& OutNodes) const;

    /**
     * Obtiene los límites geométricos de una celda.
     *
     * @param Cell Celda objetivo.
     * @return Información de límites de la celda.
     */
    FNodeBounds GetNodeBounds(const FCubeMapCell& Cell) const;

    /**
     * Obtiene la dirección normalizada del centro de una celda.
     *
     * @param Cell Celda objetivo.
     * @return Dirección normalizada en espacio esférico.
     */
    FVector GetNodeCenter(const FCubeMapCell& Cell) const;

    /**
     * Obtiene la dirección normalizada del centro de una celda.
     *
     * @param Cell Celda objetivo.
     * @return Vector dirección normalizado.
     */
    FVector GetNodeCenterDirection(const FCubeMapCell& Cell) const;

    /**
     * Obtiene la posición del centro de una celda en coordenadas del mundo.
     *
     * @param Cell Celda objetivo.
     * @param InPlanetCenter Centro del planeta.
     * @param InPlanetRadius Radio del planeta.
     * @return Posición del centro en el mundo.
     */
    FVector GetNodeCenterWorld(
        const FCubeMapCell& Cell,
        const FVector& InPlanetCenter,
        float InPlanetRadius) const;

    /**
     * Calcula el área aproximada de una celda en kilómetros cuadrados.
     *
     * @param Cell Celda objetivo.
     * @return Área aproximada en km².
     */
    float GetNodeAreaKm2(const FCubeMapCell& Cell) const;

    /**
     * Obtiene los cuatro nodos hijos de una celda.
     *
     * @param Parent Celda padre.
     * @param OutChildren Array de salida con las celdas hijas.
     */
    void GetChildren(const FCubeMapCell& Parent, TArray<FCubeMapCell>& OutChildren) const;

    /**
     * Obtiene la celda padre de una celda hija.
     *
     * @param Child Celda hija.
     * @return Celda padre correspondiente.
     */
    FCubeMapCell GetParent(const FCubeMapCell& Child) const;

    /**
     * Encuentra la celda que contiene una posición del mundo.
     *
     * @param WorldPosition Posición en el mundo.
     * @param PlanetCenter Centro del planeta.
     * @param TargetDepth Profundidad deseada de búsqueda.
     * @return Celda correspondiente a la posición indicada.
     */
    FCubeMapCell FindCellAtLocation(
        const FVector& WorldPosition,
        const FVector& PlanetCenter,
        int32 TargetDepth = -1) const;

    /**
     * Obtiene vértices de depuración para representar visualmente la celda.
     *
     * @param Cell Celda objetivo.
     * @return Lista de vértices para depuración.
     */
    TArray<FVector> GetDebugVertices(const FCubeMapCell& Cell) const;

private:

    /**
     * Radio de la esfera en centímetros.
     */
    float SphereRadius;

    /**
     * Profundidad máxima permitida para el octree.
     */
    int32 MaxDepth;

    /**
     * Convierte una celda a un punto central en el cubo proyectado.
     *
     * @param Cell Celda objetivo.
     * @return Punto correspondiente en el cubo.
     */
    FVector CellToCubePoint(const FCubeMapCell& Cell) const;

    /**
     * Convierte coordenadas UV de una cara del cubo a un punto cartesiano.
     *
     * @param Face Cara del cubo.
     * @param U Coordenada U normalizada.
     * @param V Coordenada V normalizada.
     * @return Punto en espacio cúbico.
     */
    FVector UVToCubePoint(int32 Face, float U, float V) const;

    /**
     * Proyecta un punto del cubo sobre la esfera.
     *
     * @param CubePoint Punto en el cubo.
     * @return Dirección normalizada sobre la esfera.
     */
    FVector CubePointToDirection(const FVector& CubePoint) const;

    /**
     * Calcula el radio máximo aproximado de una celda sobre la esfera.
     *
     * @param Cell Celda objetivo.
     * @return Radio máximo en centímetros.
     */
    float GetCellRadius(const FCubeMapCell& Cell) const;

    /**
     * Obtiene el tamaño angular de una celda en radianes.
     *
     * @param Cell Celda objetivo.
     * @return Tamaño angular en radianes.
     */
    float GetCellAngularSize(const FCubeMapCell& Cell) const;

    /**
     * Recorre recursivamente una celda y sus subdivisiones visibles.
     *
     * @param Cell Celda actual.
     * @param PlayerPos Posición del observador.
     * @param PlanetCenter Centro del planeta.
     * @param ViewDistanceCm Distancia máxima de visión en centímetros.
     * @param RequiredDepth Profundidad requerida.
     * @param OutNodes Array de salida con nodos visibles.
     */
    void TraverseCell(
        const FCubeMapCell& Cell,
        const FVector& PlayerPos,
        const FVector& PlanetCenter,
        float ViewDistanceCm,
        int32 RequiredDepth,
        TArray<FCubeMapCell>& OutNodes) const;
};