// Fill out your copyright notice in the Description page of Project Settings.


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

// Sets default values
ACosmicPlanet::ACosmicPlanet()
{
    PrimaryActorTick.bCanEverTick = true;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    CollisionComponent = CreateDefaultSubobject<UCosmicCollisionComponent>(TEXT("CollisionComponent"));
    ClipmapComponent = CreateDefaultSubobject<UCosmicClipmapComponent>(TEXT("ClipmapComponent"));
    OceanComponent = CreateDefaultSubobject<UCosmicOceanComponent>(TEXT("OceanComponent"));
    FoliageSpawnerComponent = CreateDefaultSubobject<UCosmicFoliageSpawner>(TEXT("FoliageSpawnerComponent"));
}

void ACosmicPlanet::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (ClipmapComponent) {
#if WITH_EDITOR
        ClipmapComponent->ParentRoot = Root;
        ClipmapComponent->PlanetRadius = RadiusKm * 100000;
        ClipmapComponent->NoiseClass = NoiseClass;
        ClipmapComponent->UpdateNoiseEvaluator();
        ClipmapComponent->CollisionComponent = CollisionComponent;
        ClipmapComponent->FoliageSpawnerComponent = FoliageSpawnerComponent;
        ClipmapComponent->SetMaterialData(PlanetMainColor1, PlanetMainColor2, PlanetAltitudeColor, MaterialNoiseScale);
        ClipmapComponent->ReasignLevels();
#else
        UpdateMaterialOnly();
        UpdateNoiseSettings();
        InitClipmap();
        UpdateOcean();
#endif
    }

    if (FoliageSpawnerComponent) 
    {
        FoliageSpawnerComponent->InitFoliageSpawner(RadiusKm);
    }
}



// Called when the game starts or when spawned
void ACosmicPlanet::BeginPlay()
{
	Super::BeginPlay(); 
}

void ACosmicPlanet::PostDuplicate(EDuplicateMode::Type Mode)
{
    Super::PostDuplicate(Mode);

    ClipmapComponent->PlanetRadius = RadiusKm * 100000;

    if (!GetWorld()->IsGameWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("Duplicando planeta"));
      
        UpdateNoiseSettings();
    }
}


void ACosmicPlanet::Destroyed()
{

    //UE_LOG(LogTemp, Warning, TEXT("Destruyendo planeta"));

    if (CollisionComponent)
    {
        //UE_LOG(LogTemp, Warning, TEXT("Eliminando colisiones"));
        CollisionComponent->ClearCollision();
    }

    if (NoiseClass && ClipmapComponent)
    {
        NoiseClass->OnNoiseSettingsChanged.RemoveAll(ClipmapComponent);
    }

    bInitializedInEditor = false;

    Super::Destroyed();
}

void ACosmicPlanet::BeginDestroy()
{
    //UE_LOG(LogTemp, Warning, TEXT("Recolector destruyendo planeta"));

    if (CollisionComponent)
    {
        //UE_LOG(LogTemp, Warning, TEXT("Eliminando colisiones"));
        CollisionComponent->ClearCollision();
    }

    if (NoiseClass && ClipmapComponent)
    {
        NoiseClass->OnNoiseSettingsChanged.RemoveAll(ClipmapComponent);
    }

    Super::BeginDestroy();
}

void ACosmicPlanet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CollisionComponent)
    {
        CollisionComponent->ClearCollision();
    }

    if (NoiseClass && ClipmapComponent)
    {
        NoiseClass->OnNoiseSettingsChanged.RemoveAll(ClipmapComponent);
    }

    Super::EndPlay(EndPlayReason);
}

void ACosmicPlanet::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

   /* if (FoliageSpawnerComponent) {
        FoliageSpawnerComponent->UpdateFoliageGeneration(DeltaTime, GetActorLocation(), RadiusKm * 100000, NoiseSettings);
    }*/
}


void ACosmicPlanet::InitClipmap()
{
    if (ClipmapComponent) {

        //UE_LOG(LogTemp, Warning, TEXT("Regenerando planeta"));
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

void ACosmicPlanet::RebuildPlanet()
{
    InitClipmap(); 
    UpdateFoliage();
    UpdateOcean();
}

void ACosmicPlanet::UpdateMaterialOnly()
{
    if (ClipmapComponent)
    {
        //UE_LOG(LogTemp, Warning, TEXT("Actualizando material"));

        ClipmapComponent->SetMaterialData(
            PlanetMainColor1,
            PlanetMainColor2,
            PlanetAltitudeColor,
            MaterialNoiseScale
        );
    }
}

void ACosmicPlanet::UpdateNoiseSettings()
{
    if (ClipmapComponent)
    {
        if (ClipmapComponent->NoiseClass)
            ClipmapComponent->NoiseClass->OnNoiseSettingsChanged.RemoveAll(ClipmapComponent);

        //UE_LOG(LogTemp, Warning, TEXT("Actualizando delegates"));

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

void ACosmicPlanet::UpdateFoliage()
{
    if (FoliageSpawnerComponent)
    {
        FoliageSpawnerComponent->ClearFoliage();
        FoliageSpawnerComponent->InitFoliageSpawner(RadiusKm);
    }
}

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
void ACosmicPlanet::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    //UE_LOG(LogTemp, Warning, TEXT("Construyendo"));

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

void ACosmicPlanet::InitPlanet(
    float InRadiusKm,
    UCosmicNoiseClass* NewNoiseClass,
    FColor color1, FColor color2, FColor colorHeight, float scale,
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

    // Noise 
    if (NoiseClass && (!NoiseClass->IsAsset() ||
        NoiseClass->GetOutermost()->HasAnyPackageFlags(PKG_DisallowExport)))
    {
        NoiseClass->ConditionalBeginDestroy();
        NoiseClass = nullptr;
    }
    if (NewNoiseClass)
        NoiseClass = NewNoiseClass;

    // Clipmap 
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

    // Ocean 
    if (OceanComponent)
    {
        OceanComponent->bHasOcean = bInHasOcean;
        OceanComponent->SeaLevelKm = InSeaLevelKm;
        OceanComponent->OceanResolution = InOceanResolution;
        OceanComponent->OceanMaterial = InOceanMaterial;

        UpdateOcean();
    }

    // Foliage 
    if (FoliageSpawnerComponent && InFoliageCollection)
        FoliageSpawnerComponent->FoliageCollection = InFoliageCollection;

    // Material colors 
    PlanetMainColor1 = color1;
    PlanetMainColor2 = color2;
    PlanetAltitudeColor = colorHeight;
    MaterialNoiseScale = scale;

    InitClipmap();
    UpdateFoliage();
    UpdateNoiseSettings();
    UpdateMaterialOnly();
}

void ACosmicPlanet::CleanupNoiseSettings()
{
    if (NoiseClass && !NoiseClass->IsAsset())
    {
        // Marcar para garbage collection
        //UE_LOG(LogTemp, Warning, TEXT("Planet %s cleaning up NoiseSettings"), *GetName());
        NoiseClass->ConditionalBeginDestroy();
        NoiseClass = nullptr;
    }
}

#if WITH_EDITOR
void ACosmicPlanet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, PlanetMainColor1) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, PlanetMainColor2) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, PlanetAltitudeColor) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, MaterialNoiseScale))
    {
        UpdateMaterialOnly();
        return;
    }

    // CAMBIOS RUIDO
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, NoiseClass))
    {
        UpdateNoiseSettings();
        return;
    }

    // CAMBIOS PESADOS (rebuild)
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, RadiusKm))
    {
        RebuildPlanet();
        return;
    }
}
#endif