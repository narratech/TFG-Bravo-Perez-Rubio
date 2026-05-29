// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CosmicPlanet.generated.h"

class UCosmicClipmapComponent;
class UCosmicNoiseClass;
class UCosmicFoliageSpawner;
class UCosmicCollisionComponent;
class UCosmicOceanComponent;
class UCosmicFoliageCollection;
class UMaterialInstance;

/**
 * ACosmicPlanet
 * Actor principal que representa un cuerpo planetario procedural. 
 * Orquestra la generación de terreno mediante Clipmaps, simulación de océanos,
 * sistemas de colisiones dinámicas y distribución de follaje a gran escala.
 */
UCLASS(HideCategories = (
	Replication, Input, Actor, LOD, Activation, Cooking, Networking,
	Physics, Navigation, Tags, DataLayers, LevelInstance))
	class COSMICARCHITECTRUNTIME_API ACosmicPlanet : public AActor
{
	GENERATED_BODY()

public:

	/** Radio base del planeta en Kilómetros (soporta Large World Coordinates). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Planet")
	double RadiusKm = 1.0;

	/** Componente raíz de la jerarquía del actor. */
	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	USceneComponent* Root;

	/** Sistema de gestión de terreno basado en niveles de detalle concéntricos (Clipmap). */
	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	TObjectPtr<UCosmicClipmapComponent> ClipmapComponent;

	/** Gestiona la generación de mallas de colisión en tiempo real alrededor del observador. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	UCosmicCollisionComponent* CollisionComponent;

	/** Componente encargado de la representación visual y física del nivel del mar. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planet")
	UCosmicOceanComponent* OceanComponent;

	/** Asset que define los algoritmos de ruido para el relieve del terreno. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Planet|Noise")
	UCosmicNoiseClass* NoiseClass;

	/** Sistema de instanciación masiva de vegetación y rocas sobre la superficie. */
	UPROPERTY(VisibleAnywhere, Category = "Planet", BlueprintReadOnly)
	UCosmicFoliageSpawner* FoliageSpawnerComponent;

	// --- CONFIGURACIÓN DE COLORES DEL MATERIAL ---

	/** Color predominante para las zonas de altitud media. */
	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetMainColor1 = FColor::Red;

	/** Color secundario para variación cromática del terreno. */
	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetMainColor2 = FColor::Orange;

	/** Tono aplicado en zonas de baja temperatura o valles profundos. */
	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetColdColor = FColor::White;

	/** Tono aplicado en cimas o zonas de alta actividad/temperatura. */
	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetHotColor = FColor::Red;

	/** Color utilizado para resaltar pendientes pronunciadas y acantilados. */
	UPROPERTY(EditAnywhere, Category = "Materials|Color")
	FColor PlanetSlopeColor = FColor::Black;

	// --- ESCALAS DE RUIDO ---

	/** Detalle fino del terreno (Micro-relieve). */
	UPROPERTY(EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleSmall = 1.f;

	/** Detalle medio del terreno (Colinas y formaciones). */
	UPROPERTY(EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleMedium = 3.f;

	/** Detalle macro del terreno (Montañas y continentes). */
	UPROPERTY(EditAnywhere, Category = "Materials|Noise", meta = (ClampMin = "0.01"))
	float NoiseScaleLarge = 100.f;


	/** Inicializa los componentes por defecto y la estructura básica. */
	ACosmicPlanet();

#if WITH_EDITOR
	/** Lógica de construcción inicial para visualización en el Editor. */
	virtual void OnConstruction(const FTransform& Transform) override;
#endif

	/**
	 * Configuración completa del planeta.
	 * Se utiliza para inicializar todas las propiedades desde un mánager o Blueprint.
	 */
	void InitPlanet(
		float InRadiusKm,
		UCosmicNoiseClass* NewNoiseClass,
		FColor Color1, FColor Color2, FColor ColorCold, FColor ColorHot,
		FColor ColorSlope, float ScaleL, float ScaleM, float ScaleS,
		UMaterialInstance* BaseMaterial,
		UTexture2D* DefaultTexture,
		// Clipmap
		bool UseClipmap = true,
		int32 InBaseResolution = 128,
		int32 InNumLevels = 4,
		int32 InMinTriangleSize = 100,
		float InHeightVisibility = 5.0f,
		// Ocean
		bool  bInHasOcean = true,
		double InSeaLevelKm = 0.0,
		int32 InOceanResolution = 128,
		UMaterialInstance* InOceanMaterial = nullptr,
		// Foliage
		UCosmicFoliageCollection* InFoliageCollection = nullptr
	);

	/** Configura el comportamiento y radios de aparición del follaje procedural. */
	void SetFoliageParams(
		int32 InFoliageInstancesPerFrame = 50.f,
		float NearLayerRadiusKm = 0.05f,
		float MediumLayerRadiusKm = 0.2f,
		float FarLayerRadiusKm = 0.5f);

	/** Libera la memoria de los objetos de ruido si no son assets persistentes. */
	void CleanupNoiseSettings();

protected:
	/** Lógica de inicio al ejecutar el juego. */
	virtual void BeginPlay() override;

#if WITH_EDITOR
	/** Maneja la duplicación del actor en el editor asegurando que los componentes se regeneren. */
	virtual void PostDuplicate(EDuplicateMode::Type Mode) override;
#endif

	/** Limpieza al destruir el actor. */
	virtual void Destroyed() override;

	/** Fase inicial de destrucción del objeto. */
	virtual void BeginDestroy() override;

	/** Inicialización de datos tras la creación de todos los sub-componentes. */
	virtual void PostInitializeComponents() override;

	/** Finalización de la ejecución en el mundo. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Configura y lanza la generación del sistema de Clipmaps. */
	void InitClipmap();

	/** Fuerza la reconstrucción total de todos los sistemas planetarios. */
	void RebuildPlanet();

	/** Actualiza el sistema de ruido y sus delegados de notificación. */
	void UpdateNoiseSettings();

	/** Regenera el sistema de vegetación y rocas. */
	void UpdateFoliage();

	/** Sincroniza el componente oceánico con el radio actual del planeta. */
	void UpdateOcean();

	/** Actualiza únicamente los parámetros visuales del material en el terreno. */
	void UpdateMaterialOnly();

	/** Limpia colisiones y desvincula delegados activos. */
	void ClearData();

	/** Bandera interna para evitar reinicializaciones redundantes en el Editor. */
	bool bInitializedInEditor = false;

#if WITH_EDITOR
	/** Notificador de cambios en el panel de detalles para actualizaciones en tiempo real. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};