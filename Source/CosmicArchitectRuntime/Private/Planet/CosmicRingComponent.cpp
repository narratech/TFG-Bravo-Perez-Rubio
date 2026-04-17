#include "Planet/CosmicRingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

// [E: Constructor: Inicializamos escala, mallas y materiales automáticos / I: Constructor: We initialize scale, meshes and automatic materials]
UCosmicRingComponent::UCosmicRingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsGeneratingAsteroids = false;

	// [E: Crear componente visual para el plano macro / I: Create visual component for the macro plane]
	MacroDiskComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MacroDiskComponent"));
	MacroDiskComponent->SetupAttachment(this);
	MacroDiskComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MacroDiskComponent->SetCastShadow(false);

	// [E: Configurar escala basada en el radio KM: (KM * 100,000 cm) / 50 cm radio base = KM * 2000 / I: Setup scale based on KM radius]
	double InitialScale = OuterRadiusKM * 2000.0;
	MacroDiskComponent->SetRelativeScale3D(FVector(InitialScale, InitialScale, 1.0f));
	MacroDiskComponent->SetRelativeRotation(RingRotation);

	// [E: Asignación automática de la malla de plano del motor / I: Automatic Engine Plane mesh assignment]
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
	if (PlaneMeshAsset.Succeeded())
	{
		MacroDiskComponent->SetStaticMesh(PlaneMeshAsset.Object);
	}

	// [E: Asignación automática del material personalizado M_CosmicRing / I: Automatic custom material M_CosmicRing assignment]
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CosmicMaterialAsset(TEXT("/Script/Engine.Material'/CosmicArchitect/Resources/Materials/M_CosmicRing.M_CosmicRing'"));
	if (CosmicMaterialAsset.Succeeded())
	{
		MacroRingMaterial = CosmicMaterialAsset.Object;
	}

	// [E: Crear gestor HISM para los asteroides físicos / I: Create HISM manager for physical asteroids]
	AsteroidHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AsteroidHISM"));
	AsteroidHISM->SetupAttachment(this);
}

void UCosmicRingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AsteroidMesh)
	{
		AsteroidHISM->SetStaticMesh(AsteroidMesh);
	}

	// [E: Crear instancia dinámica basada en el material asignado / I: Create dynamic instance based on assigned material]
	if (MacroRingMaterial && MacroDiskComponent)
	{
		DynamicRingMat = UMaterialInstanceDynamic::Create(MacroRingMaterial, this);
		MacroDiskComponent->SetMaterial(0, DynamicRingMat);

		// [E: Actualizamos físicas visuales (escala y rotación) al empezar / I: Update visual physics (scale and rotation) on play]
		double CurrentScale = OuterRadiusKM * 2000.0;
		MacroDiskComponent->SetRelativeScale3D(FVector(CurrentScale, CurrentScale, 1.0f));
		MacroDiskComponent->SetRelativeRotation(RingRotation);

		UpdateShaderParameters();
	}
}

#if WITH_EDITOR
void UCosmicRingComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (MacroRingMaterial && MacroDiskComponent)
	{
		// [E: Asegurar que el material dinámico exista y esté aplicado en el Editor / I: Ensure dynamic material exists and is applied in the Editor]
		if (!DynamicRingMat)
		{
			DynamicRingMat = UMaterialInstanceDynamic::Create(MacroRingMaterial, this);
		}

		MacroDiskComponent->SetMaterial(0, DynamicRingMat);

		// [E: Forzar actualización de tamaño en base al slider de OuterRadiusKM / I: Force size update based on OuterRadiusKM slider]
		double CurrentScale = OuterRadiusKM * 2000.0;
		MacroDiskComponent->SetRelativeScale3D(FVector(CurrentScale, CurrentScale, 1.0f));
		MacroDiskComponent->SetRelativeRotation(RingRotation);

		UpdateShaderParameters();
	}
}
#endif

void UCosmicRingComponent::UpdateShaderParameters()
{
	if (DynamicRingMat)
	{
		// [E: Enviar parámetros al shader dinámico / I: Send parameters to the dynamic shader]
		DynamicRingMat->SetVectorParameterValue(FName("RingColor"), RingColor);
		DynamicRingMat->SetScalarParameterValue(FName("BandFrequency"), (float)BandFrequency);
		DynamicRingMat->SetScalarParameterValue(FName("InnerRadius"), (float)InnerRadiusUV);
		DynamicRingMat->SetScalarParameterValue(FName("OuterRadius"), (float)OuterRadiusUV);

		// [E: Conversión de Kilómetros a Unidades Unreal para el desvanecimiento / I: Kilometers to Unreal Units conversion for fading]
		DynamicRingMat->SetScalarParameterValue(FName("FadeMinDistance"), (float)(FadeMinDistanceKM * 100000.0));
		DynamicRingMat->SetScalarParameterValue(FName("FadeMaxDistance"), (float)(FadeMaxDistanceKM * 100000.0));
	}
}

void UCosmicRingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}