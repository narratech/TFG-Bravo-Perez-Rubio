// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CosmicOceanClipmap.generated.h"

class UCosmicMeshComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UCosmicOceanClipmap : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCosmicOceanClipmap();

	void CreateLevels();
	void CreatePerformanceLevel(bool bActive);

	void ClearLevels();
	void ReasignLevels();
	void SetMaterialData(FColor color1, FColor color2, FColor colorHeight, float scale);
	USceneComponent* ParentRoot = nullptr;

	float PlanetRadius = 100.f;

	bool bInitializedInEditor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* BaseMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	UTexture2D* DefaultTexture;


	UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "8", ClampMax = "256"))
	int32 BaseResolution = 128;

	UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "1", ClampMax = "20"))
	int32 NumLevels = 4;

	UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "50"))
	float MinTriangleSize = 200.f;

	UPROPERTY(VisibleAnywhere, Category = "Clipmap")
	float BaseGridSpacing = 200.f;

	UPROPERTY(EditAnywhere, Category = "Clipmap")
	float HeightVisibility = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "0", ClampMax = "60"))
	float TimeToRefresh = 0.033f;

	UPROPERTY(EditAnywhere, Category = "Clipmap")
	bool FreezeGeneration = false;
protected:
	TArray<UCosmicMeshComponent*> Levels;
	UCosmicMeshComponent* FarLevel;

	float ElapsedTime = 0;
	float TimeToRefreshActive;
	bool bPerformaceMode = false;
	bool bInit = false;
	bool bPerformanceBuild = false;

	FColor PlanetMainColor1 = FColor::Green;
	FColor PlanetMainColor2 = FColor::Red;
	FColor PlanetAltitudeColor = FColor::Yellow;
	// Called when the game starts
	virtual void BeginPlay() override;
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void UpdatePatchTransform(const FVector& SurfacePos, const FVector& N);
	float GetDistanceToSurface(FVector& SurfacePos, FVector& N);
	FVector GetPlayerLocation();
	bool IsClipmapRingVisible(const int32 LevelIndex, const float DistanceToSurface);
	bool IsClipmapRingVisible(const float GridSpacing, const int32 Resolution, const float DistanceToSurface);
	void ReduceClimapLevel();
	void IncreaseClipmapLevel();
private:	
	UPROPERTY(EditAnywhere, Category = "Materials")
	UMaterialInstanceDynamic* DynamicPlanetMat;
	
};
