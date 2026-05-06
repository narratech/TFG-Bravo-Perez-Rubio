// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BenchMarkConfig.generated.h"

class UCosmicNoiseClass;
class UCosmicFoliageCollection;

UCLASS(ClassGroup = (CosmicArchitect), meta = (BlueprintSpawnableComponent),
    HideCategories = (Rendering, Lighting, Navigation, Replication, Physics, Collision,
        Activation, AssetUserData, HLOD, Cooking, Tags, ComponentReplication))
class COSMICARCHITECTRUNTIME_API ABenchMarkConfig : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABenchMarkConfig();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    UMaterialInstance* BaseMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    UMaterialInstance* MoonMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    UMaterialInstance* OceanMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    UMaterialInstance* StarMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    UMaterialInstance* GasGiantMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    UMaterialInstance* RingMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    UCosmicNoiseClass* NoiseClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    UCosmicFoliageCollection* FoliageCollection;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
