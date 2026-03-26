// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet/CosmicPlanet.h"
#include "Terrain/CosmicCollisionComponent.h"
#include "Terrain/CosmicClipmapComponent.h"
#include "Terrain/CosmicOceanClipmap.h"
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
    OceanClipmapComponent = CreateDefaultSubobject<UCosmicOceanClipmap>(TEXT("OceanClipmapComponent"));
    FoliageSpawnerComponent = CreateDefaultSubobject<UCosmicFoliageSpawner>(TEXT("FoliageSpawnerComponent"));
}

void ACosmicPlanet::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (FoliageSpawnerComponent) {
        FoliageSpawnerComponent->InitFoliageSpawner(RadiusKm, NoiseSettings);
    }
}



// Called when the game starts or when spawned
void ACosmicPlanet::BeginPlay()
{
	Super::BeginPlay();

    //InitClipmap();
    if (ClipmapComponent) {
        if (ClipmapComponent->bInitializedInEditor) {
            ClipmapComponent->ReasignLevels();
        }
        else {
            ClipmapComponent->ParentRoot = Root;
            ClipmapComponent->PlanetRadius = RadiusKm * 100000;
            ClipmapComponent->NoiseSettings = NoiseSettings;
            ClipmapComponent->SetMaterialData(PlanetMainColor1, PlanetMainColor2, PlanetAltitudeColor, MaterialNoiseScale);
            ClipmapComponent->CollisionComponent = CollisionComponent;
            ClipmapComponent->CreatePerformanceLevel(true);   
        }
    }

    if (OceanClipmapComponent) {
        if (OceanClipmapComponent->bInitializedInEditor) {
            OceanClipmapComponent->ReasignLevels();
        }
        else {
            OceanClipmapComponent->ParentRoot = Root;
            OceanClipmapComponent->PlanetRadius = RadiusKm * 100000;
            OceanClipmapComponent->CreatePerformanceLevel(true);
        }
    }
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

        ClipmapComponent->ParentRoot = Root;
        ClipmapComponent->PlanetRadius = RadiusKm * 100000;
        ClipmapComponent->ClearLevels();
        ClipmapComponent->NoiseSettings = NoiseSettings;
        ClipmapComponent->SetMaterialData(PlanetMainColor1, PlanetMainColor2, PlanetAltitudeColor, MaterialNoiseScale);
        ClipmapComponent->CollisionComponent = CollisionComponent;
        ClipmapComponent->CreatePerformanceLevel(true);
        ClipmapComponent->bInitializedInEditor = true;
        
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("No existe el clipmap"));
    }

    if (OceanClipmapComponent) {

        OceanClipmapComponent->ParentRoot = Root;
        OceanClipmapComponent->PlanetRadius = RadiusKm * 100000;
        OceanClipmapComponent->ClearLevels();
        OceanClipmapComponent->CreatePerformanceLevel(true);
        OceanClipmapComponent->bInitializedInEditor = true;

    }
    else {
        UE_LOG(LogTemp, Error, TEXT("No existe el ocean clipmap"));
    }
}


void ACosmicPlanet::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

//#if WITH_EDITOR
    if (!GetWorld()->IsGameWorld())
    {
        InitClipmap();
    }
//#endif
}

void ACosmicPlanet::InitPlanet(float InRadiusKm, UCosmicNoiseSettings* NewNoiseSettings, FColor color1, FColor color2, FColor colorHeight, float scale, UMaterialInterface* BaseMaterial, UTexture2D* DefaultTexture)
{
    RadiusKm = InRadiusKm;

    // Limpiar NoiseSettings anterior si existe y es transitorio
    if (NoiseSettings && (!NoiseSettings->IsAsset() || NoiseSettings->GetOutermost()->HasAnyPackageFlags(PKG_DisallowExport)))
    {
        NoiseSettings->ConditionalBeginDestroy();
        NoiseSettings = nullptr;
    }

    // Determinar qué NoiseSettings usar
    if (NewNoiseSettings)
    {
        // Usar el que nos pasaron
        NoiseSettings = NewNoiseSettings;
    }
    else if (!NoiseSettings)
    {
        // Crear uno por defecto si no tenemos ninguno
        //NoiseSettings = NewObject<UCosmicNoiseSettings>(this, TEXT("CustomNoiseSettings"));

        //// Configurar valores por defecto
        //UE_LOG(LogTemp, Log, TEXT("Created default NoiseSettings for planet"));
    }

    if (ClipmapComponent) {
        ClipmapComponent->BaseMaterial = BaseMaterial;
        ClipmapComponent->DefaultTexture = DefaultTexture;
    }

    if (OceanClipmapComponent) {
        OceanClipmapComponent->BaseMaterial = BaseMaterial;
        OceanClipmapComponent->DefaultTexture = DefaultTexture;
    }

    PlanetMainColor1 = color1;
    PlanetMainColor2 = color2;
    PlanetAltitudeColor = colorHeight;
    MaterialNoiseScale = scale;

    InitClipmap();
}

void ACosmicPlanet::CleanupNoiseSettings()
{
    if (NoiseSettings && !NoiseSettings->IsAsset())
    {
        // Marcar para garbage collection
        UE_LOG(LogTemp, Warning, TEXT("Planet %s cleaning up NoiseSettings"), *GetName());
        NoiseSettings->ConditionalBeginDestroy();
        NoiseSettings = nullptr;
    }
}

