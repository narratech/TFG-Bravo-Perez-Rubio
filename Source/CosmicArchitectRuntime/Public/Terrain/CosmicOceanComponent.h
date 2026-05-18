// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicOceanComponent.generated.h"

class UCosmicMeshComponent;
class UMaterialInstance;
class UMaterialInstanceDynamic;

/**
 * Componente encargado de generar y gestionar la malla del oceano planetario.
 *
 * Este componente crea una esfera procedural independiente que representa
 * el nivel del mar del planeta y administra su material dinamico.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
	HideCategories = (Activation, Tags, AssetUserData, Navigation, Rendering, Replication, Input, Actor, Collision, Cooking))
	class COSMICARCHITECTRUNTIME_API UCosmicOceanComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/**
	 * Constructor del componente oceano.
	 */
	UCosmicOceanComponent();

	/**
	 * Inicializa el sistema oceánico.
	 *
	 * @param PlanetRadiusKm Radio del planeta en kilometros.
	 * @param Parent Componente padre al que se adjuntara la malla del oceano.
	 */
	void InitOcean(double PlanetRadiusKm, USceneComponent* Parent);

	/**
	 * Regenera completamente la malla del oceano.
	 */
	void RegenerateOcean();

	/**
	 * Elimina y destruye la malla del oceano actual.
	 */
	void ClearOcean();

	/**
	 * Indica si el planeta posee oceano.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	bool bHasOcean = true;

	/**
	 * Nivel del mar relativo al radio del planeta en kilometros.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean"))
	double SeaLevelKm = -0.01;

	/**
	 * Resolucion de la esfera del oceano.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean", ClampMin = "8", ClampMax = "256"))
	int32 OceanResolution = 128;

	/**
	 * Material base utilizado para renderizar el oceano.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean"))
	UMaterialInstance* OceanMaterial;

protected:

	/**
	 * Instancia dinamica del material del oceano.
	 */
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicOceanMat;

	/**
	 * Malla procedural utilizada para representar el oceano.
	 */
	UCosmicMeshComponent* OceanMesh;

	/**
	 * Componente raiz al que se adjunta la malla del oceano.
	 */
	USceneComponent* ParentRoot;

	/**
	 * Radio del planeta en centimetros.
	 */
	double PlanetRadiusCm;

	/**
	 * Indica si el sistema oceánico ya fue inicializado.
	 */
	bool bInit = false;

	/**
	 * Construye y aplica el material dinamico del oceano.
	 */
	void BuildDynamicMaterial();

#if WITH_EDITOR

	/**
	 * Se ejecuta automaticamente cuando una propiedad cambia desde el editor.
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

public:

	/**
	 * Actualiza parametros dinamicos del oceano cada frame.
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};