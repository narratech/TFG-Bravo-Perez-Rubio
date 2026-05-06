// Fill out your copyright notice in the Description page of Project Settings.


#include "Benchmark/BenchMarkConfig.h"
#include "Benchmark/BenchmarkManager.h"
#include "CosmicFoliageCollection.h"

// Sets default values
ABenchMarkConfig::ABenchMarkConfig()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ABenchMarkConfig::BeginPlay()
{
	Super::BeginPlay();

	UBenchmarkManager* BenchMarkManager = UBenchmarkManager::Get(GetWorld());

	if (BenchMarkManager)
	{
		BenchMarkManager->InitializeAssets(BaseMaterial, MoonMaterial, OceanMaterial,
			StarMaterial, GasGiantMaterial, RingMaterial, NoiseClass, FoliageCollection);
	}
	
}



