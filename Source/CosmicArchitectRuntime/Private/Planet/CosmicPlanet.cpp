// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "Planet/CosmicPlanet.h"
#include "Terrain/CosmicCollisionComponent.h"
#include "Terrain/CosmicPlanetCollisionManager.h"
#include "Terrain/CosmicClipmapComponent.h"
#include "Terrain/CosmicOceanComponent.h"
#include "CosmicDefaultNoiseStrategy.h"
#include "CosmicFoliageCollection.h"
#include "CosmicNoiseClass.h"
#include "CosmicFoliageSpawner.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "Materials/MaterialInstance.h"
#include "HAL/PlatformTime.h"
#include "Engine/Engine.h"

/**
 * Constructor of the ACosmicPlanet class.
 * Establishes component structure required for planet generation.
 */
ACosmicPlanet::ACosmicPlanet()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    // Static planetary bodies do not require network replication
    bReplicates = false;

    // Root SceneComponent initialization.
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // Specialized components initialization for terrain, oceans, and foliage.
    CollisionManager = CreateDefaultSubobject<UCosmicPlanetCollisionManager>(TEXT("CollisionManager"));
    ClipmapComponent = CreateDefaultSubobject<UCosmicClipmapComponent>(TEXT("ClipmapComponent"));
    OceanComponent = CreateDefaultSubobject<UCosmicOceanComponent>(TEXT("OceanComponent"));
    FoliageSpawnerComponent = CreateDefaultSubobject<UCosmicFoliageSpawner>(TEXT("FoliageSpawnerComponent"));
}

/**
 * Post-component initialization phase.
 * Synchronizes data across different planet systems before Tick begins.
 */
void ACosmicPlanet::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    UpdateMaterialOnly();
    UpdateNoiseSettings();
    InitClipmap();
    UpdateOcean();

    if (FoliageSpawnerComponent)
    {
        FoliageSpawnerComponent->InitFoliageSpawner(RadiusKm);
    }
}

void ACosmicPlanet::BeginPlay()
{
    Super::BeginPlay();
    UpdateNoiseStrategy();
    ElapsedTime = FMath::FRandRange(0.f, TimeToRefresh);
}

void ACosmicPlanet::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const double PlanetRadius = RadiusKm * 100000.0;
    const FVector PlanetCenter = GetActorLocation();

    // On dedicated server, only run physical collision management
    if (IsRunningDedicatedServer())
    {
        if (CollisionManager)
        {
            CollisionManager->UpdateCollisions(PlanetCenter, PlanetRadius, GetNoiseStrategy());
        }
        return;
    }

    if (!ClipmapComponent)
    {
        return;
    }

    ElapsedTime += DeltaSeconds;
    if (ElapsedTime < TimeToRefresh)
    {
        return;
    }
    ElapsedTime -= TimeToRefresh;

    FVector ViewerPos;
    FVector SurfacePos;
    FVector N;

    // Use fast distance first to determine if we are in performance mode
    const double FastDistance = ClipmapComponent->GetFastDistanceToSurface(ViewerPos, SurfacePos, N);
    const bool bPerformanceMode = FastDistance > PlanetRadius * ClipmapComponent->HeightVisibility;

    if (bPerformanceMode)
    {
        ClipmapComponent->UpdateMeshPhase(ViewerPos, SurfacePos, N, static_cast<float>(FastDistance));
    }
    else
    {
        const double DistanceToSurface = ClipmapComponent->GetDistanceToSurface(ViewerPos, SurfacePos, N);
        const FVector FoliageViewerPos = SurfacePos + N * DistanceToSurface;

        const ECosmicPlanetUpdatePhase PhaseToExecute = CurrentPhase;

        switch (CurrentPhase)
        {
        case ECosmicPlanetUpdatePhase::Foliage:
            if (FoliageSpawnerComponent)
            {
                FoliageSpawnerComponent->UpdateFoliageSpawner(
                    TimeToRefresh,
                    FoliageViewerPos,
                    PlanetCenter,
                    PlanetRadius,
                    DistanceToSurface,
                    GetNoiseStrategy()
                );
            }
            break;

        case ECosmicPlanetUpdatePhase::Collision:
        {
            const bool bCollisionUpdated = CollisionManager ? CollisionManager->UpdateCollisions(
                PlanetCenter,
                PlanetRadius,
                GetNoiseStrategy()
            ) : false;
            bool bOceanApplied = false;

            if (OceanComponent && OceanComponent->HasCompletedTask())
            {
                // Prioritize applying ocean mesh update when collision is free to avoid accumulating frame work.
                // If collision was updated, defer at most 2 collision cycles before applying anyway.
                if (!bCollisionUpdated || DeferredOceanPhaseCount >= 2)
                {
                    OceanComponent->CheckAndApplyOceanMeshUpdate();
                    bOceanApplied = true;
                    DeferredOceanPhaseCount = 0;
                }
                else
                {
                    DeferredOceanPhaseCount++;
                }
            }
            else
            {
                DeferredOceanPhaseCount = 0;
            }

            // Only run extra foliage if neither collision nor ocean consumed this frame's budget
            if (!bCollisionUpdated && !bOceanApplied && FoliageSpawnerComponent)
            {
                FoliageSpawnerComponent->UpdateFoliageSpawner(
                    TimeToRefresh,
                    FoliageViewerPos,
                    PlanetCenter,
                    PlanetRadius,
                    DistanceToSurface,
                    GetNoiseStrategy()
                );
            }
            break;
        }

        case ECosmicPlanetUpdatePhase::Mesh:
            ClipmapComponent->UpdateMeshPhase(ViewerPos, SurfacePos, N, static_cast<float>(DistanceToSurface));
            break;
        }

        CurrentPhase = static_cast<ECosmicPlanetUpdatePhase>(((uint8)CurrentPhase + 1) % 3);
    }
}

#if WITH_EDITOR
/**
 * Logic to handle planet duplication in the Editor.
 * Ensures new planet has its radii and foliage systems properly initialized.
 */
void ACosmicPlanet::PostDuplicate(EDuplicateMode::Type Mode)
{
    Super::PostDuplicate(Mode);

    if (!GetWorld()->IsGameWorld())
    {
        bInitializedInEditor = false;

        // 1. Duplicate noise object if not a persistent asset to avoid sharing state
        if (NoiseClass && !NoiseClass->IsAsset())
        {
            NoiseClass = DuplicateObject<UCosmicNoiseClass>(NoiseClass, this);
        }

        // 2. Unbind raw references from original planet without destroying its components
        if (ClipmapComponent)
        {
            ClipmapComponent->ResetPointersAfterDuplicate(Root);
            ClipmapComponent->PlanetRadius = RadiusKm * 100000;
        }

        if (OceanComponent)
        {
            OceanComponent->ResetPointersAfterDuplicate(Root);
        }

        UpdateMaterialOnly();
        UpdateNoiseSettings();
        InitClipmap();
        UpdateFoliage();
        UpdateOcean();
    }
}
#endif

/**
 * Cleanup logic when actor is removed from the world.
 */
void ACosmicPlanet::Destroyed()
{
    ClearData();
    Super::Destroyed();
}

/**
 * Pre-destruction phase.
 * Guarantees release of references and procedural memory.
 */
void ACosmicPlanet::BeginDestroy()
{
    ClearData();
    Super::BeginDestroy();
}

/**
 * Logic executed when execution session terminates.
 */
void ACosmicPlanet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearData();
    Super::EndPlay(EndPlayReason);
}

/**
 * Configures Clipmap component parameters.
 * Establishes hierarchy, radii, and activates level of detail generation.
 */
void ACosmicPlanet::InitClipmap()
{
    if (ClipmapComponent) {
        ClipmapComponent->ParentRoot = Root;
        ClipmapComponent->PlanetRadius = RadiusKm * 100000;
        ClipmapComponent->ClearLevels();
        ClipmapComponent->FoliageSpawnerComponent = FoliageSpawnerComponent;
        ClipmapComponent->OceanComponent = OceanComponent;
        ClipmapComponent->CreatePerformanceLevel(true);
        bInitializedInEditor = true;
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("No existe el clipmap"));
    }
}

/**
 * Convenience function to fully reconstruct planet's physical presence.
 */
void ACosmicPlanet::RebuildPlanet()
{
    InitClipmap();
    UpdateFoliage();
    UpdateOcean();
}

/**
 * Updates terrain material parameters without reconstructing geometry.
 */
void ACosmicPlanet::UpdateMaterialOnly()
{
    if (ClipmapComponent)
    {
        ClipmapComponent->SetMaterialData(
            PlanetMainColor1, PlanetMainColor2, PlanetColdColor, PlanetHotColor,
            PlanetSlopeColor, NoiseScaleLarge, NoiseScaleMedium, NoiseScaleSmall
        );
    }
}

/**
 * Unbinds delegates and cleans collisions to avoid memory leaks or reference errors.
 */
void ACosmicPlanet::ClearData()
{
    if (CollisionManager)
    {
        CollisionManager->ClearAllPatches();
    }

#if WITH_EDITOR
    if (NoiseClass)
    {
        NoiseClass->OnNoiseSettingsChanged.RemoveAll(this);
    }
#endif

    DeferredOceanPhaseCount = 0;
}

/**
 * Handles notifications when the assigned NoiseClass settings change in the Editor.
 */
void ACosmicPlanet::OnNoiseSettingsChanged()
{
    UpdateNoiseStrategy();

    if (CollisionManager)
    {
        CollisionManager->ClearAllPatches();
    }

    if (ClipmapComponent)
    {
        ClipmapComponent->RequestCompleteMeshUpdate();
    }
}

/**
 * Updates noise system.
 * Binds noise change delegate to allow automatic updates when settings are modified.
 */
void ACosmicPlanet::UpdateNoiseSettings()
{
    UpdateNoiseStrategy();

    if (ClipmapComponent)
    {
        ClipmapComponent->NoiseClass = NoiseClass;
    }

#if WITH_EDITOR
    if (NoiseClass)
    {
        NoiseClass->OnNoiseSettingsChanged.RemoveAll(this);
        NoiseClass->OnNoiseSettingsChanged.AddUObject(
            this,
            &ACosmicPlanet::OnNoiseSettingsChanged
        );
    }
#endif

    if (CollisionManager)
    {
        CollisionManager->ClearAllPatches();
    }

    if (ClipmapComponent)
    {
        ClipmapComponent->RequestCompleteMeshUpdate();
    }
}

/**
 * Reinitializes procedural foliage distribution system.
 */
void ACosmicPlanet::UpdateFoliage()
{
    if (FoliageSpawnerComponent)
    {
        FoliageSpawnerComponent->ClearFoliage();
        FoliageSpawnerComponent->InitFoliageSpawner(RadiusKm);
    }
}

/**
 * Synchronizes ocean with current planet state.
 */
void ACosmicPlanet::UpdateOcean()
{
    if (OceanComponent)
    {
        OceanComponent->InitOcean(RadiusKm, Root);
        if (OceanComponent->bHasOcean) OceanComponent->RegenerateOcean();
        else OceanComponent->ClearOcean();
    }
}

#if WITH_EDITOR
/**
 * Construction logic for the editor. Ensures the planet is visible right after being dropped into the level.
 */
void ACosmicPlanet::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (!GetWorld()->IsGameWorld() && !bInitializedInEditor)
    {
        UpdateMaterialOnly();
        UpdateNoiseSettings();
        InitClipmap();
        UpdateFoliage();
        UpdateOcean();
    }
}
#endif

/**
 * Bulk initialization of planetary parameters.
 * Also manages internal noise object lifecycle to avoid memory redundancy.
 */
void ACosmicPlanet::InitPlanet(
    float InRadiusKm,
    UCosmicNoiseClass* NewNoiseClass,
    FColor Color1, FColor Color2, FColor ColorCold, FColor ColorHot,
    FColor ColorSlope, float ScaleL, float ScaleM, float ScaleS,
    UMaterialInstance* InBaseMaterial,
    UTexture2D* InDefaultTexture,
    // Clipmap
    bool UseClipmap,
    int32 InBaseResolution,
    int32 InNumLevels,
    int32 InMinTriangleSize,
    float InHeightVisibility,
    // Ocean
    bool bInHasOcean,
    double InSeaLevelKm,
    int32 InOceanResolution,
    UMaterialInstance* InOceanMaterial,
    // Foliage
    UCosmicFoliageCollection* InFoliageCollection
)
{
    RadiusKm = InRadiusKm;

    if (NewNoiseClass)
    {
#if WITH_EDITOR
        if (NoiseClass && NoiseClass != NewNoiseClass)
        {
            NoiseClass->OnNoiseSettingsChanged.RemoveAll(this);
        }
#endif
        NoiseClass = NewNoiseClass;
    }

    // Clipmap component configuration.
    if (ClipmapComponent)
    {
        ClipmapComponent->BaseMaterial = InBaseMaterial;
        ClipmapComponent->DefaultTexture = InDefaultTexture;
        ClipmapComponent->BaseResolution = InBaseResolution;
        ClipmapComponent->NumLevels = InNumLevels;
        ClipmapComponent->MinTriangleSize = InMinTriangleSize;
        ClipmapComponent->HeightVisibility = InHeightVisibility;
        ClipmapComponent->UseClipmap = UseClipmap;
    }

    // Ocean configuration.
    if (OceanComponent)
    {
        OceanComponent->bHasOcean = bInHasOcean;
        OceanComponent->SeaLevelKm = InSeaLevelKm;
        OceanComponent->OceanResolution = InOceanResolution;
        OceanComponent->OceanMaterial = InOceanMaterial;

        UpdateOcean();
    }

    // Foliage configuration.
    if (FoliageSpawnerComponent && InFoliageCollection)
        FoliageSpawnerComponent->FoliageCollection = InFoliageCollection;

    // Assignment of colors and scales for terrain shader.
    PlanetMainColor1 = Color1;
    PlanetMainColor2 = Color2;
    PlanetColdColor = ColorCold;
    PlanetHotColor = ColorHot;
    PlanetSlopeColor = ColorSlope;
    NoiseScaleLarge = ScaleL;
    NoiseScaleMedium = ScaleM;
    NoiseScaleSmall = ScaleS;

    // Trigger reconstruction of systems after data loading.
    InitClipmap();
    UpdateFoliage();
    UpdateNoiseSettings();
    UpdateMaterialOnly();
}

/**
 * Defines foliage density and render ranges.
 */
void ACosmicPlanet::SetFoliageParams(int32 InFoliageInstancesPerFrame, float InNearLayerRadiusKm, float InMediumLayerRadiusKm, float InFarLayerRadiusKm)
{
    if (FoliageSpawnerComponent)
    {
        FoliageSpawnerComponent->MaxInstancesGeneratedPerFrame = InFoliageInstancesPerFrame;
        FoliageSpawnerComponent->NearLayerRadiusKm = InNearLayerRadiusKm;
        FoliageSpawnerComponent->MediumLayerRadiusKm = InMediumLayerRadiusKm;
        FoliageSpawnerComponent->FarLayerRadiusKm = InFarLayerRadiusKm;
    }
}

/**
 * Cleanup of volatile generated noise objects (non-assets).
 */
void ACosmicPlanet::CleanupNoiseSettings()
{
#if WITH_EDITOR
    if (NoiseClass)
    {
        NoiseClass->OnNoiseSettingsChanged.RemoveAll(this);
    }
#endif

    if (NoiseClass && !NoiseClass->IsAsset())
    {
        NoiseClass->ConditionalBeginDestroy();
        NoiseClass = nullptr;
    }
}

#if WITH_EDITOR
void ACosmicPlanet::PreEditChange(FProperty* PropertyAboutToChange)
{
    Super::PreEditChange(PropertyAboutToChange);

    const FName PropertyName = PropertyAboutToChange
        ? PropertyAboutToChange->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, NoiseClass))
    {
        if (NoiseClass)
        {
            NoiseClass->OnNoiseSettingsChanged.RemoveAll(this);
        }
    }
}

/**
 * Handles reactive actor updates in Unreal editor.
 * Allows seeing changes in colors, radii, or noise immediately without reloading level.
 */
void ACosmicPlanet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    const FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Category: Quick visual material update.
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, PlanetMainColor1) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, PlanetMainColor2) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, PlanetColdColor) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, PlanetHotColor) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, PlanetSlopeColor) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, NoiseScaleSmall) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, NoiseScaleMedium) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, NoiseScaleLarge))
    {
        UpdateMaterialOnly();
        return;
    }

    // Category: Changes to noise generator.
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, NoiseClass))
    {
        UpdateNoiseSettings();
        return;
    }

    // Category: Structural changes requiring full reconstruction.
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, RadiusKm))
    {
        RebuildPlanet();
        return;
    }
}
#endif

void ACosmicPlanet::UpdateNoiseStrategy()
{
    if (NoiseClass)
    {
        NoiseGenerationStrategy = NoiseClass->CreateStrategy();
    }
    else
    {
        TSharedPtr<FCosmicDefaultNoiseStrategy> Strategy = MakeShared<FCosmicDefaultNoiseStrategy>();
        Strategy->Initialize(1337, FCosmicNoiseLayer(), FCosmicNoiseBiomeParameters());
        NoiseGenerationStrategy = Strategy;
    }
}

TSharedPtr<ICosmicNoiseStrategy> ACosmicPlanet::GetNoiseStrategy()
{
    if (!NoiseGenerationStrategy.IsValid())
    {
        UpdateNoiseStrategy();
    }
    return NoiseGenerationStrategy;
}