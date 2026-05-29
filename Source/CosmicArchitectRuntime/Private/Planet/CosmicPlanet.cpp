// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.


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
 * Constructor de la clase ACosmicPlanet.
 * Establece la estructura de componentes necesaria para la generación del planeta.
 */
ACosmicPlanet::ACosmicPlanet()
{
    PrimaryActorTick.bCanEverTick = true; 

    // Inicialización del SceneComponent raíz.
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // Inicialización de componentes especializados en terreno, océanos y follaje.
    CollisionComponent = CreateDefaultSubobject<UCosmicCollisionComponent>(TEXT("CollisionComponent"));
    ClipmapComponent = CreateDefaultSubobject<UCosmicClipmapComponent>(TEXT("ClipmapComponent"));
    OceanComponent = CreateDefaultSubobject<UCosmicOceanComponent>(TEXT("OceanComponent"));
    FoliageSpawnerComponent = CreateDefaultSubobject<UCosmicFoliageSpawner>(TEXT("FoliageSpawnerComponent"));
}

/**
 * Fase posterior a la inicialización de componentes.
 * Sincroniza los datos entre los distintos sistemas del planeta antes de que comience el Tick.
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
 * Lógica para manejar la duplicación de planetas en el Editor.
 * Asegura que el nuevo planeta tenga sus radios y sistemas de follaje correctamente inicializados.
 */
void ACosmicPlanet::PostDuplicate(EDuplicateMode::Type Mode)
{
    Super::PostDuplicate(Mode);

    if (!GetWorld()->IsGameWorld())
    {
        // Escalamiento del radio a unidades de Unreal (Centímetros).
        ClipmapComponent->PlanetRadius = RadiusKm * 100000;
        if (FoliageSpawnerComponent)
        {
            ClipmapComponent->FoliageSpawnerComponent = FoliageSpawnerComponent;
            FoliageSpawnerComponent->InitFoliageSpawner(RadiusKm);
        }
        UpdateNoiseSettings();
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
 * Configura los parámetros del componente de Clipmap.
 * Establece la jerarquía, radios y activa la generación de los niveles de detalle.
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
 * Función de conveniencia para reconstruir totalmente la presencia física del planeta.
 */
void ACosmicPlanet::RebuildPlanet()
{
    InitClipmap();
    UpdateFoliage();
    UpdateOcean();
}

/**
 * Actualiza los parámetros del material del terreno sin reconstruir la geometría.
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
 * Desvincula delegados y limpia colisiones para evitar fugas de memoria o errores de referencia.
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
 * Actualiza el sistema de ruido.
 * Vincula el delegado de cambio de ruido para permitir actualizaciones automáticas cuando se modifican los settings.
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
 * Reinicializa el sistema de distribución de follaje procedural.
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
 * Sincroniza el océano con el estado actual del planeta.
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
 * Lógica de construcción para el editor. Asegura que el planeta sea visible nada más soltarlo en el nivel.
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
 * Inicialización masiva de parámetros planetarios.
 * Gestiona también el ciclo de vida de los objetos de ruido internos para evitar redundancia de memoria.
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

    // Configuración del componente de Clipmap.
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

    // Configuración del océano.
    if (OceanComponent)
    {
        OceanComponent->bHasOcean = bInHasOcean;
        OceanComponent->SeaLevelKm = InSeaLevelKm;
        OceanComponent->OceanResolution = InOceanResolution;
        OceanComponent->OceanMaterial = InOceanMaterial;

        UpdateOcean();
    }

    // Configuración del follaje.
    if (FoliageSpawnerComponent && InFoliageCollection)
        FoliageSpawnerComponent->FoliageCollection = InFoliageCollection;

    // Asignación de colores y escalas para el shader de terreno.
    PlanetMainColor1 = Color1;
    PlanetMainColor2 = Color2;
    PlanetColdColor = ColorCold;
    PlanetHotColor = ColorHot;
    PlanetSlopeColor = ColorSlope;
    NoiseScaleLarge = ScaleL;
    NoiseScaleMedium = ScaleM;
    NoiseScaleSmall = ScaleS;

    // Activación de la reconstrucción de sistemas tras la carga de datos.
    InitClipmap();
    UpdateFoliage();
    UpdateNoiseSettings();
    UpdateMaterialOnly();
}

/**
 * Define la densidad y rangos de renderizado del follaje.
 */
void ACosmicPlanet::SetFoliageParams(int32 InFoliageInstancesPerFrame, float InNearLayerRadiusKm, float InMediumLayerRadiusKm, float InFarLayerRadiusKm)
{
    if (FoliageSpawnerComponent)
    {
        FoliageSpawnerComponent->MaxInstancesPerFrame = InFoliageInstancesPerFrame;
        FoliageSpawnerComponent->NearLayerRadiusKm = InNearLayerRadiusKm;
        FoliageSpawnerComponent->MediumLayerRadiusKm = InMediumLayerRadiusKm;
        FoliageSpawnerComponent->FarLayerRadiusKm = InFarLayerRadiusKm;
    }
}

/**
 * Limpieza de objetos de ruido generados de forma volátil (no assets).
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
 * Maneja las actualizaciones reactivas del actor en el editor de Unreal.
 * Permite ver los cambios en colores, radios o ruido de forma inmediata sin reiniciar el nivel.
 */
void ACosmicPlanet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // Categoría: Actualización visual rápida de materiales.
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

    // Categoría: Cambios en el generador de ruido.
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, NoiseClass))
    {
        UpdateNoiseSettings();
        return;
    }

    // Categoría: Cambios estructurales que requieren reconstrucción total.
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, RadiusKm))
    {
        RebuildPlanet();
        return;
    }
}
#endif