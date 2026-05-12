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

/**
 * Configuración del objeto en tiempo de construcción.
 * Establece la jerarquía base y realiza la búsqueda de assets esenciales del plugin.
 */
UCosmicRingComponent::UCosmicRingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;

	MacroDiskComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MacroDiskComponent"));
	MacroDiskComponent->SetupAttachment(this);
	MacroDiskComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MacroDiskComponent->SetCastShadow(false);

	// Conversión de escala: Transforma el radio de Kilómetros a unidades de Unreal (Centímetros).
	// Un radio de 50 unidades base en el plano requiere un factor de 2000 para igualar la escala KM.
	double InitialScale = OuterRadiusKM * 2000.0;
	MacroDiskComponent->SetRelativeScale3D(FVector(InitialScale, InitialScale, 1.0f));
	SetRelativeRotation(RingRotation);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
	if (PlaneMeshAsset.Succeeded())
	{
		MacroDiskComponent->SetStaticMesh(PlaneMeshAsset.Object);
	}

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

	if (MacroRingMaterial && MacroDiskComponent)
	{
		DynamicRingMat = UMaterialInstanceDynamic::Create(MacroRingMaterial, this);
		MacroDiskComponent->SetMaterial(0, DynamicRingMat);

		double CurrentScale = OuterRadiusKM * 2000.0;
		MacroDiskComponent->SetRelativeScale3D(FVector(CurrentScale, CurrentScale, 1.0f));
		SetRelativeRotation(RingRotation);

		UpdateShaderParameters();
	}
}

void UCosmicRingComponent::OnRegister()
{
	Super::OnRegister();

	if (MacroDiskComponent && MacroRingMaterial)
	{
		if (!DynamicRingMat)
		{
			DynamicRingMat = UMaterialInstanceDynamic::Create(MacroRingMaterial, this);
		}

		MacroDiskComponent->SetMaterial(0, DynamicRingMat);

		double CurrentScale = OuterRadiusKM * 2000.0;
		MacroDiskComponent->SetRelativeScale3D(FVector(CurrentScale, CurrentScale, 1.0f));
		SetRelativeRotation(RingRotation);
		UpdateShaderParameters();
	}
}

void UCosmicRingComponent::OnAttachmentChanged()
{
	Super::OnAttachmentChanged();
	// Reset de transformaciones para asegurar que el plano macro coincida siempre con el centro del actor.
	SetRelativeLocation(FVector::ZeroVector);
	SetRelativeRotation(FRotator::ZeroRotator);
}

void UCosmicRingComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (IsValid(MacroDiskComponent))
	{
		MacroDiskComponent->DestroyComponent();
	}

	for (auto& Pair : ActiveSectors)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->DestroyComponent();
		}
	}
	ActiveSectors.Empty();

	for (UHierarchicalInstancedStaticMeshComponent* PoolHISM : HISMPool)
	{
		if (IsValid(PoolHISM))
		{
			PoolHISM->DestroyComponent();
		}
	}
	HISMPool.Empty();

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

/**
 * Gestión de Pooling: Extrae componentes del pool o crea nuevos HISMs.
 * Es vital para evitar los picos de frame (stutter) asociados a la instanciación de objetos.
 */
UHierarchicalInstancedStaticMeshComponent* UCosmicRingComponent::GetOrCreateHISM()
{
	if (HISMPool.Num() > 0)
	{
		return HISMPool.Pop();
	}

	UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
	NewHISM->SetStaticMesh(AsteroidMesh);
	NewHISM->SetupAttachment(this);
	NewHISM->RegisterComponent();
	return NewHISM;
}

#if WITH_EDITOR
void UCosmicRingComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, RingRotation))
	{
		SetRelativeRotation(RingRotation);
	}

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

		// Envío de distancias en unidades de Unreal (cm) para compatibilidad con el shader.
		DynamicRingMat->SetScalarParameterValue(FName("FadeMinDistance"), (float)(FadeMinDistanceKM * 100000.0));
		DynamicRingMat->SetScalarParameterValue(FName("FadeMaxDistance"), (float)(FadeMaxDistanceKM * 100000.0));
	}
}

/**
 * Lógica principal de sectorización dinámica.
 * Calcula la posición polar del observador para determinar qué cuñas del anillo deben
 * renderizarse con mallas 3D y cuáles deben permanecer como representación macro.
 */
void UCosmicRingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetWorld() || !AsteroidMesh) return;

	FVector CameraLocation = FVector::ZeroVector;
	bool bGotCameraLocation = false;

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

	double DistanceToCamera = FVector::Distance(CameraLocation, GetComponentLocation());
	double GenerationThreshold = FadeMaxDistanceKM * 100000.0;

	if (DistanceToCamera <= GenerationThreshold)
	{
		// Transformación de la posición global a local para el cálculo de ángulos polares.
		FVector LocalCamLoc = GetComponentTransform().InverseTransformPosition(CameraLocation);

		float AngleRad = FMath::Atan2(LocalCamLoc.Y, LocalCamLoc.X);
		if (AngleRad < 0) AngleRad += 2 * PI;
		float AngleDeg = FMath::RadiansToDegrees(AngleRad);

		int32 CurrentSector = FMath::FloorToInt(AngleDeg / SectorAngleDegrees);
		int32 TotalSectors = FMath::FloorToInt(360.0f / SectorAngleDegrees);

		// Paso 1: Determinar el set de sectores requeridos alrededor de la cámara.
		TArray<int32> RequiredSectors;
		for (int32 i = -VisibleSectors; i <= VisibleSectors; ++i)
		{
			int32 SectorID = (CurrentSector + i) % TotalSectors;
			if (SectorID < 0) SectorID += TotalSectors;
			RequiredSectors.Add(SectorID);
		}

		// Paso 2: Limpieza de sectores obsoletos (Fuera de la vista).
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

		// Paso 3: Generación asíncrona de nuevos sectores frente al observador.
		double InnerCm = InnerRadiusKM * 100000.0;
		double OuterCm = OuterRadiusKM * 100000.0;
		float LocalMinScale = MinScale;
		float LocalMaxScale = MaxScale;
		int32 Density = AsteroidsPerSector;

		for (int32 ReqID : RequiredSectors)
		{
			if (!ActiveSectors.Contains(ReqID))
			{
				UHierarchicalInstancedStaticMeshComponent* SectorHISM = GetOrCreateHISM();
				SectorHISM->SetStaticMesh(AsteroidMesh);
				UMaterialInstanceDynamic* DynMat = SectorHISM->CreateDynamicMaterialInstance(0);
				if (DynMat)
				{
					DynMat->SetVectorParameterValue(FName("AsteroidColor"), RingColor);
				}
				ActiveSectors.Add(ReqID, SectorHISM);

				float StartAngleRad = FMath::DegreesToRadians(ReqID * SectorAngleDegrees);
				float EndAngleRad = FMath::DegreesToRadians((ReqID + 1) * SectorAngleDegrees);

				// Ejecución en segundo plano para evitar bloqueos del Game Thread durante el cálculo de dispersión.
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
							float Z = FMath::RandRange(-20000.0f, 20000.0f); // Dispersión vertical del anillo.

							FVector Loc = FVector(X, Y, Z);
							FRotator Rot = FRotator(FMath::RandRange(0.f, 360.f), FMath::RandRange(0.f, 360.f), FMath::RandRange(0.f, 360.f));
							FVector Sca = FVector(FMath::RandRange(LocalMinScale, LocalMaxScale));

							NewTransforms.Add(FTransform(Rot, Loc, Sca));
						}

						// Los datos calculados se inyectan en el hilo principal para su renderizado.
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
	// Gestión de limpieza total si el observador abandona las proximidades del anillo.
	else if (ActiveSectors.Num() > 0)
	{
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