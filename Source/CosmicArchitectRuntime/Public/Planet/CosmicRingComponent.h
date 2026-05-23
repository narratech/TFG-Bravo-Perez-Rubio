#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CosmicRingComponent.generated.h"

/**
 * Delegado para notificar a sistemas externos o Blueprints cuando un
 * sector de asteroides ha finalizado su generación asíncrona.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAsteroidFieldGenerated);

/**
 * UCosmicRingComponent
 *
 * Gestiona la representación visual y física de anillos planetarios a escala real.
 * Implementa un sistema de "Treadmill" (cinta de correr) que segmenta el anillo en sectores
 * angulares, cargando y reciclando instancias de asteroides (HISM) dinámicamente según
 * la proximidad del observador para optimizar el rendimiento y la memoria.
 */
UCLASS(ClassGroup = (CosmicArchitect), meta = (BlueprintSpawnableComponent),
	HideCategories = (Rendering, Lighting, Navigation, Replication, Physics, Collision,
		Activation, AssetUserData, HLOD, Cooking, Tags, ComponentReplication))
	class COSMICARCHITECTRUNTIME_API UCosmicRingComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/** Inicialización de subobjetos y configuración por defecto del componente. */
	UCosmicRingComponent();

protected:
	/** Inicializa el estado dinámico del material y transformaciones al inicio de la simulación. */
	virtual void BeginPlay() override;

#if WITH_EDITOR
	/** Actualiza las propiedades visuales en el viewport del editor al modificar parámetros. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	/** Gestiona el ciclo de vida de los sectores y la detección de la cámara en cada frame. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Configura el componente tras su registro en el mundo, estableciendo escalas y materiales iniciales. */
	virtual void OnRegister() override;

	/** Garantiza que el componente mantenga su posición relativa neutral respecto al padre. */
	virtual void OnAttachmentChanged() override;

	/** Limpieza de memoria y destrucción de componentes instanciados antes de eliminar el objeto. */
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	// --- PROPIEDADES DE DISEÑO ---

	/** Interfaz del material base para el disco macro (Shader LWC). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	class UMaterialInterface* MacroRingMaterial;

	/** Malla estática utilizada para representar cada asteroide individual en los sectores. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	class UStaticMesh* AsteroidMesh;

	/** Color base para el polvo y los asteroides del anillo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	FLinearColor RingColor = FLinearColor(0.2f, 0.3f, 1.0f, 1.0f);

	/** Frecuencia de las bandas de ruido en el material procedural. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	double BandFrequency = 50.0;

	// --- DIMENSIONES ---

	/** Radio interno real del anillo en Kilómetros. También determina la máscara UV interna del shader. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double InnerRadiusKM = 2.0;

	/** Radio externo real del anillo en Kilómetros. Determina la escala del disco macro y la máscara UV externa. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double OuterRadiusKM = 5.0;

	/** Espesor total vertical del anillo en Kilómetros. Controla la dispersión Z de los asteroides y la detección de proximidad. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double RingThicknessKM = 0.4;

	/** Rotación orbital del sistema de anillos. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	FRotator RingRotation = FRotator::ZeroRotator;

	// --- LOD ---

	/** Tamaño mínimo aleatorio para las instancias de asteroides. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	float MinScale = 0.01f;

	/** Tamaño máximo aleatorio para las instancias de asteroides. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	float MaxScale = 0.05f;

	/**
	 * Distancia en KM desde el punto más cercano del anillo (borde o superficie) a la que
	 * los asteroides 3D comienzan a generarse. Se evalúa sobre la geometría real del anillo
	 * (annulus + espesor), no desde el centro del componente.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	double AsteroidActivationDistanceKM = 8.0;

	/** Distancia en KM donde comienza el desvanecimiento del shader macro. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	double FadeMinDistanceKM = 1.0;

	/** Distancia en KM donde el shader macro se oculta totalmente. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	double FadeMaxDistanceKM = 8.0;

	// --- OPTIMIZACIÓN Y RENDIMIENTO ---

	/** Amplitud angular de cada sector generado (en grados). */
	UPROPERTY(EditAnywhere, Category = "Cosmic Architect | Optimization")
	float SectorAngleDegrees = 15.0f;

	/** Cantidad de sectores adyacentes a la posición del observador que se mantienen activos. */
	UPROPERTY(EditAnywhere, Category = "Cosmic Architect | Optimization")
	int32 VisibleSectors = 2;

	/** Cantidad de asteroides individuales a generar por cada sector activo. */
	UPROPERTY(EditAnywhere, Category = "Cosmic Architect | Optimization")
	int32 AsteroidsPerSector = 500;

	/**
	 * Límite de instancias (asteroides) procesadas por segundo entre creación y destrucción de sectores.
	 * Un sector en curso siempre se completa aunque se supere el límite en ese frame.
	 * Los sectores pendientes se procesan en el frame siguiente.
	 */
	UPROPERTY(EditAnywhere, Category = "Cosmic Architect | Optimization")
	int32 MaxInstancesPerSecond = 500;

private:
	/** Componente que renderiza el material macro del anillo. */
	UPROPERTY(VisibleAnywhere, Category = "Cosmic Architect | Internal")
	class UStaticMeshComponent* MacroDiskComponent;

	/** Puntero a la instancia dinámica para manipular parámetros de shader en tiempo real. */
	UPROPERTY()
	class UMaterialInstanceDynamic* DynamicRingMat;

	/** Mapa que asocia IDs de sector con sus respectivos componentes HISM activos. */
	UPROPERTY()
	TMap<int32, UHierarchicalInstancedStaticMeshComponent*> ActiveSectors;

	/** Repositorio de componentes HISM inactivos para su reutilización inmediata (Pooling). */
	UPROPERTY()
	TArray<UHierarchicalInstancedStaticMeshComponent*> HISMPool;

	/** Recupera un componente HISM del pool o crea uno nuevo si no hay disponibles. */
	UHierarchicalInstancedStaticMeshComponent* GetOrCreateHISM();

	/**
	 * Sincroniza los valores de las propiedades C++ con los parámetros del Material Instance.
	 * Los radios UV se calculan automáticamente a partir de InnerRadiusKM y OuterRadiusKM.
	 */
	void UpdateShaderParameters();

	/**
	 * Invalida todos los sectores activos devolviéndolos al pool para forzar su regeneración
	 * en el siguiente tick. Se llama cuando propiedades que afectan la geometría cambian en el editor.
	 */
	void InvalidateAllSectors();

	/**
	 * Calcula la distancia en centímetros desde una posición (en espacio local del componente)
	 * hasta el punto más cercano del volumen del anillo (annulus + espesor vertical).
	 */
	float ComputeDistanceToRing(const FVector& LocalPosition) const;
};