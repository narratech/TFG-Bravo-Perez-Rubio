// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CubeMapCell.h"

/**
 * Gestor del octree esferico usando CubeMap projection
 */
class COSMICARCHITECTCOMMON_API FCosmicOctree
{
public:
	FCosmicOctree();
	~FCosmicOctree();

    // Inicializar con radio del planeta
    void Initialize(float InPlanetRadius, int32 InMaxDepth = 8);

    // Obtener nodos dentro de un radio (en distancia sobre la superficie)
    void GetNodesInRadius(
        const FVector& ViewerLocation,
        const FVector& PlanetCenter,
        float ViewDistanceKm,      // Distancia de visión en km
        float DistanceToSurface,   // Distancia del jugador a la superficie en cm
        TArray<FCubeMapCell>& OutNodes) const;

    // Obtener los limites de un nodo
    FNodeBounds GetNodeBounds(const FCubeMapCell& Cell) const;

    // Obtener el centro del nodo en dirección normalizada
    FVector GetNodeCenter(const FCubeMapCell& Cell) const;

    FVector GetNodeCenterDirection(const FCubeMapCell& Cell) const;

    FVector GetNodeCenterWorld(const FCubeMapCell& Cell, const FVector& InPlanetCenter, float InPlanetRadius) const;

    // Calcular el area del nodo en km2
    float GetNodeAreaKm2(const FCubeMapCell& Cell) const;

    // Obtener todos los nodos hijos de un nodo
    void GetChildren(const FCubeMapCell& Parent, TArray<FCubeMapCell>& OutChildren) const;

    // Obtener el nodo padre
    FCubeMapCell GetParent(const FCubeMapCell& Child) const;

    // Encontrar que celda contiene un punto en la esfera
    FCubeMapCell FindCellAtLocation(const FVector& WorldPosition, const FVector& PlanetCenter, int32 TargetDepth = -1) const;

    // Debug: Obtener vertices para dibujar la celda
    TArray<FVector> GetDebugVertices(const FCubeMapCell& Cell) const;

private:
    float SphereRadius;
    int32 MaxDepth;

    // Convertir coordenadas de celda a punto en el cubo [-1, 1]
    FVector CellToCubePoint(const FCubeMapCell& Cell) const;

    // Convertir coordenadas UV en una cara a punto en el cubo
    FVector UVToCubePoint(int32 Face, float U, float V) const;

    // Convertir punto en el cubo a direccion normalizada (proyectada a esfera)
    FVector CubePointToDirection(const FVector& CubePoint) const;

    float GetCellRadius(const FCubeMapCell& Cell) const;

    // Obtener el tamano angular de una celda en radianes
    float GetCellAngularSize(const FCubeMapCell& Cell) const;

    //int32 GetMaxDepthFromDistance(float DistanceToSurfaceCm) const;

    void TraverseCell(const FCubeMapCell& Cell, const FVector& PlayerDir, float ViewAngleRad, TArray<FCubeMapCell>& OutNodes) const; 

    // Funcion auxiliar para obtener el nivel de detalle deseado segun distancia
    int32 GetDesiredDepth(float DistanceKm) const;
};
