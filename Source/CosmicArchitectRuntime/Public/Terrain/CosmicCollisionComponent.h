// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ShapeComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Engine/EngineTypes.h"
#include "CosmicCollisionComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), HideCategories = (Physics, Collision))
class COSMICARCHITECTRUNTIME_API UCosmicCollisionComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
public:
    UCosmicCollisionComponent();

    // Genera la malla de colisión en una posicion
    void GenerateCollisionMesh(const FVector& Center, const FVector& SurfaceNormal, float Radius, int32 Resolution);

    // Limpia la malla actual
    void ClearCollisionMesh();

    // PROPIEDADES CONFIGURABLES EN DETAILS 

    /** Tamaño del área de colision alrededor del jugador (en cm) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Area", meta = (ClampMin = "100", ClampMax = "100000"))
    float CollisionAreaSize = 10000.0f; // 100 metros

    /** Resolucion de la malla de colision (menor = mas rapida) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Area", meta = (ClampMin = "4", ClampMax = "64"))
    int32 CollisionResolution = 16;

    /** Tamaño mínimo de triangulo para colisión (en cm) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Area", meta = (ClampMin = "50", ClampMax = "5000"))
    float CollisionMinTriangleSize = 500.0f;

    /** Distancia maxima a la que se genera colision */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Area", meta = (ClampMin = "1000", ClampMax = "100000"))
    float MaxCollisionDistance = 20000.0f;

    // CONFIGURACION DE COLISIONES

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Settings")
    TEnumAsByte<ECollisionEnabled::Type> CollisionEnabled = ECollisionEnabled::QueryAndPhysics;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Settings")
    TEnumAsByte<ECanBeCharacterBase> CanBeCharacterBase = ECB_Yes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Settings")
    TEnumAsByte<ECollisionChannel> ObjectType = ECC_WorldStatic;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Settings", meta = (ToolTip = "Custom collision channels"))
    TMap<TEnumAsByte<ECollisionChannel>, TEnumAsByte<ECollisionResponse>> CustomResponses;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Settings")
    bool bGenerateComplexCollision = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision Settings")
    bool bGenerateSimpleCollision = true;

    /** Forzar reconstruccion de colision */
    UFUNCTION(CallInEditor, Category = "Collision Area")
    void RebuildCollision();

protected:
    virtual void OnRegister() override;
    virtual UBodySetup* GetBodySetup() override;  

private:
    UBodySetup* BodySetup;  

    FVector CurrentCollisionCenter;
    float CurrentCollisionRadius;
    bool bNeedsRebuild = false;

    void UpdateCollisionSettings();
    void GenerateCollisionMeshData(const FVector& Center, const FVector& SurfaceNormal, float Radius, int32 Resolution, TArray<FVector>& OutVerts, TArray<int32>& OutTris);
};
