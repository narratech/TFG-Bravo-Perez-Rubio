#include "Planet/CosmicRingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#endif

// [E: Constructor: Inicializamos escala, mallas y materiales automáticos / I: Constructor: We initialize scale, meshes and automatic materials]
UCosmicRingComponent::UCosmicRingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
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

void UCosmicRingComponent::OnRegister()
{
	Super::OnRegister();

	// [E: Validamos que tengamos malla y material base asignados / I: Validate we have mesh and base material assigned]
	if (MacroDiskComponent && MacroRingMaterial)
	{
		// [E: Si el material dinámico no existe, lo creamos / I: If dynamic material doesn't exist, create it]
		if (!DynamicRingMat)
		{
			DynamicRingMat = UMaterialInstanceDynamic::Create(MacroRingMaterial, this);
		}

		// [E: Asignamos el material a la malla / I: Assign material to the mesh]
		MacroDiskComponent->SetMaterial(0, DynamicRingMat);

		// [E: Aplicamos escala inicial y parámetros del shader / I: Apply initial scale and shader parameters]
		double CurrentScale = OuterRadiusKM * 2000.0;
		MacroDiskComponent->SetRelativeScale3D(FVector(CurrentScale, CurrentScale, 1.0f));
		// MacroDiskComponent->SetRelativeRotation(RingRotation); // Descomenta si usas rotación inicial

		UpdateShaderParameters();
	}

	if (AsteroidHISM && AsteroidMesh)
	{
		AsteroidHISM->SetStaticMesh(AsteroidMesh);
	}
}

void UCosmicRingComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// [E: Destruimos el disco visual explícitamente para evitar fantasmas en el Editor / I: Explicitly destroy visual disk to avoid Editor ghosts]
	if (IsValid(MacroDiskComponent))
	{
		MacroDiskComponent->DestroyComponent();
	}

	// [E: Destruimos el generador de asteroides / I: Destroy the asteroid generator]
	if (IsValid(AsteroidHISM))
	{
		AsteroidHISM->ClearInstances(); // Vaciamos la memoria primero por seguridad
		AsteroidHISM->DestroyComponent();
	}

	// [E: Llamamos a la clase padre para que termine el proceso de destrucción / I: Call parent class to finish the destruction process]
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

#if WITH_EDITOR
void UCosmicRingComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// [E: Como OnRegister ya lo creó, aquí solo actualizamos escala y variables / I: Since OnRegister created it, here we only update scale and variables]
	if (MacroDiskComponent && DynamicRingMat)
	{
		double CurrentScale = OuterRadiusKM * 2000.0;
		MacroDiskComponent->SetRelativeScale3D(FVector(CurrentScale, CurrentScale, 1.0f));
		// MacroDiskComponent->SetRelativeRotation(RingRotation);

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

	// [E: Validación básica: necesitamos mundo y malla para trabajar / I: Basic validation: world and mesh needed to work]
	if (!GetWorld() || !AsteroidMesh) return;

	FVector CameraLocation = FVector::ZeroVector;
	bool bGotCameraLocation = false;

	// [E: LÓGICA DE DETECCIÓN DE CÁMARA (JUEGO VS EDITOR) / I: CAMERA DETECTION LOGIC (GAME VS EDITOR)]
	if (GetWorld()->IsGameWorld())
	{
		// [E: Si estamos en juego, usamos la posición del Pawn del jugador / I: If in game, use player's Pawn location]
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (PlayerPawn)
		{
			CameraLocation = PlayerPawn->GetActorLocation();
			bGotCameraLocation = true;
		}
	}
#if WITH_EDITOR
	else
	{
		// [E: Si estamos en el Editor, buscamos la cámara activa del Viewport / I: If in Editor, look for active Viewport camera]
		if (GEditor && GEditor->GetActiveViewport() && GEditor->GetActiveViewport()->GetClient())
		{
			FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
			if (ViewportClient)
			{
				CameraLocation = ViewportClient->GetViewLocation();
				bGotCameraLocation = true;
			}
		}
	}
#endif

	if (!bGotCameraLocation) return;

	// [E: Cálculo de distancia y umbral de generación / I: Distance calculation and generation threshold]
	double DistanceToCamera = FVector::Distance(CameraLocation, GetComponentLocation());
	double GenerationThreshold = FadeMaxDistanceKM * 100000.0; // KM a Unidades Unreal (cm)

	// [E: LÓGICA DE GENERACIÓN ASÍNCRONA / I: ASYNC GENERATION LOGIC]
	// [E: Si entramos en el rango y no hay asteroides ni proceso activo / I: If in range and no asteroids or active process]
	if (DistanceToCamera <= GenerationThreshold && AsteroidHISM->GetInstanceCount() == 0 && !bIsGeneratingAsteroids)
	{
		bIsGeneratingAsteroids = true;

		// [E: Captura de variables locales para el hilo secundario / I: Capture local variables for background thread]
		double InnerCm = InnerRadiusKM * 100000.0;
		double OuterCm = OuterRadiusKM * 100000.0;
		int32 Count = 10000; // [E: Cantidad de asteroides / I: Asteroids count]

		AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, InnerCm, OuterCm, Count]()
			{
				TArray<FTransform> NewTransforms;
				NewTransforms.Reserve(Count);

				for (int32 i = 0; i < Count; i++)
				{
					float Angle = FMath::RandRange(0.0f, PI * 2.0f);
					float Distance = FMath::RandRange((float)InnerCm, (float)OuterCm);

					float X = FMath::Cos(Angle) * Distance;
					float Y = FMath::Sin(Angle) * Distance;
					float Z = FMath::RandRange(-20000.0f, 20000.0f); // [E: Grosor del anillo / I: Ring thickness]

					FVector Loc = FVector(X, Y, Z);
					FRotator Rot = FRotator(FMath::RandRange(0.f, 360.f), FMath::RandRange(0.f, 360.f), FMath::RandRange(0.f, 360.f));
					FVector Sca = FVector(FMath::RandRange(0.5f, 2.0f));

					NewTransforms.Add(FTransform(Rot, Loc, Sca));
				}

				// [E: Regreso al Hilo Principal para aplicar cambios en el HISM / I: Return to GameThread to apply HISM changes]
				AsyncTask(ENamedThreads::GameThread, [this, NewTransforms]()
					{
						if (IsValid(this) && AsteroidHISM)
						{
							AsteroidHISM->AddInstances(NewTransforms, false);
							UE_LOG(LogTemp, Warning, TEXT("¡Asteroides Generados! Instancias totales: %d"), AsteroidHISM->GetInstanceCount());
						}
						bIsGeneratingAsteroids = false;
					});
			});
	}
	// [E: LÓGICA DE LIMPIEZA / I: CLEANUP LOGIC]
	// [E: Si nos alejamos del umbral, liberamos memoria RAM / I: If we move away from threshold, free RAM]
	else if (DistanceToCamera > GenerationThreshold && AsteroidHISM->GetInstanceCount() > 0 && !bIsGeneratingAsteroids)
	{
		AsteroidHISM->ClearInstances();
	}
}