// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicOceanComponent.generated.h"

class UCosmicMeshComponent;
class UMaterialInstance;
class UMaterialInstanceDynamic;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
	HideCategories = (Activation, Tags, AssetUserData, Navigation, Rendering, Replication, Input, Actor, Collision, Cooking))
class COSMICARCHITECTRUNTIME_API UCosmicOceanComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCosmicOceanComponent();

	void InitOcean(double PlanetRadiusKm, USceneComponent* Parent);

	void RegenerateOcean();

	void ClearOcean();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	bool bHasOcean = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean"))
	double SeaLevelKm = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean", ClampMin = "8", ClampMax = "256"))
	int32 OceanResolution = 128;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean", meta = (EditCondition = "bHasOcean"))
	UMaterialInstance* OceanMaterial;

protected:

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicOceanMat;

	UCosmicMeshComponent* OceanMesh;

	USceneComponent* ParentRoot;

	double PlanetRadiusCm;

	bool bInit = false;

	void BuildDynamicMaterial();

#if WITH_EDITOR
	// Se llama automáticamente cuando cambias algo en el panel de Detalles
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;		
};
