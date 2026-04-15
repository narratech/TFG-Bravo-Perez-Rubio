#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CosmicRingComponent.generated.h"

// [E: Delegado para notificar a Blueprints cuando se termina de generar un sector / I: Delegate to notify Blueprints when a sector generation finishes]
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAsteroidFieldGenerated);

UCLASS(ClassGroup = (CosmicArchitect), meta = (BlueprintSpawnableComponent))
class COSMICARCHITECTRUNTIME_API UCosmicRingComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UCosmicRingComponent();

protected:
	virtual void BeginPlay() override;

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

	// [E: Radio interno en Kilómetros (LWC) / I: Inner radius in Kilometers (LWC)]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double InnerRadiusKM = 70000.0;

	// [E: Radio externo en Kilómetros (LWC) / I: Outer radius in Kilometers (LWC)]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Dimensions")
	double OuterRadiusKM = 140000.0;

	// [E: Densidad de asteroides por sector / I: Asteroid density per sector]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | Generation")
	int32 LocalAsteroidDensity = 5000;

	// [E: Distancia límite para desvanecer el shader y generar mallas / I: Threshold distance to fade shader and spawn meshes]
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	double FadeMinDistanceKM = 500.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cosmic Architect | LOD")
	double FadeMaxDistanceKM = 1000.0;

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

	// [E: Método interno para gestionar la carga asíncrona / I: Internal method to handle async loading]
	void GenerateAsteroidsAsync(const FVector3d& PlayerLocation);
};