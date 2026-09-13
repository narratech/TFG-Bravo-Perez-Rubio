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
#include "Net/UnrealNetwork.h"

/**
 * Constructor of the ACosmicPlanet class.
 * Establishes component structure required for planet generation.
 */
ACosmicPlanet::ACosmicPlanet()
{
    PrimaryActorTick.bCanEverTick = true; 

    // Multiplayer replication configuration
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(60.0f);

    // Root SceneComponent initialization.
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // Specialized components initialization for terrain, oceans, and foliage.
    CollisionManager = CreateDefaultSubobject<UCosmicPlanetCollisionManager>(TEXT("CollisionManager"));
    ClipmapComponent = CreateDefaultSubobject<UCosmicClipmapComponent>(TEXT("ClipmapComponent"));
    OceanComponent = CreateDefaultSubobject<UCosmicOceanComponent>(TEXT("OceanComponent"));
    FoliageSpawnerComponent = CreateDefaultSubobject<UCosmicFoliageSpawner>(TEXT("FoliageSpawnerComponent"));
}

void ACosmicPlanet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ACosmicPlanet, RadiusKm);
    DOREPLIFETIME(ACosmicPlanet, NoiseClass);
    DOREPLIFETIME(ACosmicPlanet, PlanetMainColor1);
    DOREPLIFETIME(ACosmicPlanet, PlanetMainColor2);
    DOREPLIFETIME(ACosmicPlanet, PlanetColdColor);
    DOREPLIFETIME(ACosmicPlanet, PlanetHotColor);
    DOREPLIFETIME(ACosmicPlanet, PlanetSlopeColor);
    DOREPLIFETIME(ACosmicPlanet, NoiseScaleSmall);
    DOREPLIFETIME(ACosmicPlanet, NoiseScaleMedium);
    DOREPLIFETIME(ACosmicPlanet, NoiseScaleLarge);

    DOREPLIFETIME(ACosmicPlanet, BaseMaterial);
    DOREPLIFETIME(ACosmicPlanet, DefaultTexture);
    DOREPLIFETIME(ACosmicPlanet, bUseClipmap);
    DOREPLIFETIME(ACosmicPlanet, BaseResolution);
    DOREPLIFETIME(ACosmicPlanet, NumLevels);
    DOREPLIFETIME(ACosmicPlanet, MinTriangleSize);
    DOREPLIFETIME(ACosmicPlanet, HeightVisibility);
    DOREPLIFETIME(ACosmicPlanet, bHasOcean);
    DOREPLIFETIME(ACosmicPlanet, SeaLevelKm);
    DOREPLIFETIME(ACosmicPlanet, OceanResolution);
    DOREPLIFETIME(ACosmicPlanet, OceanMaterial);
    DOREPLIFETIME(ACosmicPlanet, FoliageCollection);
}

void ACosmicPlanet::OnRep_PlanetConfig()
{
    if (IsRunningDedicatedServer() || (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer))
    {
        return;
    }

    if (ClipmapComponent)
    {
        ClipmapComponent->BaseMaterial = BaseMaterial;
        ClipmapComponent->DefaultTexture = DefaultTexture;
        ClipmapComponent->BaseResolution = BaseResolution;
        ClipmapComponent->NumLevels = NumLevels;
        ClipmapComponent->MinTriangleSize = MinTriangleSize;
        ClipmapComponent->HeightVisibility = HeightVisibility;
        ClipmapComponent->UseClipmap = bUseClipmap;
    }

    if (OceanComponent)
    {
        OceanComponent->bHasOcean = bHasOcean;
        OceanComponent->SeaLevelKm = SeaLevelKm;
        OceanComponent->OceanResolution = OceanResolution;
        OceanComponent->OceanMaterial = OceanMaterial;
    }

    if (FoliageSpawnerComponent && FoliageCollection)
    {
        FoliageSpawnerComponent->FoliageCollection = FoliageCollection;
    }

    UpdateMaterialOnly();
    UpdateNoiseSettings();
    InitClipmap();
    UpdateOcean();
    UpdateFoliage();
}

/**
 * Post-component initialization phase.
 * Synchronizes data across different planet systems before Tick begins.
 */
void ACosmicPlanet::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // On remote clients, defer heavy procedural generation until replicated properties arrive
    if (GetWorld() && GetWorld()->IsGameWorld() && GetNetMode() == NM_Client && !NoiseClass)
    {
        return;
    }

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

        UpdateNoiseStrategy();
        RebuildPlanet();
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
    UpdateNoiseStrategy();

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
        NoiseClass = NewNoiseClass;

    this->BaseMaterial = InBaseMaterial;
    this->DefaultTexture = InDefaultTexture;
    this->bUseClipmap = UseClipmap;
    this->BaseResolution = InBaseResolution;
    this->NumLevels = InNumLevels;
    this->MinTriangleSize = InMinTriangleSize;
    this->HeightVisibility = InHeightVisibility;

    this->bHasOcean = bInHasOcean;
    this->SeaLevelKm = InSeaLevelKm;
    this->OceanResolution = InOceanResolution;
    this->OceanMaterial = InOceanMaterial;

    this->FoliageCollection = InFoliageCollection;

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