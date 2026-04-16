// Fill out your copyright notice in the Description page of Project Settings.


#include "Planet/CosmicPlanet.h"
#include "Terrain/CosmicCollisionComponent.h"
#include "Terrain/CosmicClipmapComponent.h"
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
    OceanMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OceanMesh"));
    OceanMesh->SetupAttachment(Root);
    OceanMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OceanMesh->SetCastShadow(false);
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

    if (ClipmapComponent) {
#if WITH_EDITOR
        ClipmapComponent->ParentRoot = Root;
        ClipmapComponent->PlanetRadius = RadiusKm * 100000;
        ClipmapComponent->NoiseSettings = NoiseSettings;
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

    UE_LOG(LogTemp, Warning, TEXT("Destruyendo planeta"));

    if (CollisionComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("Eliminando colisiones"));
        CollisionComponent->ClearCollision();
    }

    if (NoiseSettings && ClipmapComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("Eliminando referencias restantes"));
        NoiseSettings->OnNoiseSettingsChanged.RemoveAll(ClipmapComponent);
    }

    bInitializedInEditor = false;

    Super::Destroyed();
}

void ACosmicPlanet::BeginDestroy()
{
    UE_LOG(LogTemp, Warning, TEXT("Recolector destruyendo planeta"));

    if (CollisionComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("Eliminando colisiones"));
        CollisionComponent->ClearCollision();
    }

    if (NoiseSettings && ClipmapComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("Eliminando referencias restantes"));
        NoiseSettings->OnNoiseSettingsChanged.RemoveAll(ClipmapComponent);
    }

    Super::BeginDestroy();
}

void ACosmicPlanet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CollisionComponent)
    {
        CollisionComponent->ClearCollision();
    }

    if (NoiseSettings && ClipmapComponent)
    {
        NoiseSettings->OnNoiseSettingsChanged.RemoveAll(ClipmapComponent);
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

        UE_LOG(LogTemp, Warning, TEXT("Regenerando planeta"));
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
    InitClipmap(); // heavy
}

void ACosmicPlanet::UpdateMaterialOnly()
{
    if (ClipmapComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("Actualizando material"));

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
        if (ClipmapComponent->NoiseSettings)
            ClipmapComponent->NoiseSettings->OnNoiseSettingsChanged.RemoveAll(ClipmapComponent);

        UE_LOG(LogTemp, Warning, TEXT("Actualizando delegates"));

        
        ClipmapComponent->NoiseSettings = NoiseSettings;

        if (NoiseSettings) 
        {
            NoiseSettings->OnNoiseSettingsChanged.AddUObject(
                ClipmapComponent,
                &UCosmicClipmapComponent::RequestCompleteMeshUpdate
            );      
        }  

        ClipmapComponent->RequestCompleteMeshUpdate();
    }
}

void ACosmicPlanet::UpdateOcean()
{
    if (!OceanMesh) return;

    OceanMesh->SetVisibility(bHasOcean);

    if (bHasOcean)
    {
        UE_LOG(LogTemp, Warning, TEXT("Actualizando oceano"));

        float PlanetRadiusUnreal = RadiusKm * 100000.0f;
        float TotalOceanRadius = PlanetRadiusUnreal + SeaLevel;
        float SphereScale = TotalOceanRadius / 50.0f;

        OceanMesh->SetWorldScale3D(FVector(SphereScale));
    }
}

#if WITH_EDITOR
void ACosmicPlanet::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    UE_LOG(LogTemp, Warning, TEXT("Construyendo"));

    if (!GetWorld()->IsGameWorld() && !bInitializedInEditor)
    {
        UpdateMaterialOnly();
        UpdateNoiseSettings();
        InitClipmap();      
        UpdateOcean();
    }
}
#endif

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

#if WITH_EDITOR
void ACosmicPlanet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // CAMBIOS LIGEROS (no rebuild)
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, bHasOcean) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, SeaLevel) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, RadiusKm))
    {
        UpdateOcean();
    }

    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, PlanetMainColor1) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, PlanetMainColor2) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, PlanetAltitudeColor) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, MaterialNoiseScale))
    {
        UpdateMaterialOnly();
        return;
    }

    // CAMBIOS RUIDO
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ACosmicPlanet, NoiseSettings))
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