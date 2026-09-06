// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicOceanComponent.generated.h"

class UCosmicMeshComponent;
class UMaterialInstance;
class UMaterialInstanceDynamic;

/**
 * Component responsible for generating and managing planetary ocean mesh.
 *
 * This component creates an independent procedural sphere representing
 * the planet sea level and manages its dynamic material.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
	HideCategories = (Activation, Tags, AssetUserData, Navigation, Rendering, Replication, Input, Actor, Collision, Cooking))
	class COSMICARCHITECTRUNTIME_API UCosmicOceanComponent : public UActorComponent
{
	GENERATED_BODY()

public: 

	/**
	 * Ocean component constructor.
	 */
	UCosmicOceanComponent();

	/**
	 * Initializes ocean system.
	 *
	 * @param PlanetRadiusKm Planet radius in kilometers.
	 * @param Parent Parent component to which ocean mesh will be attached.
	 */
	void InitOcean(double PlanetRadiusKm, USceneComponent* Parent);

	/**
	 * Completely regenerates ocean mesh.
	 */
	void RegenerateOcean();

	/**
	 * Removes and destroys current ocean mesh.
	 */
	void ClearOcean();

	/**
	 * Clears inherited references after duplication without destroying original actor mesh.
	 *
	 * @param NewRoot Root component of new actor.
	 */
	void ResetPointersAfterDuplicate(USceneComponent* NewRoot);

	/**
	 * Indicates whether the planet has an ocean.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	bool bHasOcean = true;

	/**
	 * Sea level relative to planet radius in kilometers.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean"))
	double SeaLevelKm = -0.01;

	/**
	 * Ocean sphere resolution.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean", ClampMin = "8", ClampMax = "256"))
	int32 OceanResolution = 128;

	/**
	 * Base material used to render ocean.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean"))
	UMaterialInstance* OceanMaterial;

protected:

	/**
	 * Dynamic material instance of ocean.
	 */
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicOceanMat;

	/**
	 * Procedural mesh used to represent ocean.
	 */
	UCosmicMeshComponent* OceanMesh;

	/**
	 * Root component to which ocean mesh is attached.
	 */
	USceneComponent* ParentRoot;

	/**
	 * Planet radius in centimeters.
	 */
	double PlanetRadiusCm;

	/**
	 * Indicates whether ocean system has already been initialized.
	 */
	bool bInit = false;

	/**
	 * Builds and applies dynamic ocean material.
	 */
	void BuildDynamicMaterial();

#if WITH_EDITOR

	/**
	 * Executes automatically when a property changes from the editor.
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

public:

	/**
	 * Updates dynamic ocean parameters every frame.
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};