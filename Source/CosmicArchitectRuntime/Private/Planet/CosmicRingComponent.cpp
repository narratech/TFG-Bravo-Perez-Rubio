// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Planet/CosmicRingComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Math/RandomStream.h"

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

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CosmicMaterialAsset(TEXT("/Script/Engine.MaterialInstanceConstant'/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicRing.M_CosmicRing'"));
	if (CosmicMaterialAsset.Succeeded())
	{
		MacroRingMaterial = CosmicMaterialAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> AsteroidMeshAsset(TEXT("/Script/Engine.StaticMesh'/CosmicArchitect/CosmicArchitect/Resources/Mesh/asteroid_01.asteroid_01'"));
	if (AsteroidMeshAsset.Succeeded())
	{
		AsteroidMesh = AsteroidMeshAsset.Object;
	}
}

void UCosmicRingComponent::BeginPlay()
{
	Super::BeginPlay();

	// Asegurar que las pools están vacías (pueden quedar restos del editor)
	ActiveSectors.Empty();
	HISMPool.Empty();

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
	while (HISMPool.Num() > 0)
	{
		UHierarchicalInstancedStaticMeshComponent* PooledHISM = HISMPool.Pop();

		// IsValid comprueba de forma segura que no sea nullptr ni esté destruido
		if (IsValid(PooledHISM))
		{
			return PooledHISM;
		}
	}

	// Si el pool estaba vacío (o lleno de punteros muertos), creamos uno nuevo de forma segura
	UHierarchicalInstancedStaticMeshComponent* NewHISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(
		this,
		NAME_None,
		RF_Transient | RF_DuplicateTransient
	);

	NewHISM->SetupAttachment(this);
	NewHISM->RegisterComponent();

	return NewHISM;
}

/**
 * Devuelve la distancia en centímetros desde LocalPosition hasta el punto más cercano
 * del volumen del anillo (annulus en el plano XY + espesor vertical RingThicknessKM).
 *
 * El cálculo se realiza en espacio local del componente para ser independiente de la
 * rotación/traslación mundial, lo que garantiza coherencia con la geometría real del plano.
 */
float UCosmicRingComponent::ComputeDistanceToRing(const FVector& LocalPosition) const
{
	const float InnerCm      = (float)(InnerRadiusKM  * 100000.0);
	const float OuterCm      = (float)(OuterRadiusKM  * 100000.0);
	const float HalfThickCm  = (float)(RingThicknessKM * 50000.0); // mitad del espesor total

	// Distancia radial desde el eje Z del anillo en el plano XY.
	const float RadialDist = FMath::Sqrt(LocalPosition.X * LocalPosition.X + LocalPosition.Y * LocalPosition.Y);

	// Punto más cercano dentro del rango radial del annulus.
	const float ClampedRadial = FMath::Clamp(RadialDist, InnerCm, OuterCm);

	// Dirección radial normalizada (evitar división por cero en el origen).
	float NX, NY;
	if (RadialDist > KINDA_SMALL_NUMBER)
	{
		NX = LocalPosition.X / RadialDist * ClampedRadial;
		NY = LocalPosition.Y / RadialDist * ClampedRadial;
	}
	else
	{
		NX = ClampedRadial;
		NY = 0.f;
	}

	// Punto más cercano en Z dentro del espesor del anillo.
	const float NZ = FMath::Clamp(LocalPosition.Z, -HalfThickCm, HalfThickCm);

	const FVector NearestRingPoint(NX, NY, NZ);
	return FVector::Distance(LocalPosition, NearestRingPoint);
}

/**
 * Sincroniza las propiedades C++ con el Material Instance Dinámico.
 *
 * Los radios UV se derivan automáticamente de los radios en KM:
 *   - El plano (Plane) tiene UV 0-1 con el centro en UV(0.5, 0.5).
 *   - El radio UV hasta el borde del plano es 0.5 (radio normalizado = 1.0).
 *   - OuterRadiusUV = 0.5 siempre (la malla escala para coincidir con OuterRadiusKM).
 *   - InnerRadiusUV = 0.5 * (InnerRadiusKM / OuterRadiusKM).
 */
void UCosmicRingComponent::UpdateShaderParameters()
{
	if (!DynamicRingMat) return;

	// Cálculo automático de UVs a partir de los radios reales.
	const float OuterRadiusUV = 0.49f;
	const float InnerRadiusUV = (OuterRadiusKM > 0.0)
		? (float)(0.5 * InnerRadiusKM / OuterRadiusKM)
		: 0.0f;

	DynamicRingMat->SetVectorParameterValue(FName("RingColor"),      RingColor);
	DynamicRingMat->SetScalarParameterValue(FName("BandFrequency"),  (float)BandFrequency);
	DynamicRingMat->SetScalarParameterValue(FName("InnerRadius"),    InnerRadiusUV);
	DynamicRingMat->SetScalarParameterValue(FName("OuterRadius"),    OuterRadiusUV);

	// Distancias de fade en unidades de Unreal (cm) para compatibilidad con el shader.
	DynamicRingMat->SetScalarParameterValue(FName("FadeMinDistance"), (float)(FadeMinDistanceKM * 100000.0));
	DynamicRingMat->SetScalarParameterValue(FName("FadeMaxDistance"), (float)(FadeMaxDistanceKM * 100000.0));
}

#if WITH_EDITOR
void UCosmicRingComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	// Actualizar rotación si cambió.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, RingRotation))
	{
		SetRelativeRotation(RingRotation);
	}

	// Actualizar escala y shader si cambió cualquier propiedad dimensional o visual.
	if (MacroDiskComponent && DynamicRingMat)
	{
		const double CurrentScale = OuterRadiusKM * 2000.0;
		MacroDiskComponent->SetRelativeScale3D(FVector(CurrentScale, CurrentScale, 1.0f));
		UpdateShaderParameters();
	}

	// Actualizar el color de los materiales de asteroides activos.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, RingColor))
	{
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

	// Propiedades que afectan la distribución geométrica de los asteroides:
	// invalidar todos los sectores para forzar regeneración en el siguiente tick.
	static const TArray<FName> RegenerationTriggers = {
		GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, InnerRadiusKM),
		GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, OuterRadiusKM),
		GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, RingThicknessKM),
		GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, AsteroidsPerSector),
		GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, MinScale),
		GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, MaxScale),
		GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, SectorAngleDegrees),
		GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, AsteroidActivationDistanceKM),
	};

	if (RegenerationTriggers.Contains(PropertyName))
	{
		InvalidateAllSectors();
	}
}
#endif

/**
 * Devuelve todos los sectores activos al pool sin destruir los componentes HISM.
 * El siguiente tick detectará los sectores que faltan y los regenerará con los parámetros actuales.
 */
void UCosmicRingComponent::InvalidateAllSectors()
{
	TArray<int32> ActiveKeys;
	ActiveSectors.GetKeys(ActiveKeys);

	for (int32 Key : ActiveKeys)
	{
		UHierarchicalInstancedStaticMeshComponent* HISM = ActiveSectors[Key];
		if (IsValid(HISM))
		{
			HISM->ClearInstances();
			HISMPool.Add(HISM);
		}
	}
	ActiveSectors.Empty();
}

/**
 * Lógica principal de sectorización dinámica.
 *
 * Calcula la posición polar del observador para determinar qué cuñas del anillo deben
 * renderizarse con mallas 3D. La detección de proximidad usa la distancia real al punto
 * más cercano del volumen del anillo (no al centro del componente).
 *
 * La generación y destrucción de sectores está presupuestada por MaxInstancesPerSecond:
 * se procesa un sector completo aunque supere el límite en ese frame, y los sectores
 * pendientes se aplazan al frame siguiente.
 */
void UCosmicRingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetWorld() || !AsteroidMesh) return;

	// 1. Obtener la posición del observador (juego o editor).
	FVector CameraLocation = FVector::ZeroVector;
	bool bGotCameraLocation = false;

	if (GetWorld()->IsGameWorld())
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (PlayerPawn)
		{
			CameraLocation     = PlayerPawn->GetActorLocation();
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
				CameraLocation     = ViewportClient->GetViewLocation();
				bGotCameraLocation = true;
			}
		}
	}
#endif

	if (!bGotCameraLocation) return;


	// Calcular distancia al punto más cercano del anillo (espacio local).
	// Esto garantiza que la activación se dispare cuando el observador se
	// aproxima al borde real del annulus, no al centro del componente.
	const FVector LocalCamLoc = GetComponentTransform().InverseTransformPosition(CameraLocation);
	const float   DistToRing  = ComputeDistanceToRing(LocalCamLoc);
	const float   ActivationThresholdCm = (float)(AsteroidActivationDistanceKM * 100000.0);

	if (DistToRing <= ActivationThresholdCm)
	{

		// Calcular el sector actual del observador en coordenadas polares locales.

		float AngleRad = FMath::Atan2(LocalCamLoc.Y, LocalCamLoc.X);
		if (AngleRad < 0.f) AngleRad += 2.f * PI;
		const float  AngleDeg     = FMath::RadiansToDegrees(AngleRad);
		const int32  CurrentSector = FMath::FloorToInt(AngleDeg / SectorAngleDegrees);
		const int32  TotalSectors  = FMath::Max(1, FMath::FloorToInt(360.0f / SectorAngleDegrees));


		// Determinar el conjunto de sectores requeridos alrededor del observador.
		TArray<int32> RequiredSectors;
		RequiredSectors.Reserve(VisibleSectors * 2 + 1);
		for (int32 i = -VisibleSectors; i <= VisibleSectors; ++i)
		{
			int32 SectorID = (CurrentSector + i) % TotalSectors;
			if (SectorID < 0) SectorID += TotalSectors;
			RequiredSectors.Add(SectorID);
		}

		// Presupuesto de instancias por frame.
		// Siempre se garantiza al menos un sector completo de margen.
		const int32 MaxInstancesThisFrame  = FMath::Max(AsteroidsPerSector, FMath::RoundToInt(MaxInstancesPerSecond * DeltaTime));
		int32       InstancesThisFrame     = 0;

		// Destrucción de sectores obsoletos (throttled).
		// Se procesan hasta agotar el presupuesto; el resto, al frame siguiente.
		TArray<int32> ActiveKeys;
		ActiveSectors.GetKeys(ActiveKeys);

		for (int32 ActiveID : ActiveKeys)
		{
			if (!RequiredSectors.Contains(ActiveID))
			{
				// Comprobar presupuesto ANTES de iniciar este sector.
				if (InstancesThisFrame >= MaxInstancesThisFrame) break;

				UHierarchicalInstancedStaticMeshComponent* OldHISM = ActiveSectors[ActiveID];
				if (IsValid(OldHISM))
				{
					OldHISM->ClearInstances();
					HISMPool.Add(OldHISM);
				}
				ActiveSectors.Remove(ActiveID);

				// Contabilizar DESPUÉS de completar el sector.
				InstancesThisFrame += AsteroidsPerSector;
			}
		}


		// Generación asíncrona de nuevos sectores (throttled).
		// El presupuesto es compartido con la destrucción del paso anterior.
		const double InnerCm         = InnerRadiusKM  * 99000.0;
		const double OuterCm         = OuterRadiusKM  * 99000.0;
		const float  HalfThicknessCm = (float)(RingThicknessKM * 49500.0);
		const float  LocalMinScale   = MinScale;
		const float  LocalMaxScale   = MaxScale;
		const int32  Density         = AsteroidsPerSector;

		for (int32 ReqID : RequiredSectors)
		{
			if (ActiveSectors.Contains(ReqID)) continue;

			// Comprobar presupuesto ANTES de iniciar este sector.
			if (InstancesThisFrame >= MaxInstancesThisFrame) break;

			UHierarchicalInstancedStaticMeshComponent* SectorHISM = GetOrCreateHISM();
			SectorHISM->SetStaticMesh(AsteroidMesh);

			UMaterialInstanceDynamic* DynMat = SectorHISM->CreateDynamicMaterialInstance(0);
			if (DynMat)
			{
				DynMat->SetVectorParameterValue(FName("AsteroidColor"), RingColor);
			}
			ActiveSectors.Add(ReqID, SectorHISM);

			const float StartAngleRad = FMath::DegreesToRadians(ReqID       * SectorAngleDegrees);
			const float EndAngleRad   = FMath::DegreesToRadians((ReqID + 1) * SectorAngleDegrees);

			/**
			 * Ejecución en segundo plano para evitar bloqueos del Game Thread.
			 *
			 * Se usa FRandomStream con semilla derivada del ReqID para que el mismo sector
			 * genere siempre exactamente la misma distribución de asteroides,
			 * independientemente del orden de carga o regeneración.
			 */
			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
				[SectorHISM, StartAngleRad, EndAngleRad, InnerCm, OuterCm,
				 Density, LocalMinScale, LocalMaxScale, HalfThicknessCm, ReqID]()
				{
					FRandomStream SectorStream(ReqID * 2654435761); // Multiplicador primo para mejor distribución.

					TArray<FTransform> NewTransforms;
					NewTransforms.Reserve(Density);

					for (int32 i = 0; i < Density; ++i)
					{
						const float Angle    = SectorStream.FRandRange(StartAngleRad, EndAngleRad);
						const float Distance = SectorStream.FRandRange((float)InnerCm, (float)OuterCm);

						const float X = FMath::Cos(Angle) * Distance;
						const float Y = FMath::Sin(Angle) * Distance;
						const float Z = SectorStream.FRandRange(-HalfThicknessCm, HalfThicknessCm);

						const FVector    Loc(X, Y, Z);
						const FRotator   Rot(
							SectorStream.FRandRange(0.f, 360.f),
							SectorStream.FRandRange(0.f, 360.f),
							SectorStream.FRandRange(0.f, 360.f));
						const FVector    Sca(SectorStream.FRandRange(LocalMinScale, LocalMaxScale));

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

			// Contabilizar despues de despachar el sector.
			InstancesThisFrame += AsteroidsPerSector;
		}
	}

	// Limpieza total (throttled) si el observador abandona la zona de activación.
	else if (ActiveSectors.Num() > 0)
	{
		const int32 MaxInstancesThisFrame = FMath::Max(AsteroidsPerSector, FMath::RoundToInt(MaxInstancesPerSecond * DeltaTime));
		int32       InstancesThisFrame    = 0;

		TArray<int32> ActiveKeys;
		ActiveSectors.GetKeys(ActiveKeys);

		for (int32 ActiveID : ActiveKeys)
		{
			if (InstancesThisFrame >= MaxInstancesThisFrame) break;

			UHierarchicalInstancedStaticMeshComponent* OldHISM = ActiveSectors[ActiveID];
			if (IsValid(OldHISM))
			{
				OldHISM->ClearInstances();
				HISMPool.Add(OldHISM);
			}
			ActiveSectors.Remove(ActiveID);

			InstancesThisFrame += AsteroidsPerSector;
		}
	}
}