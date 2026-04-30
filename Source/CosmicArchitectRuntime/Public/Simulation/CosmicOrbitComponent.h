// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CosmicOrbitComponent.generated.h"

// E: Componente que maneja el movimiento orbital y la rotación de un cuerpo celeste.
// I: Component that handles the orbital movement and rotation of a celestial body.
UCLASS(ClassGroup = (Cosmic), meta = (BlueprintSpawnableComponent))
class COSMICARCHITECTRUNTIME_API UCosmicOrbitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// E: Constructor: Establece los valores predeterminados de las propiedades de este componente.
	// I: Constructor: Sets default values for this component's properties.
	UCosmicOrbitComponent();

#if WITH_EDITOR
	// E: Se llama cuando cambia una propiedad en el editor (útil para actualizar visualizaciones).
	// I: Called when a property is changed in the editor (useful for updating visualizations).
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	// E: Se ejecuta cuando el juego comienza o se genera el actor.
	// I: Executed when the game starts or the actor is spawned.
	virtual void BeginPlay() override;

public:
	// E: Función llamada cada frame para actualizar la posición y rotación del actor.
	// I: Function called every frame to update the actor's position and rotation.
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// E: Referencia al cuerpo central alrededor del cual orbita este actor.
	// I: Reference to the central body around which this actor orbits.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit State")
	AActor* ParentBody;

	// E: Tiempo actual transcurrido en la simulación de la órbita.
	// I: Current time elapsed in the orbit simulation.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Orbit State")
	float CurrentOrbitTime = 0.0f;

	// E: Semieje mayor de la órbita en kilómetros (define el tamaño general de la órbita).
	// I: Semi-major axis of the orbit in kilometers (defines the overall size of the orbit).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params")
	float SemiMajorAxisKm = 1.0f;

	// E: Excentricidad de la órbita (0 = círculo perfecto, >0 a <1 = elipse).
	// I: Eccentricity of the orbit (0 = perfect circle, >0 to <1 = ellipse).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "1"))
	float Eccentricity = 0.0f; // 0 círculo, 0.99 elipse extrema

	// E: Posición inicial en la órbita como fracción del periodo (0 a 1).
	// I: Initial position in the orbit as a fraction of the period (0 to 1).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "1"))
	float InitialPosition = 0.0f;

	// E: Tiempo en segundos que tarda en completar una órbita entera.
	// I: Time in seconds it takes to complete a full orbit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params")
	float OrbitalPeriod = 10.0f;

	// E: Inclinación de la órbita sobre el eje X (en grados).
	// I: Inclination of the orbit on the X axis (in degrees).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "360"))
	float InclinationX = 0.0f;

	// E: Inclinación de la órbita sobre el eje Y (en grados).
	// I: Inclination of the orbit on the Y axis (in degrees).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "360"))
	float InclinationY = 0.0f;

	// E: Inclinación de la órbita sobre el eje Z (en grados).
	// I: Inclination of the orbit on the Z axis (in degrees).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Orbit Params", meta = (ClampMin = "0", ClampMax = "360"))
	float InclinationZ = 0.0f;

	// E: Velocidad a la que el objeto rota sobre su propio eje Z (grados por segundo).
	// I: Speed at which the object rotates around its own Z axis (degrees per second).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	float SpinSpeed = 0.0f; // Grados por segundo en el eje Z (Yaw)

	// E: Indica si el generador está simulando órbitas en el editor.
    // I: Indicates whether the generator is simulating orbits in the editor.
	UPROPERTY()
	bool bEditorSimulating = false;

	// E: Multiplicador de velocidad orbital inyectado desde el generador.
	// I: Orbital speed multiplier injected from the generator.
	UPROPERTY()
	float EditorSpeedMultiplier = 1.0f;

	// E: Inicializa los parámetros visuales básicos de la órbita (color, grosor).
	// I: Initializes the basic visual parameters of the orbit (color, thickness).
	void InitOrbit(FColor color = FColor::Cyan);

protected:
	// E: Bandera interna que indica si el componente se está previsualizando en el editor.
	// I: Internal flag indicating if the component is being previewed in the editor.
	bool bIsInEditorPreview = false;

	// E: Calcula y aplica la posición inicial en la órbita basándose en InitialPosition.
	// I: Calculates and applies the starting position in the orbit based on InitialPosition.
	void UpdateInitialOrbitPosition();

private:
	// E: Función para generar/actualizar el dibujo (Debug Lines) de la órbita en el editor.
	// I: Function to generate/update the drawing (Debug Lines) of the orbit in the editor.
	void UpdateOrbitVisualization();

	// E: Número de segmentos o líneas rectas usadas para dibujar la forma de la órbita.
	// I: Number of segments or straight lines used to draw the shape of the orbit.
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization", meta = (ClampMin = "8", ClampMax = "360"))
	int32 OrbitSegments = 72;

	// E: Color de la línea de la órbita al visualizarse en el editor.
	// I: Color of the orbit line when visualized in the editor.
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization")
	FColor OrbitColor = FColor::White;

	// E: Grosor de las líneas dibujadas para representar la órbita.
	// I: Thickness of the lines drawn to represent the orbit.
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization", meta = (ClampMin = "0.1", ClampMax = "100000"))
	float OrbitThickness = 5000.0f;

	// E: Define si se debe dibujar o no la órbita en el visor del editor.
	// I: Defines whether or not the orbit should be drawn in the editor viewport.
	UPROPERTY(EditAnywhere, Category = "Orbit Visualization")
	bool bShowOrbitInEditor = true;
};