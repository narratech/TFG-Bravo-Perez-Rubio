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
 * Object configuration at construction time.
 * Sets base hierarchy and performs search for essential plugin assets.
 */
UCosmicRingComponent::UCosmicRingComponent()
{ 
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;

	MacroDiskComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MacroDiskComponent"));
	MacroDiskComponent->SetupAttachment(this);
	MacroDiskComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MacroDiskComponent->SetCastShadow(false);

	// Scale conversion: Transforms radius from Kilometers to Unreal units (Centimeters).
	// A base radius of 50 units on the plane requires a factor of 2000 to match KM scale.
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

	// Ensure pools are empty (residues from editor may remain)
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
	// Reset transforms to ensure macro plane always matches actor center.
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
 * Pooling Management: Extracts components from pool or creates new HISMs.
 * Vital to prevent frame stutter associated with object instantiation.
 */
UHierarchicalInstancedStaticMeshComponent* UCosmicRingComponent::GetOrCreateHISM()
{
	while (HISMPool.Num() > 0)
	{
		UHierarchicalInstancedStaticMeshComponent* PooledHISM = HISMPool.Pop();

		// IsValid safely checks that it is not nullptr or destroyed
		if (IsValid(PooledHISM))
		{
			return PooledHISM;
		}
	}

	// If pool was empty (or full of dead pointers), safely create a new one
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
 * Returns distance in centimeters from LocalPosition to nearest point
 * on ring volume (annulus on XY plane + vertical thickness RingThicknessKM).
 *
 * Calculation performed in component local space to be independent of
 * world rotation/translation, guaranteeing consistency with real plane geometry.
 */
float UCosmicRingComponent::ComputeDistanceToRing(const FVector& LocalPosition) const
{
	const float InnerCm      = (float)(InnerRadiusKM  * 100000.0);
	const float OuterCm      = (float)(OuterRadiusKM  * 100000.0);
	const float HalfThickCm  = (float)(RingThicknessKM * 50000.0); // half of total thickness

	// Radial distance from ring Z axis in XY plane.
	const float RadialDist = FMath::Sqrt(LocalPosition.X * LocalPosition.X + LocalPosition.Y * LocalPosition.Y);

	// Closest point within annulus radial range.
	const float ClampedRadial = FMath::Clamp(RadialDist, InnerCm, OuterCm);

	// Normalized radial direction (avoid division by zero at origin).
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

	// Closest point in Z within ring thickness.
	const float NZ = FMath::Clamp(LocalPosition.Z, -HalfThickCm, HalfThickCm);

	const FVector NearestRingPoint(NX, NY, NZ);
	return FVector::Distance(LocalPosition, NearestRingPoint);
}

/**
 * Synchronizes C++ properties with Dynamic Material Instance.
 *
 * UV radii are derived automatically from radii in KM:
 *   - Plane has UV 0-1 with center at UV(0.5, 0.5).
 *   - UV radius to plane edge is 0.5 (normalized radius = 1.0).
 *   - OuterRadiusUV = 0.5 always (mesh scales to match OuterRadiusKM).
 *   - InnerRadiusUV = 0.5 * (InnerRadiusKM / OuterRadiusKM).
 */
void UCosmicRingComponent::UpdateShaderParameters()
{
	if (!DynamicRingMat) return;

	// Automatic calculation of UVs from real radii.
	const float OuterRadiusUV = 0.49f;
	const float InnerRadiusUV = (OuterRadiusKM > 0.0)
		? (float)(0.5 * InnerRadiusKM / OuterRadiusKM)
		: 0.0f;

	DynamicRingMat->SetVectorParameterValue(FName("RingColor"),      RingColor);
	DynamicRingMat->SetScalarParameterValue(FName("BandFrequency"),  (float)BandFrequency);
	DynamicRingMat->SetScalarParameterValue(FName("InnerRadius"),    InnerRadiusUV);
	DynamicRingMat->SetScalarParameterValue(FName("OuterRadius"),    OuterRadiusUV);

	// Fade distances in Unreal units (cm) for shader compatibility.
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

	// Update rotation if changed.
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicRingComponent, RingRotation))
	{
		SetRelativeRotation(RingRotation);
	}

	// Update scale and shader if any dimensional or visual property changed.
	if (MacroDiskComponent && DynamicRingMat)
	{
		const double CurrentScale = OuterRadiusKM * 2000.0;
		MacroDiskComponent->SetRelativeScale3D(FVector(CurrentScale, CurrentScale, 1.0f));
		UpdateShaderParameters();
	}

	// Update material color of active asteroids.
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

	// Properties affecting geometric distribution of asteroids:
	// invalidate all sectors to force regeneration on next tick.
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
 * Returns all active sectors to pool without destroying HISM components.
 * Next tick will detect missing sectors and regenerate them with current parameters.
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
 * Main dynamic sectorization logic.
 *
 * Calculates observer polar position to determine which ring wedges should
 * be rendered with 3D meshes. Proximity detection uses actual distance to
 * closest point on ring volume (not component center).
 *
 * Sector generation and destruction is budgeted by MaxInstancesPerSecond:
 * a complete sector is processed even if exceeding limit in that frame, and
 * pending sectors are deferred to next frame.
 */
void UCosmicRingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetWorld() || !AsteroidMesh) return;

	// 1. Get observer position (game or editor).
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


	// Calculate distance to nearest ring point (local space).
	// This guarantees activation triggers when observer
	// approaches the true annulus edge, not component center.
	const FVector LocalCamLoc = GetComponentTransform().InverseTransformPosition(CameraLocation);
	const float   DistToRing  = ComputeDistanceToRing(LocalCamLoc);
	const float   ActivationThresholdCm = (float)(AsteroidActivationDistanceKM * 100000.0);

	if (DistToRing <= ActivationThresholdCm)
	{

		// Calculate observer current sector in local polar coordinates.

		float AngleRad = FMath::Atan2(LocalCamLoc.Y, LocalCamLoc.X);
		if (AngleRad < 0.f) AngleRad += 2.f * PI;
		const float  AngleDeg     = FMath::RadiansToDegrees(AngleRad);
		const int32  CurrentSector = FMath::FloorToInt(AngleDeg / SectorAngleDegrees);
		const int32  TotalSectors  = FMath::Max(1, FMath::FloorToInt(360.0f / SectorAngleDegrees));


		// Determine required set of sectors around observer.
		TArray<int32> RequiredSectors;
		RequiredSectors.Reserve(VisibleSectors * 2 + 1);
		for (int32 i = -VisibleSectors; i <= VisibleSectors; ++i)
		{
			int32 SectorID = (CurrentSector + i) % TotalSectors;
			if (SectorID < 0) SectorID += TotalSectors;
			RequiredSectors.Add(SectorID);
		}

		// Instance budget per frame.
		// At least one full sector margin is always guaranteed.
		const int32 MaxInstancesThisFrame  = FMath::Max(AsteroidsPerSector, FMath::RoundToInt(MaxInstancesPerSecond * DeltaTime));
		int32       InstancesThisFrame     = 0;

		// Destruction of obsolete sectors (throttled).
		// Processed until budget exhausted; remainder deferred to next frame.
		TArray<int32> ActiveKeys;
		ActiveSectors.GetKeys(ActiveKeys);

		for (int32 ActiveID : ActiveKeys)
		{
			if (!RequiredSectors.Contains(ActiveID))
			{
				// Check budget BEFORE starting this sector.
				if (InstancesThisFrame >= MaxInstancesThisFrame) break;

				UHierarchicalInstancedStaticMeshComponent* OldHISM = ActiveSectors[ActiveID];
				if (IsValid(OldHISM))
				{
					OldHISM->ClearInstances();
					HISMPool.Add(OldHISM);
				}
				ActiveSectors.Remove(ActiveID);

				// Account AFTER completing sector.
				InstancesThisFrame += AsteroidsPerSector;
			}
		}


		// Asynchronous generation of new sectors (throttled).
		// Budget is shared with destruction step above.
		const double InnerCm         = InnerRadiusKM  * 99000.0;
		const double OuterCm         = OuterRadiusKM  * 99000.0;
		const float  HalfThicknessCm = (float)(RingThicknessKM * 49500.0);
		const float  LocalMinScale   = MinScale;
		const float  LocalMaxScale   = MaxScale;
		const int32  Density         = AsteroidsPerSector;

		for (int32 ReqID : RequiredSectors)
		{
			if (ActiveSectors.Contains(ReqID)) continue;

			// Check budget BEFORE starting this sector.
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
			 * Background execution to avoid blocking the Game Thread.
			 *
			 * FRandomStream with seed derived from ReqID is used so that the same sector
			 * always generates the exact same asteroid distribution,
			 * regardless of loading or regeneration order.
			 */
			AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
				[SectorHISM, StartAngleRad, EndAngleRad, InnerCm, OuterCm,
				 Density, LocalMinScale, LocalMaxScale, HalfThicknessCm, ReqID]()
				{
					FRandomStream SectorStream(ReqID * 2654435761); // Prime multiplier for better distribution.

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

					// Calculated data is injected into main thread for rendering.
					AsyncTask(ENamedThreads::GameThread, [SectorHISM, NewTransforms]()
					{
						if (IsValid(SectorHISM))
						{
							SectorHISM->AddInstances(NewTransforms, false);
						}
					});
				});

			// Account after dispatching sector.
			InstancesThisFrame += AsteroidsPerSector;
		}
	}

	// Total cleanup (throttled) if observer leaves activation zone.
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