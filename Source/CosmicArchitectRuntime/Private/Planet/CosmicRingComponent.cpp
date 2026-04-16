#include "Planet/CosmicRingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

// [E: Constructor: Inicializamos la arquitectura y la malla automática / I: Constructor: We initialize the architecture and automatic mesh]
UCosmicRingComponent::UCosmicRingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsGeneratingAsteroids = false;

	// [E: Crear componente visual para el plano macro / I: Create visual component for the macro plane]
	MacroDiskComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MacroDiskComponent"));
	MacroDiskComponent->SetupAttachment(this);
	MacroDiskComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MacroDiskComponent->SetCastShadow(false);

	// --- [E: ASIGNACIÓN AUTOMÁTICA DEL PLANO / I: AUTOMATIC PLANE ASSIGNMENT] ---
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
	if (PlaneMeshAsset.Succeeded())
	{
		MacroDiskComponent->SetStaticMesh(PlaneMeshAsset.Object);
	}

	// [E: Crear gestor HISM para instanciar miles de rocas de forma barata / I: Create HISM manager to instantiate thousands of rocks cheaply]
	AsteroidHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AsteroidHISM"));
	AsteroidHISM->SetupAttachment(this);
	AsteroidHISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void UCosmicRingComponent::BeginPlay()
{
	Super::BeginPlay();

	// [E: Configurar la malla del asteroide en el HISM / I: Setup asteroid mesh on the HISM]
	if (AsteroidMesh)
	{
		AsteroidHISM->SetStaticMesh(AsteroidMesh);
	}

	// [E: Instanciar el material del shader para controlarlo en C++ / I: Instantiate the shader material to control it in C++]
	if (MacroRingMaterial && MacroDiskComponent)
	{
		// [E: Forzamos la creación del dinámico desde nuestra variable, no desde el slot vacío / I: Force dynamic creation from our variable, not from empty slot]
		DynamicRingMat = UMaterialInstanceDynamic::Create(MacroRingMaterial, this);
		MacroDiskComponent->SetMaterial(0, DynamicRingMat);

		UpdateShaderParameters();
	}
}

// [E: Esta función permite que los cambios se vean en el Editor al mover sliders / I: This function allows changes to be seen in Editor when moving sliders]
#if WITH_EDITOR
void UCosmicRingComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (MacroRingMaterial && MacroDiskComponent)
	{
		// [E: Si el material dinámico no existe aún en el editor, lo creamos / I: If dynamic material doesn't exist yet in editor, create it]
		if (!DynamicRingMat)
		{
			DynamicRingMat = UMaterialInstanceDynamic::Create(MacroRingMaterial, this);
		}

		// [E: Aseguramos que el Element 0 use nuestro material dinámico / I: Ensure Element 0 uses our dynamic material]
		MacroDiskComponent->SetMaterial(0, DynamicRingMat);

		UpdateShaderParameters();
	}
}
#endif

void UCosmicRingComponent::UpdateShaderParameters()
{
	if (DynamicRingMat)
	{
		// [E: Inyectar parámetros respetando los nombres de tu Shader / I: Inject parameters respecting your Shader names]
		DynamicRingMat->SetVectorParameterValue(FName("RingColor"), RingColor);
		DynamicRingMat->SetScalarParameterValue(FName("BandFrequency"), (float)BandFrequency);
		DynamicRingMat->SetScalarParameterValue(FName("InnerRadius"), (float)InnerRadiusUV);
		DynamicRingMat->SetScalarParameterValue(FName("OuterRadius"), (float)OuterRadiusUV);

		// [E: Conversión de KM a Unidades Unreal (cm) para el desvanecimiento / I: KM to Unreal Units (cm) conversion for fading]
		DynamicRingMat->SetScalarParameterValue(FName("FadeMinDistance"), (float)(FadeMinDistanceKM * 100000.0));
		DynamicRingMat->SetScalarParameterValue(FName("FadeMaxDistance"), (float)(FadeMaxDistanceKM * 100000.0));
	}
}

void UCosmicRingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// [E: Obtener la cámara del jugador (LWC seguro en UE5) / I: Get player camera (LWC safe in UE5)]
	APlayerCameraManager* CamManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
	if (!CamManager) return;

	FVector3d CameraLocation = CamManager->GetCameraLocation();
	FVector3d RingLocation = GetComponentLocation();

	// [E: Calcular distancia euclidiana en LWC (doubles) / I: Calculate Euclidean distance in LWC (doubles)]
	double DistanceToRingCenter = FVector3d::Distance(CameraLocation, RingLocation);

	// [E: Lógica de la "Ventana Local": Si estamos cerca y no estamos generando, poblamos / I: "Local Window" logic: If close and not generating, populate]
	if (DistanceToRingCenter <= (OuterRadiusKM * 100000.0) && !bIsGeneratingAsteroids)
	{
		// GenerateAsteroidsAsync(CameraLocation);
	}
}

void UCosmicRingComponent::GenerateAsteroidsAsync(const FVector3d& PlayerLocation)
{
	bIsGeneratingAsteroids = true;
	AsteroidHISM->ClearInstances();

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, PlayerLocation]()
		{
			TArray<FTransform> LocalTransforms;

			for (int32 i = 0; i < LocalAsteroidDensity; ++i)
			{
				FVector3d RandomOffset = FVector3d(FMath::RandRange(-50000.0, 50000.0), FMath::RandRange(-50000.0, 50000.0), FMath::RandRange(-2000.0, 2000.0));
				FVector3d AsteroidPos = PlayerLocation + RandomOffset;

				FTransform NewTransform;
				NewTransform.SetLocation(AsteroidPos);
				FRotator RandomRot(FMath::RandRange(0.0, 360.0), FMath::RandRange(0.0, 360.0), FMath::RandRange(0.0, 360.0));
				NewTransform.SetRotation(RandomRot.Quaternion());
				NewTransform.SetScale3D(FVector3d(FMath::RandRange(1.0, 5.0)));

				LocalTransforms.Add(NewTransform);
			}

			AsyncTask(ENamedThreads::GameThread, [this, LocalTransforms]()
				{
					AsteroidHISM->AddInstances(LocalTransforms, false);
					bIsGeneratingAsteroids = false;
				});
		});
}