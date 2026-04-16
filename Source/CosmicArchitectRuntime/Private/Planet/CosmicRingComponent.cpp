#include "Planet/CosmicRingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "Materials/MaterialInstanceDynamic.h"

// [E: Constructor: Inicializamos la arquitectura de componentes / I: Constructor: We initialize the component architecture]
UCosmicRingComponent::UCosmicRingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsGeneratingAsteroids = false;

	// [E: Crear componente visual para el plano macro / I: Create visual component for the macro plane]
	MacroDiskComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MacroDiskComponent"));
	MacroDiskComponent->SetupAttachment(this);
	MacroDiskComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MacroDiskComponent->SetCastShadow(false); // [E: Sombras planetarias desactivadas por rendimiento / I: Planetary shadows disabled for performance]

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
		MacroDiskComponent->SetMaterial(0, MacroRingMaterial);
		DynamicRingMat = MacroDiskComponent->CreateAndSetMaterialInstanceDynamic(0);

		if (DynamicRingMat)
		{
			// [E: Inyectar variables visuales al shader / I: Inject visual variables to the shader]
			DynamicRingMat->SetVectorParameterValue(FName("RingColor"), RingColor);
			DynamicRingMat->SetScalarParameterValue(FName("BandFrequency"), BandFrequency);

			// [E: Usamos las variables UV para el Shader, ya que los KM romperían la escala 0-1 del TextureCoordinate / I: We use UV variables for the Shader, since KM would break the 0-1 TextureCoordinate scale]
			DynamicRingMat->SetScalarParameterValue(FName("InnerRadius"), InnerRadiusUV);
			DynamicRingMat->SetScalarParameterValue(FName("OuterRadius"), OuterRadiusUV);

			// [E: Inyectamos distancias de cámara en Centímetros (Unidades de Unreal) / I: Inject camera distances in Centimeters (Unreal Units)]
			DynamicRingMat->SetScalarParameterValue(FName("FadeMinDistance"), FadeMinDistanceKM * 100000.0);
			DynamicRingMat->SetScalarParameterValue(FName("FadeMaxDistance"), FadeMaxDistanceKM * 100000.0);
		}
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
		// [E: Aquí añadiríamos lógica para ver si el jugador se ha movido lo suficiente para requerir nuevos asteroides / I: Here we would add logic to see if player moved enough to require new asteroids]
		// GenerateAsteroidsAsync(CameraLocation);
	}
}

void UCosmicRingComponent::GenerateAsteroidsAsync(const FVector3d& PlayerLocation)
{
	bIsGeneratingAsteroids = true;

	// [E: Limpiar las rocas viejas en el hilo principal antes de calcular nuevas / I: Clear old rocks on main thread before calculating new ones]
	AsteroidHISM->ClearInstances();

	// [E: Lanzar hilo asíncrono para cálculos matemáticos pesados / I: Fire async thread for heavy mathematical calculations]
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, PlayerLocation]()
		{
			// [E: Array local para almacenar transformaciones (No toca el motor gráfico) / I: Local array to store transforms (Does not touch rendering engine)]
			TArray<FTransform> LocalTransforms;

			for (int32 i = 0; i < LocalAsteroidDensity; ++i)
			{
				// [E: Aquí irá nuestra matemática de distribución toroidal procedural / I: Here goes our procedural toroidal distribution math]
				FVector3d RandomOffset = FVector3d(FMath::RandRange(-50000.0, 50000.0), FMath::RandRange(-50000.0, 50000.0), FMath::RandRange(-2000.0, 2000.0));
				FVector3d AsteroidPos = PlayerLocation + RandomOffset;

				FTransform NewTransform;
				NewTransform.SetLocation(AsteroidPos);
				// [E: Generamos ángulos aleatorios de 0 a 360 para Pitch, Yaw y Roll / I: We generate random angles from 0 to 360 for Pitch, Yaw and Roll]
				FRotator RandomRot(FMath::RandRange(0.0, 360.0), FMath::RandRange(0.0, 360.0), FMath::RandRange(0.0, 360.0));

				// [E: Lo convertimos a cuaternión de doble precisión para la matriz LWC / I: Convert to double precision quaternion for the LWC matrix]
				NewTransform.SetRotation(RandomRot.Quaternion());
				NewTransform.SetScale3D(FVector3d(FMath::RandRange(1.0, 5.0)));

				LocalTransforms.Add(NewTransform);
			}

			// [E: Volver al hilo principal para inyectar las mallas al HISM / I: Return to main thread to inject meshes to HISM]
			AsyncTask(ENamedThreads::GameThread, [this, LocalTransforms]()
				{
					// [E: Inserción en lote (Batch) por rendimiento / I: Batch insertion for performance]
					AsteroidHISM->AddInstances(LocalTransforms, false);

					// [E: Liberar el candado asíncrono / I: Release async lock]
					bIsGeneratingAsteroids = false;
				});
		});
}