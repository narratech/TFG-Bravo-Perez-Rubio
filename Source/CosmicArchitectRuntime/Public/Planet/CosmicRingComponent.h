#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CosmicRingComponent.generated.h"

// [E: Delegado para notificar a Blueprints cuando se termina de generar un sector / I: Delegate to notify Blueprints when a sector generation finishes]
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAsteroidFieldGenerated);

UCLASS(ClassGroup = (CosmicArchitect), meta = (BlueprintSpawnableComponent),
	HideCategories = (Rendering, Lighting, Navigation, Replication, Physics, Collision,
		Activation, AssetUserData, HLOD, Cooking, Tags, ComponentReplication))
class COSMICARCHITECTRUNTIME_API UCosmicRingComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UCosmicRingComponent();

protected:
	virtual void BeginPlay() override;

	// [E: Lógica para actualizar el material en tiempo real dentro del Editor / I: Logic to update material in real-time inside the Editor]
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// -------------------------------------------------------------------------
	// [E: PROPIEDADES DE DISEÑO (Expuestas al Usuario) / I: DESIGN PROPERTIES (Exposed to User)]
	// -------------------------------------------------------------------------

	// [E: Material Macro procedural (Nuestro Shader) / I: Procedural Macro Material (Our Shader)]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	class UMaterialInterface* MacroRingMaterial;

	// [E: Malla 3D del asteroide para el HISM / I: 3D Asteroid mesh for the HISM]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	class UStaticMesh* AsteroidMesh;

	// [E: Color principal del polvo estelar / I: Main color of the stardust]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	FLinearColor RingColor = FLinearColor(0.2f, 0.3f, 1.0f, 1.0f);

	// [E: Frecuencia (cantidad) de las bandas del anillo / I: Frequency (amount) of the ring bands]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Visuals")
	double BandFrequency = 50.0;

	// [E: Radio interno en UV (0.0 a 0.5) / I: Inner radius in UV (0.0 to 0.5)]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double InnerRadiusUV = 0.2;

	// [E: Radio externo en UV (0.0 a 0.5) / I: Outer radius in UV (0.0 to 0.5)]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double OuterRadiusUV = 0.45;

	// [E: Radio interno en Kilómetros (LWC) / I: Inner radius in Kilometers (LWC)]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double InnerRadiusKM = 2.0;

	// [E: Radio externo en Kilómetros (LWC) / I: Outer radius in Kilometers (LWC)]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double OuterRadiusKM = 5.0;

	// [E: Rotación del plano del anillo / I: Ring plane rotation]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	FRotator RingRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	int32 NumAsteroids = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	float MinScale = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	float MaxScale = 10;

	// [E: Distancia límite para desvanecer el shader (KM) / I: Threshold distance to fade shader (KM)]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	double FadeMinDistanceKM = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	double FadeMaxDistanceKM = 15.0;

	// -------------------------------------------------------------------------
	// [E: COMPONENTES INTERNOS / I: INTERNAL COMPONENTS]
	// -------------------------------------------------------------------------
private:
	// [E: Plano visual para el material LWC / I: Visual plane for the LWC material]
	UPROPERTY(VisibleAnywhere, Category = "Cosmic Architect | Internal")
	class UStaticMeshComponent* MacroDiskComponent;

	// [E: Gestor de instancias para rendimiento extremo / I: Instance manager for extreme performance]
	UPROPERTY(VisibleAnywhere, Category = "Cosmic Architect | Internal")
	UHierarchicalInstancedStaticMeshComponent* AsteroidHISM;

	// [E: Referencia al material dinámico para modificar parámetros / I: Reference to dynamic material to modify parameters]
	UPROPERTY()
	class UMaterialInstanceDynamic* DynamicRingMat;

	// [E: Bandera para evitar múltiples llamadas asíncronas simultáneas / I: Flag to prevent multiple simultaneous async calls]
	bool bIsGeneratingAsteroids;

	// [E: Función auxiliar para inyectar datos al Shader / I: Helper function to inject data into Shader]
	void UpdateShaderParameters();
};