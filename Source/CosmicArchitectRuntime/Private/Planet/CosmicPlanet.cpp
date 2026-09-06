// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "Planet/CosmicPlanet.h"
#include "Terrain/CosmicCollisionComponent.h"
#include "Terrain/CosmicClipmapComponent.h"
#include "Terrain/CosmicOceanComponent.h"
#include "CosmicFoliageCollection.h"
#include "CosmicNoiseClass.h"
#include "CosmicFoliageSpawner.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "Materials/MaterialInstance.h"

/**
 * Constructor of the ACosmicPlanet class.
 * Establishes component structure required for planet generation.
 */
ACosmicPlanet::ACosmicPlanet()
{
    PrimaryActorTick.bCanEverTick = true; 

    // Root SceneComponent initialization.
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // Specialized components initialization for terrain, oceans, and foliage.
    CollisionComponent = CreateDefaultSubobject<UCosmicCollisionComponent>(TEXT("CollisionComponent"));
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

        // 3. Orderly reconstruct all subsystems for the new planet
        UpdateMaterialOnly();
        UpdateNoiseSettings();
        InitClipmap();
        UpdateFoliage();
        UpdateOcean();
    }
}
#endif

void ACosmicPlanet::Destroyed()
{
    ClearData();
    bInitializedInEditor = false;
    Super::Destroyed();
}

void ACosmicPlanet::BeginDestroy()
{
    ClearData();
    Super::BeginDestroy();
}

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
        ClipmapComponent->CollisionComponent = CollisionComponent;
        ClipmapComponent->FoliageSpawnerComponent = FoliageSpawnerComponent;
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
    if (CollisionComponent)
    {
        CollisionComponent->ClearCollision();
    }

    if (NoiseClass && ClipmapComponent)
    {
        NoiseClass->OnNoiseSettingsChanged.RemoveAll(ClipmapComponent);
    }
}

/**
 * Updates noise system.
 * Binds noise change delegate to allow automatic updates when settings are modified.
 */
void ACosmicPlanet::UpdateNoiseSettings()
{
    if (ClipmapComponent)
    {
        if (ClipmapComponent->NoiseClass)
            ClipmapComponent->NoiseClass->OnNoiseSettingsChanged.RemoveAll(ClipmapComponent);

        ClipmapComponent->NoiseClass = NoiseClass;

        if (NoiseClass)
        {
            NoiseClass->OnNoiseSettingsChanged.AddUObject(
                ClipmapComponent,
                &UCosmicClipmapComponent::RequestCompleteMeshUpdate
            );
        }

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
    UMaterialInstance* BaseMaterial,
    UTexture2D* DefaultTexture,
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
        NoiseClass = NewNoiseClass;

    // Clipmap component configuration.
    if (ClipmapComponent)
    {
        ClipmapComponent->BaseMaterial = BaseMaterial;
        ClipmapComponent->DefaultTexture = DefaultTexture;
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
    if (NoiseClass && !NoiseClass->IsAsset())
    {
        NoiseClass->ConditionalBeginDestroy();
        NoiseClass = nullptr;
    }
}

#if WITH_EDITOR
/**
 * Handles reactive actor updates in Unreal editor.
 * Allows seeing changes in colors, radii, or noise immediately without reloading level.
 */
void ACosmicPlanet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

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