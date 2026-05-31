
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicOrbitComponent.generated.h"

/**
 * Componente responsable de simular movimiento orbital
 * y rotación axial para cuerpos celestes.
 *
 * El sistema implementa:
 * - Órbitas elípticas básicas
 * - Rotación local del actor
 * - Inclinación orbital tridimensional
 * - Visualización debug en editor
 *
 * La órbita se calcula utilizando una aproximación
 * kepleriana basada en anomalía media y anomalía excéntrica.
 */
UCLASS(ClassGroup = (Cosmic), meta = (BlueprintSpawnableComponent),
	HideCategories = (Navigation, Replication, Activation, AssetUserData, Cooking, Tags))
	class COSMICARCHITECTRUNTIME_API UCosmicOrbitComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	// ============================================================
	// CONSTRUCTION
	// ============================================================

	/**
	 * Inicializa el componente orbital con valores por defecto.
	 *
	 * Configura:
	 * - Tick en tiempo real
	 * - Simulación en editor
	 * - Estado inicial orbital
	 */
	UCosmicOrbitComponent();

#if WITH_EDITOR

	/**
	 * Responde a cambios de propiedades desde el editor.
	 *
	 * Actualiza automáticamente:
	 * - Posición orbital inicial
	 * - Visualización debug
	 * - Relaciones de attachment
	 *
	 * @param PropertyChangedEvent Información sobre la propiedad modificada.
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

protected:

	// ============================================================
	// ENGINE LIFECYCLE
	// ============================================================

	/**
	 * Inicializa el estado orbital al comenzar la simulación.
	 */
	virtual void BeginPlay() override;

public:

	// ============================================================
	// RUNTIME UPDATE
	// ============================================================

	/**
	 * Actualiza la simulación orbital cada frame.
	 *
	 * Responsabilidades:
	 * - Integración temporal orbital
	 * - Resolución de anomalía excéntrica
	 * - Actualización de posición relativa
	 * - Rotación axial del actor
	 * - Visualización debug en editor
	 *
	 * @param DeltaTime Tiempo transcurrido desde el frame anterior.
	 * @param TickType Tipo de actualización actual.
	 * @param ThisTickFunction Información del tick actual.
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ============================================================
	// ORBIT STATE
	// ============================================================

	/**
	 * Cuerpo central alrededor del cual orbita este actor.
	 *
	 * El actor propietario se moverá utilizando coordenadas
	 * relativas respecto a este cuerpo padre.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit State")
	AActor* ParentBody;

	/**
	 * Tiempo orbital acumulado actual.
	 *
	 * Expresado en segundos simulados.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit State")
	float CurrentOrbitTime = 0.0f;

	// ============================================================
	// ORBIT PARAMETERS
	// ============================================================

	/**
	 * Semieje mayor de la órbita.
	 *
	 * Define el tamaño general de la trayectoria orbital.
	 *
	 * Unidad: kilómetros.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params")
	float SemiMajorAxisKm = 1.0f;

	/**
	 * Excentricidad orbital.
	 *
	 * Valores:
	 * - 0.0  -> órbita circular
	 * - 0-1  -> órbita elíptica
	 *
	 * Valores cercanos a 1 producen órbitas extremadamente elongadas.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "1"))
	float Eccentricity = 0.0f;

	/**
	 * Posición inicial sobre la órbita.
	 *
	 * Representa una fracción normalizada del periodo orbital.
	 *
	 * Rango:
	 * - 0.0 -> inicio de órbita
	 * - 1.0 -> órbita completa
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "1"))
	float InitialPosition = 0.0f;

	/**
	 * Tiempo necesario para completar una órbita completa.
	 *
	 * Unidad: segundos.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params")
	float OrbitalPeriod = 10.0f;

	/**
	 * Inclinación orbital sobre el eje X.
	 *
	 * Unidad: grados.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "360"))
	float InclinationX = 0.0f;

	/**
	 * Inclinación orbital sobre el eje Y.
	 *
	 * Unidad: grados.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "360"))
	float InclinationY = 0.0f;

	/**
	 * Inclinación orbital sobre el eje Z.
	 *
	 * Unidad: grados.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "360"))
	float InclinationZ = 0.0f;

	// ============================================================
	// ROTATION
	// ============================================================

	/**
	 * Velocidad de rotación axial del actor.
	 *
	 * Controla la rotación local aplicada sobre el eje yaw.
	 *
	 * Unidad: grados por segundo.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	float SpinSpeed = 0.0f;

	// ============================================================
	// EDITOR STATE
	// ============================================================

	/**
	 * Indica si el sistema orbital está siendo simulado en editor.
	 */
	UPROPERTY()
	bool bEditorSimulating = false;

	/**
	 * Multiplicador global de velocidad orbital.
	 *
	 * Utilizado principalmente para previsualización
	 * y control temporal desde herramientas editoriales.
	 */
	UPROPERTY()
	float EditorSpeedMultiplier = 1.0f;

	// ============================================================
	// INITIALIZATION
	// ============================================================

	/**
	 * Inicializa los parámetros visuales básicos de la órbita.
	 *
	 * @param color Color utilizado para la visualización debug.
	 */
	void InitOrbit(FColor color = FColor::Cyan);

protected:

	// ============================================================
	// INTERNAL STATE
	// ============================================================

	/**
	 * Indica si el componente está siendo previsualizado en editor.
	 */
	bool bIsInEditorPreview = false;

	/**
	 * Calcula y aplica la posición orbital inicial.
	 *
	 * La posición se obtiene utilizando:
	 * - periodo orbital
	 * - excentricidad
	 * - posición inicial normalizada
	 */
	void UpdateInitialOrbitPosition();

private:

	// ============================================================
	// ORBIT VISUALIZATION
	// ============================================================

	/**
	 * Genera la representación visual debug de la órbita.
	 *
	 * La órbita se dibuja mediante segmentos lineales
	 * aproximando una trayectoria elíptica.
	 *
	 * @note Solo disponible en editor.
	 */
	void UpdateOrbitVisualization();

	/**
	 * Número de segmentos utilizados para aproximar la órbita.
	 *
	 * Valores elevados producen órbitas visualmente más suaves
	 * a costa de mayor coste de debug rendering.
	 */
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization", meta = (ClampMin = "8", ClampMax = "360"))
	int32 OrbitSegments = 72;

	/**
	 * Color utilizado para representar la órbita en editor.
	 */
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization")
	FColor OrbitColor = FColor::White;

	/**
	 * Grosor visual de las líneas debug orbitales.
	 */
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization", meta = (ClampMin = "0.1", ClampMax = "100000"))
	float OrbitThickness = 5000.0f;

	/**
	 * Activa o desactiva la visualización orbital en editor.
	 */
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization")
	bool bShowOrbitInEditor = true;
};
