#include "Planet/CosmicRingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

// [E: Cabeceras de Editor protegidas para que el juego empaquetado no de error / I: Editor headers protected so packaged game doesn't error]
#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#endif

// [E: Constructor: Inicializamos escala, mallas y materiales automáticos / I: Constructor: We initialize scale, meshes and automatic materials]
UCosmicRingComponent::UCosmicRingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;

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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> AsteroidMeshAsset(TEXT("/Script/Engine.StaticMesh'/CosmicArchitect/Models/Asteroid/asteroid_01.asteroid_01'"));

	if (AsteroidMeshAsset.Succeeded())
	{
		AsteroidMesh = AsteroidMeshAsset.Object;
	}
}

void UCosmicRingComponent::BeginPlay()
{
	Super::BeginPlay();

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

		MacroDiskComponent->SetMaterial(0, DynamicRingMat);

		// [E: Aplicamos escala inicial y parámetros del shader / I: Apply initial scale and shader parameters]
		double CurrentScale = OuterRadiusKM * 2000.0;
		MacroDiskComponent->SetRelativeScale3D(FVector(CurrentScale, CurrentScale, 1.0f));

		UpdateShaderParameters();
	}
}

void UCosmicRingComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	// [E: Destruimos el disco visual explícitamente para evitar fantasmas en el Editor / I: Explicitly destroy visual disk to avoid Editor ghosts]
	if (IsValid(MacroDiskComponent))
	{
		MacroDiskComponent->DestroyComponent();
	}

	// [E: Destruimos todos los HISMs activos de los sectores / I: Destroy all active HISMs from sectors]
	for (auto& Pair : ActiveSectors)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->DestroyComponent();
		}
	}
	ActiveSectors.Empty();

	// [E: Destruimos los componentes reciclados guardados en el almacén / I: Destroy recycled components stored in the pool]
	for (UHierarchicalInstancedStaticMeshComponent* PoolHISM : HISMPool)
	{
		if (IsValid(PoolHISM))
		{
			PoolHISM->DestroyComponent();
		}
	}
	HISMPool.Empty();

	// [E: Llamamos a la clase padre para que termine el proceso de destrucción / I: Call parent class to finish the destruction process]
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

UHierarchicalInstancedStaticMeshComponent* UCosmicRingComponent::GetOrCreateHISM()
{
	// [E: Si tenemos un componente reciclado en el almacén, lo sacamos y lo usamos / I: If we have a recycled component, pop and use it]
	if (HISMPool.Num() > 0)
	{
		return HISMPool.Pop();
	}

	// [E: Si no hay reciclados, creamos uno nuevo dinámicamente / I: If no recycled ones, create dynamically]
	UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
	NewHISM->SetStaticMesh(AsteroidMesh);
	NewHISM->SetupAttachment(this);
	NewHISM->RegisterComponent(); // Obligatorio al crear componentes en tiempo de ejecución
	return NewHISM;
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

		UpdateShaderParameters();
	}

	for (auto& Pair : ActiveSectors)
	{
		if (IsValid(Pair.Value))
		{
			UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(Pair.Value->GetMaterial(0));
			if (DynMat)
			{
				DynMat->SetVectorParameterValue(FName("AsteroidColor"), RingColor);
			}
		}
	}
}
#endif

void UCosmicRingComponent::UpdateShaderParameters()
{
	if (DynamicRingMat)
	{
		DynamicRingMat->SetVectorParameterValue(FName("RingColor"), RingColor);
		DynamicRingMat->SetScalarParameterValue(FName("BandFrequency"), (float)BandFrequency);
		DynamicRingMat->SetScalarParameterValue(FName("InnerRadius"), (float)InnerRadiusUV);
		DynamicRingMat->SetScalarParameterValue(FName("OuterRadius"), (float)OuterRadiusUV);

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
	double GenerationThreshold = FadeMaxDistanceKM * 100000.0;

	// ===================================================================================================
	// [E: LÓGICA DE SECTORIZACIÓN DINÁMICA (TREADMILL) / I: DYNAMIC SECTORIZATION LOGIC (TREADMILL)]
	// ===================================================================================================
	if (DistanceToCamera <= GenerationThreshold)
	{
		// [E: Transformamos la posición de la cámara a coordenadas locales del anillo / I: Transform camera loc to ring local coordinates]
		FVector LocalCamLoc = GetComponentTransform().InverseTransformPosition(CameraLocation);

		// [E: Calculamos el ángulo en Radianes y lo pasamos a Grados (0 a 360) / I: Calculate angle in Radians and to Degrees]
		float AngleRad = FMath::Atan2(LocalCamLoc.Y, LocalCamLoc.X);
		if (AngleRad < 0) AngleRad += 2 * PI;
		float AngleDeg = FMath::RadiansToDegrees(AngleRad);

		// [E: Averiguamos en qué sector estamos (Ej: 45º / 15º = Sector 3) / I: Find current sector]
		int32 CurrentSector = FMath::FloorToInt(AngleDeg / SectorAngleDegrees);
		int32 TotalSectors = FMath::FloorToInt(360.0f / SectorAngleDegrees);

		// [E: 1. Crear lista de los sectores que DEBEN estar visibles / I: 1. Create list of REQUIRED visible sectors]
		TArray<int32> RequiredSectors;
		for (int32 i = -VisibleSectors; i <= VisibleSectors; ++i)
		{
			int32 SectorID = (CurrentSector + i) % TotalSectors;
			if (SectorID < 0) SectorID += TotalSectors; // Mantenemos el valor circular
			RequiredSectors.Add(SectorID);
		}

		// [E: 2. Eliminar sectores viejos que dejamos atrás / I: 2. Remove old sectors left behind]
		TArray<int32> ActiveKeys;
		ActiveSectors.GetKeys(ActiveKeys);

		for (int32 ActiveID : ActiveKeys)
		{
			if (!RequiredSectors.Contains(ActiveID))
			{
				UHierarchicalInstancedStaticMeshComponent* OldHISM = ActiveSectors[ActiveID];
				if (IsValid(OldHISM))
				{
					OldHISM->ClearInstances();
					HISMPool.Add(OldHISM);
				}
				ActiveSectors.Remove(ActiveID);
			}
		}

		// [E: 3. Generar sectores nuevos frente a nosotros / I: 3. Generate new sectors ahead of us]
		double InnerCm = InnerRadiusKM * 100000.0;
		double OuterCm = OuterRadiusKM * 100000.0;
		float LocalMinScale = MinScale;
		float LocalMaxScale = MaxScale;
		int32 Density = AsteroidsPerSector;

		for (int32 ReqID : RequiredSectors)
		{
			if (!ActiveSectors.Contains(ReqID))
			{
				// [E: Pedimos un HISM (nuevo o reciclado) y lo guardamos / I: Request a HISM (new or recycled) and store it]
				UHierarchicalInstancedStaticMeshComponent* SectorHISM = GetOrCreateHISM();
				// [E: Aseguramos que tiene la malla correcta por si el usuario la cambió en el editor / I: Ensure correct mesh in case of Editor changes]
				SectorHISM->SetStaticMesh(AsteroidMesh);
				UMaterialInstanceDynamic* DynMat = SectorHISM->CreateDynamicMaterialInstance(0);
				if (DynMat)
				{
					DynMat->SetVectorParameterValue(FName("AsteroidColor"), RingColor);
				}
				ActiveSectors.Add(ReqID, SectorHISM);

				float StartAngleRad = FMath::DegreesToRadians(ReqID * SectorAngleDegrees);
				float EndAngleRad = FMath::DegreesToRadians((ReqID + 1) * SectorAngleDegrees);

				// [E: Lanzamos el hilo asíncrono SOLO para este trozo / I: Launch async thread ONLY for this slice]
				AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [SectorHISM, StartAngleRad, EndAngleRad, InnerCm, OuterCm, Density, LocalMinScale, LocalMaxScale]()
					{
						TArray<FTransform> NewTransforms;
						NewTransforms.Reserve(Density);

						for (int32 i = 0; i < Density; i++)
						{
							float Angle = FMath::RandRange(StartAngleRad, EndAngleRad);
							float Distance = FMath::RandRange((float)InnerCm, (float)OuterCm);

							float X = FMath::Cos(Angle) * Distance;
							float Y = FMath::Sin(Angle) * Distance;
							float Z = FMath::RandRange(-20000.0f, 20000.0f);

							FVector Loc = FVector(X, Y, Z);
							FRotator Rot = FRotator(FMath::RandRange(0.f, 360.f), FMath::RandRange(0.f, 360.f), FMath::RandRange(0.f, 360.f));
							FVector Sca = FVector(FMath::RandRange(LocalMinScale, LocalMaxScale));

							NewTransforms.Add(FTransform(Rot, Loc, Sca));
						}

						AsyncTask(ENamedThreads::GameThread, [SectorHISM, NewTransforms]()
							{
								if (IsValid(SectorHISM))
								{
									SectorHISM->AddInstances(NewTransforms, false);
								}
							});
					});
			}
		}
	}
	// ===================================================================================================
	// [E: LÓGICA DE LIMPIEZA TOTAL / I: FULL CLEANUP LOGIC]
	// ===================================================================================================
	else if (ActiveSectors.Num() > 0)
	{
		// [E: Si nos alejamos del anillo, mandamos todos los sectores activos al almacén / I: If we move away, send all active sectors to pool]
		TArray<int32> ActiveKeys;
		ActiveSectors.GetKeys(ActiveKeys);

		for (int32 ActiveID : ActiveKeys)
		{
			UHierarchicalInstancedStaticMeshComponent* OldHISM = ActiveSectors[ActiveID];
			if (IsValid(OldHISM))
			{
				OldHISM->ClearInstances();
				HISMPool.Add(OldHISM);
			}
		}
		ActiveSectors.Empty();
	}
}