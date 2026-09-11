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
 * the planet sea level and manages its dynamic material. Supports both
 * a custom auto-generated Gerstner wave material and manual material override.
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

	// OCEAN TOGGLE

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

	// MATERIAL MODE 

	/**
	 * If true, uses the auto-generated Gerstner wave material.
	 * If false, uses the manually assigned OceanMaterial.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean"))
	bool bUseGeneratedMaterial = true;

	/**
	 * Base material used to render ocean (manual override).
	 * Only used when bUseGeneratedMaterial is false.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean && !bUseGeneratedMaterial"))
	UMaterialInstance* OceanMaterial;

	// WAVE CONFIGURATION 

	/**
	 * Global wave amplitude multiplier. Higher = taller waves.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean && bUseGeneratedMaterial", ClampMin = "0.0", ClampMax = "10.0"))
	float WaveAmplitudeScale = 1.0f;

	/**
	 * Global wave steepness (sharpness of crests). Q factor [0,1].
	 * Higher values create sharper, more peaked wave crests.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean && bUseGeneratedMaterial", ClampMin = "0.0", ClampMax = "1.0"))
	float WaveSteepness = 0.5f;

	/**
	 * Global wave animation speed multiplier.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean && bUseGeneratedMaterial", ClampMin = "0.0", ClampMax = "5.0"))
	float WaveSpeed = 1.0f;

	/**
	 * Radius of action for wave effects in km. Waves attenuate beyond this distance.
	 * Set to 0 for unlimited range (global waves).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean && bUseGeneratedMaterial", ClampMin = "0.0"))
	float WaveActionRadiusKm = 5.0f;

	/**
	 * Transition zone width for wave falloff in km.
	 * Controls how smoothly waves fade at the edge of the action radius.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves",
		meta = (EditCondition = "bHasOcean && bUseGeneratedMaterial", ClampMin = "0.001"))
	float WaveActionFalloffKm = 1.0f;

	// APPEARANCE 

	/**
	 * Shallow water tint color.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Appearance",
		meta = (EditCondition = "bHasOcean && bUseGeneratedMaterial"))
	FLinearColor WaterShallowColor = FLinearColor(0.1f, 0.4f, 0.6f, 1.0f);

	/**
	 * Deep water absorption color.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Appearance",
		meta = (EditCondition = "bHasOcean && bUseGeneratedMaterial"))
	FLinearColor WaterDeepColor = FLinearColor(0.02f, 0.05f, 0.15f, 1.0f);

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

	/**
	 * Updates all wave-related parameters on the dynamic material instance.
	 */
	void UpdateWaveParameters();

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