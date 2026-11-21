// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "System/CosmicCelestialBodySpawner.h"
#include "Planet/CosmicPlanet.h"
#include "Planet/CosmicRingComponent.h"
#include "Simulation/CosmicGravityComponent.h"
#include "Simulation/CosmicOrbitComponent.h"
#include "Terrain/CosmicOceanComponent.h"
#include "System/CosmicSystemLayoutManager.h"
#include "CosmicFoliageBiome.h"
#include "CosmicFoliageCollection.h"
#include "CosmicFoliageTypes.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
// #include "Components/PointLightComponent.h"
// #include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstance.h"
#include "UObject/Package.h"
#include "UObject/UObjectThreadContext.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#endif

namespace
{
    UStaticMesh* LoadNatureMesh(const TCHAR* RelativePath)
    {
        const FString FullPath = FString::Printf(
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Mesh/NaturePack/%s"),
            RelativePath);
        return LoadObject<UStaticMesh>(nullptr, *FullPath);
    }

    FCosmicFoliageMesh MakeFoliageMesh(
        UStaticMesh* Mesh,
        ECosmicFoliageLayer Layer,
        int32 InstancesPerKm2,
        float ScaleMin,
        float ScaleMax,
        ECosmicFoliageAlignmentType AlignmentType,
        float RandomRotMax = 5.0f,
        bool bHasCollision = true)
    {
        FCosmicFoliageMesh Item;
        Item.Mesh = Mesh;
        Item.FoliageLayer = Layer;
        Item.InstancesPerKm2 = InstancesPerKm2;
        Item.ScaleMin = ScaleMin;
        Item.ScaleMax = ScaleMax;
        Item.Alignment.Type = AlignmentType;
        Item.RandomRotationMin = 0.0f;
        Item.RandomRotationMax = RandomRotMax;
        Item.bHasCollision = bHasCollision;
        return Item;
    }

    FCosmicFoliageCollectionEntry MakeEntry(
        UCosmicFoliageBiome* Biome,
        float ElevationMinKm,
        float ElevationMaxKm,
        float TempMin,
        float TempMax,
        float HumMin,
        float HumMax,
        float SlopeMin = 0.0f,
        float SlopeMax = 32.0f)
    {
        FCosmicFoliageCollectionEntry Entry;
        Entry.FoliageBiome = Biome;
        Entry.ElevationMinKm = ElevationMinKm;
        Entry.ElevationMaxKm = ElevationMaxKm;
        Entry.TemperatureMin = TempMin;
        Entry.TemperatureMax = TempMax;
        Entry.HumidityMin = HumMin;
        Entry.HumidityMax = HumMax;
        Entry.SlopeMin = SlopeMin;
        Entry.SlopeMax = SlopeMax;
        return Entry;
    }

    UCosmicFoliageBiome* FindOrCreateBiomeAsset(
        const FString& AssetName,
        const TArray<FCosmicFoliageMesh>& DefaultMeshes,
        TArray<UPackage*>& OutPackagesToSave)
    {
        const FString PackageName = FString::Printf(TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/Biomes/%s"), *AssetName);
        const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);

        if (UCosmicFoliageBiome* Existing = LoadObject<UCosmicFoliageBiome>(nullptr, *ObjectPath))
        {
            if (Existing->Foliage.IsEmpty() && !DefaultMeshes.IsEmpty())
            {
                Existing->Foliage = DefaultMeshes;
                Existing->MarkPackageDirty();
                OutPackagesToSave.AddUnique(Existing->GetOutermost());
            }
            return Existing;
        }

#if WITH_EDITOR
        if (FUObjectThreadContext::Get().IsRoutingPostLoad)
        {
            return nullptr;
        }
        UPackage* Package = CreatePackage(*PackageName);
        if (!Package)
        {
            return nullptr;
        }
        Package->FullyLoad();

        UCosmicFoliageBiome* NewBiome = NewObject<UCosmicFoliageBiome>(
            Package,
            UCosmicFoliageBiome::StaticClass(),
            *AssetName,
            RF_Public | RF_Standalone | RF_Transactional);
        if (NewBiome)
        {
            NewBiome->Foliage = DefaultMeshes;
            FAssetRegistryModule::AssetCreated(NewBiome);
            NewBiome->MarkPackageDirty();
            OutPackagesToSave.AddUnique(Package);
        }
        return NewBiome;
#else
        return nullptr;
#endif
    }

    UCosmicFoliageCollection* FindOrCreateCollectionAsset(
        const FString& AssetName,
        const TArray<FCosmicFoliageCollectionEntry>& DefaultEntries,
        TArray<UPackage*>& OutPackagesToSave)
    {
        const FString PackageName = FString::Printf(TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/%s"), *AssetName);
        const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);

        if (UCosmicFoliageCollection* Existing = LoadObject<UCosmicFoliageCollection>(nullptr, *ObjectPath))
        {
            if (Existing->FoliageEntries.IsEmpty() && !DefaultEntries.IsEmpty())
            {
                Existing->FoliageEntries = DefaultEntries;
                Existing->MarkPackageDirty();
                OutPackagesToSave.AddUnique(Existing->GetOutermost());
            }
            return Existing;
        }

#if WITH_EDITOR
        if (FUObjectThreadContext::Get().IsRoutingPostLoad)
        {
            return nullptr;
        }
        UPackage* Package = CreatePackage(*PackageName);
        if (!Package)
        {
            return nullptr;
        }
        Package->FullyLoad();

        UCosmicFoliageCollection* NewCollection = NewObject<UCosmicFoliageCollection>(
            Package,
            UCosmicFoliageCollection::StaticClass(),
            *AssetName,
            RF_Public | RF_Standalone | RF_Transactional);
        if (NewCollection)
        {
            NewCollection->FoliageEntries = DefaultEntries;
            FAssetRegistryModule::AssetCreated(NewCollection);
            NewCollection->MarkPackageDirty();
            OutPackagesToSave.AddUnique(Package);
        }
        return NewCollection;
#else
        return nullptr;
#endif
    }
}

UMaterialInstance* FCosmicCelestialBodySpawner::ResolveDefaultMaterial(const TArray<const TCHAR*>& CandidatePaths)
{
    for (const TCHAR* Path : CandidatePaths)
    {
        if (UMaterialInstance* Mat = LoadObject<UMaterialInstance>(nullptr, Path))
        {
            return Mat;
        }
    }
    return nullptr;
}

UMaterialInstance* FCosmicCelestialBodySpawner::GetDefaultEarthMaterial()
{
    const TArray<const TCHAR*> Candidates = {
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicEarthV2.MI_CosmicEarthV2"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicEarth2.MI_CosmicEarth2"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicEarthV2.MI_CosmicEarthV2"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicEarth2.MI_CosmicEarth2"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicEarth.MI_CosmicEarth"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicRealEarth.MI_CosmicRealEarth"),
        TEXT("/Game/Materials/MI_CosmicEarthV2.MI_CosmicEarthV2")
    };
    return ResolveDefaultMaterial(Candidates);
}

UMaterialInstance* FCosmicCelestialBodySpawner::GetDefaultMoonMaterial()
{
    const TArray<const TCHAR*> Candidates = {
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicMoonV2.MI_CosmicMoonV2"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicMoonV2.MI_CosmicMoonV2"),
        TEXT("/Game/Materials/MI_CosmicMoonV2.MI_CosmicMoonV2"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicMoon.MI_CosmicMoon")
    };
    return ResolveDefaultMaterial(Candidates);
}

UMaterialInstance* FCosmicCelestialBodySpawner::GetDefaultStarMaterial()
{
    const TArray<const TCHAR*> Candidates = {
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicSun.MI_CosmicSun"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicSun.MI_CosmicSun"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicStar.M_CosmicStar")
    };
    return ResolveDefaultMaterial(Candidates);
}

UMaterialInstance* FCosmicCelestialBodySpawner::GetDefaultGasGiantMaterial()
{
    const TArray<const TCHAR*> Candidates = {
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicGasGiant.MI_CosmicGasGiant"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicGasGiant.MI_CosmicGasGiant"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicJupiter.MI_CosmicJupiter")
    };
    return ResolveDefaultMaterial(Candidates);
}

UMaterialInstance* FCosmicCelestialBodySpawner::GetDefaultRingMaterial()
{
    const TArray<const TCHAR*> Candidates = {
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/MI_CosmicRing.MI_CosmicRing"),
        TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicRing.MI_CosmicRing"),
        TEXT("/CosmicArchitect/CosmicArchitect/Resources/Materials/M_CosmicRing.M_CosmicRing")
    };
    return ResolveDefaultMaterial(Candidates);
}

void FCosmicCelestialBodySpawner::EnsureDefaultFoliageAssets()
{
    // Load plugin NaturePack meshes
    UStaticMesh* TreeAutumn    = LoadNatureMesh(TEXT("Trees/SM_CA_Tree_Autumn_01.SM_CA_Tree_Autumn_01"));
    UStaticMesh* TreeBroadleaf = LoadNatureMesh(TEXT("Trees/SM_CA_Tree_Broadleaf_01.SM_CA_Tree_Broadleaf_01"));
    UStaticMesh* TreeCherry    = LoadNatureMesh(TEXT("Trees/SM_CA_Tree_Cherry_01.SM_CA_Tree_Cherry_01"));
    UStaticMesh* TreeDead      = LoadNatureMesh(TEXT("Trees/SM_CA_Tree_Dead_01.SM_CA_Tree_Dead_01"));
    UStaticMesh* TreePalm      = LoadNatureMesh(TEXT("Trees/SM_CA_Tree_Palm_01.SM_CA_Tree_Palm_01"));
    UStaticMesh* TreePine      = LoadNatureMesh(TEXT("Trees/SM_CA_Tree_Pine_01.SM_CA_Tree_Pine_01"));
    UStaticMesh* TreeSavanna   = LoadNatureMesh(TEXT("Trees/SM_CA_Tree_Savanna_01.SM_CA_Tree_Savanna_01"));
    UStaticMesh* TreeSnowyFir  = LoadNatureMesh(TEXT("Trees/SM_CA_Tree_SnowyFir_01.SM_CA_Tree_SnowyFir_01"));

    UStaticMesh* PlantBush     = LoadNatureMesh(TEXT("Plants/SM_CA_Plant_Bush_01.SM_CA_Plant_Bush_01"));
    UStaticMesh* CactusBarrel  = LoadNatureMesh(TEXT("Plants/SM_CA_Plant_Cactus_Barrel_01.SM_CA_Plant_Cactus_Barrel_01"));
    UStaticMesh* CactusSaguaro = LoadNatureMesh(TEXT("Plants/SM_CA_Plant_Cactus_Saguaro_01.SM_CA_Plant_Cactus_Saguaro_01"));
    UStaticMesh* DesertShrub   = LoadNatureMesh(TEXT("Plants/SM_CA_Plant_DesertShrub_01.SM_CA_Plant_DesertShrub_01"));
    UStaticMesh* PlantFern     = LoadNatureMesh(TEXT("Plants/SM_CA_Plant_Fern_01.SM_CA_Plant_Fern_01"));
    UStaticMesh* GrassClump    = LoadNatureMesh(TEXT("Plants/SM_CA_Plant_GrassClump_01.SM_CA_Plant_GrassClump_01"));

    UStaticMesh* RockBoulder   = LoadNatureMesh(TEXT("Rocks/SM_CA_Rock_Boulder_01.SM_CA_Rock_Boulder_01"));
    UStaticMesh* RockCluster   = LoadNatureMesh(TEXT("Rocks/SM_CA_Rock_Cluster_01.SM_CA_Rock_Cluster_01"));
    UStaticMesh* RockFlat      = LoadNatureMesh(TEXT("Rocks/SM_CA_Rock_Flat_01.SM_CA_Rock_Flat_01"));
    UStaticMesh* RockMedium    = LoadNatureMesh(TEXT("Rocks/SM_CA_Rock_Medium_01.SM_CA_Rock_Medium_01"));

    TArray<UPackage*> PackagesToSave;

    // 1. Existing Biomes
    UCosmicFoliageBiome* BiomeBeach = LoadObject<UCosmicFoliageBiome>(
        nullptr, TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/Biomes/CosmicBeach.CosmicBeach"));
    UCosmicFoliageBiome* BiomeForest = LoadObject<UCosmicFoliageBiome>(
        nullptr, TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/Biomes/CosmicForestBiome.CosmicForestBiome"));
    UCosmicFoliageBiome* BiomeRainForest = LoadObject<UCosmicFoliageBiome>(
        nullptr, TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/Biomes/CosmicRainForestBiome.CosmicRainForestBiome"));
    UCosmicFoliageBiome* BiomeSnow = LoadObject<UCosmicFoliageBiome>(
        nullptr, TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/Biomes/CosmicSnowBiome.CosmicSnowBiome"));
    UCosmicFoliageBiome* BiomeDesert = LoadObject<UCosmicFoliageBiome>(
        nullptr, TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/Biomes/DessertBiome.DessertBiome"));

    // 2. New Specialized Biomes using all NaturePack vegetation & rock meshes
    UCosmicFoliageBiome* BiomeBroadleafForest = FindOrCreateBiomeAsset(
        TEXT("CosmicBroadleafForestBiome"),
        {
            MakeFoliageMesh(TreeBroadleaf, ECosmicFoliageLayer::Far, 2600, 1.0f, 2.2f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 6.0f, true),
            MakeFoliageMesh(TreePine,      ECosmicFoliageLayer::Far, 900,  1.0f, 2.0f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 5.0f, true),
            MakeFoliageMesh(PlantBush,     ECosmicFoliageLayer::Medium, 7500, 0.9f, 1.8f, ECosmicFoliageAlignmentType::AlignToGround, 8.0f, false),
            MakeFoliageMesh(RockMedium,    ECosmicFoliageLayer::Medium, 1500, 0.8f, 2.2f, ECosmicFoliageAlignmentType::AlignToGround, 15.0f, true),
            MakeFoliageMesh(GrassClump,    ECosmicFoliageLayer::Near, 160000, 1.0f, 2.0f, ECosmicFoliageAlignmentType::AlignToGround, 5.0f, false),
            MakeFoliageMesh(PlantFern,     ECosmicFoliageLayer::Near, 35000,  0.8f, 1.6f, ECosmicFoliageAlignmentType::AlignToGround, 8.0f, false)
        },
        PackagesToSave);

    UCosmicFoliageBiome* BiomeTropicalJungle = FindOrCreateBiomeAsset(
        TEXT("CosmicTropicalJungleBiome"),
        {
            MakeFoliageMesh(TreeBroadleaf, ECosmicFoliageLayer::Far, 2200, 1.2f, 2.5f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 8.0f, true),
            MakeFoliageMesh(TreePalm,      ECosmicFoliageLayer::Far, 1800, 1.0f, 2.2f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 12.0f, true),
            MakeFoliageMesh(PlantBush,     ECosmicFoliageLayer::Medium, 11000, 1.0f, 2.2f, ECosmicFoliageAlignmentType::AlignToGround, 10.0f, false),
            MakeFoliageMesh(RockCluster,   ECosmicFoliageLayer::Medium, 2000,  0.6f, 1.8f, ECosmicFoliageAlignmentType::AlignToGround, 10.0f, true),
            MakeFoliageMesh(PlantFern,     ECosmicFoliageLayer::Near, 80000,  1.0f, 2.2f, ECosmicFoliageAlignmentType::AlignToGround, 10.0f, false),
            MakeFoliageMesh(GrassClump,    ECosmicFoliageLayer::Near, 130000, 1.0f, 2.0f, ECosmicFoliageAlignmentType::AlignToGround, 5.0f, false)
        },
        PackagesToSave);

    UCosmicFoliageBiome* BiomeTaigaPine = FindOrCreateBiomeAsset(
        TEXT("CosmicTaigaPineBiome"),
        {
            MakeFoliageMesh(TreePine,     ECosmicFoliageLayer::Far, 3000, 1.0f, 2.2f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 6.0f, true),
            MakeFoliageMesh(TreeSnowyFir, ECosmicFoliageLayer::Far, 800,  0.9f, 1.8f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 6.0f, true),
            MakeFoliageMesh(RockBoulder,  ECosmicFoliageLayer::Medium, 1800, 0.8f, 2.4f, ECosmicFoliageAlignmentType::AlignToGround, 20.0f, true),
            MakeFoliageMesh(PlantBush,    ECosmicFoliageLayer::Medium, 4500, 0.8f, 1.5f, ECosmicFoliageAlignmentType::AlignToGround, 8.0f, false),
            MakeFoliageMesh(PlantFern,    ECosmicFoliageLayer::Near, 35000, 0.6f, 1.4f, ECosmicFoliageAlignmentType::AlignToGround, 8.0f, false)
        },
        PackagesToSave);

    UCosmicFoliageBiome* BiomeAutumnForest = FindOrCreateBiomeAsset(
        TEXT("CosmicAutumnForestBiome"),
        {
            MakeFoliageMesh(TreeAutumn, ECosmicFoliageLayer::Far, 2500, 1.0f, 2.2f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 6.0f, true),
            MakeFoliageMesh(TreeCherry, ECosmicFoliageLayer::Far, 1200, 1.0f, 2.0f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 6.0f, true),
            MakeFoliageMesh(RockFlat,   ECosmicFoliageLayer::Medium, 2000, 0.8f, 2.5f, ECosmicFoliageAlignmentType::AlignToGround, 10.0f, true),
            MakeFoliageMesh(PlantBush,  ECosmicFoliageLayer::Medium, 6000, 0.8f, 1.6f, ECosmicFoliageAlignmentType::AlignToGround, 8.0f, false),
            MakeFoliageMesh(GrassClump, ECosmicFoliageLayer::Near, 160000, 0.9f, 1.8f, ECosmicFoliageAlignmentType::AlignToGround, 5.0f, false)
        },
        PackagesToSave);

    UCosmicFoliageBiome* BiomeSavannaScrub = FindOrCreateBiomeAsset(
        TEXT("CosmicSavannaScrubBiome"),
        {
            MakeFoliageMesh(TreeSavanna, ECosmicFoliageLayer::Far, 1200, 1.0f, 2.2f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 6.0f, true),
            MakeFoliageMesh(TreeDead,    ECosmicFoliageLayer::Far, 350,  0.9f, 1.6f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 10.0f, true),
            MakeFoliageMesh(RockMedium,  ECosmicFoliageLayer::Medium, 2500, 0.7f, 2.2f, ECosmicFoliageAlignmentType::AlignToGround, 15.0f, true),
            MakeFoliageMesh(PlantBush,   ECosmicFoliageLayer::Medium, 4000, 0.8f, 1.5f, ECosmicFoliageAlignmentType::AlignToGround, 8.0f, false),
            MakeFoliageMesh(GrassClump,  ECosmicFoliageLayer::Near, 80000, 0.8f, 1.6f, ECosmicFoliageAlignmentType::AlignToGround, 5.0f, false),
            MakeFoliageMesh(DesertShrub, ECosmicFoliageLayer::Near, 35000, 0.8f, 1.8f, ECosmicFoliageAlignmentType::AlignToGround, 8.0f, false)
        },
        PackagesToSave);

    UCosmicFoliageBiome* BiomeAridCanyon = FindOrCreateBiomeAsset(
        TEXT("CosmicAridCanyonBiome"),
        {
            MakeFoliageMesh(CactusSaguaro, ECosmicFoliageLayer::Far, 400, 1.0f, 2.8f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 6.0f, true),
            MakeFoliageMesh(RockBoulder,   ECosmicFoliageLayer::Far, 550, 1.5f, 3.5f, ECosmicFoliageAlignmentType::AlignToGround, 25.0f, true),
            MakeFoliageMesh(RockFlat,      ECosmicFoliageLayer::Medium, 3800, 1.0f, 3.0f, ECosmicFoliageAlignmentType::AlignToGround, 12.0f, true),
            MakeFoliageMesh(CactusBarrel,  ECosmicFoliageLayer::Medium, 2500, 1.2f, 2.6f, ECosmicFoliageAlignmentType::AlignToGround, 10.0f, false),
            MakeFoliageMesh(DesertShrub,   ECosmicFoliageLayer::Near, 25000, 0.8f, 2.0f, ECosmicFoliageAlignmentType::AlignToGround, 8.0f, false)
        },
        PackagesToSave);

    UCosmicFoliageBiome* BiomeRockyBarren = FindOrCreateBiomeAsset(
        TEXT("CosmicRockyBarrenBiome"),
        {
            MakeFoliageMesh(RockBoulder, ECosmicFoliageLayer::Far, 850, 1.4f, 3.8f, ECosmicFoliageAlignmentType::AlignToGround, 45.0f, true),
            MakeFoliageMesh(RockCluster, ECosmicFoliageLayer::Medium, 5500, 0.8f, 2.6f, ECosmicFoliageAlignmentType::AlignToGround, 25.0f, true),
            MakeFoliageMesh(RockFlat,    ECosmicFoliageLayer::Medium, 4200, 0.8f, 2.8f, ECosmicFoliageAlignmentType::AlignToGround, 15.0f, true),
            MakeFoliageMesh(RockMedium,  ECosmicFoliageLayer::Near, 22000, 0.35f, 1.4f, ECosmicFoliageAlignmentType::AlignToGround, 60.0f, false)
        },
        PackagesToSave);

    UCosmicFoliageBiome* BiomeVolcanicWaste = FindOrCreateBiomeAsset(
        TEXT("CosmicVolcanicWasteBiome"),
        {
            MakeFoliageMesh(RockBoulder, ECosmicFoliageLayer::Far, 1000, 1.5f, 4.0f, ECosmicFoliageAlignmentType::AlignToGround, 40.0f, true),
            MakeFoliageMesh(TreeDead,    ECosmicFoliageLayer::Far, 400,  0.8f, 1.8f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 14.0f, true),
            MakeFoliageMesh(RockCluster, ECosmicFoliageLayer::Medium, 6500, 0.8f, 2.8f, ECosmicFoliageAlignmentType::AlignToGround, 25.0f, true),
            MakeFoliageMesh(RockMedium,  ECosmicFoliageLayer::Near, 20000, 0.4f, 1.5f, ECosmicFoliageAlignmentType::AlignToGround, 45.0f, false),
            MakeFoliageMesh(DesertShrub, ECosmicFoliageLayer::Near, 7500,  0.6f, 1.4f, ECosmicFoliageAlignmentType::AlignToGround, 10.0f, false)
        },
        PackagesToSave);

    UCosmicFoliageBiome* BiomeGlacialRock = FindOrCreateBiomeAsset(
        TEXT("CosmicGlacialRockBiome"),
        {
            MakeFoliageMesh(TreeSnowyFir, ECosmicFoliageLayer::Far, 1400, 0.9f, 2.0f, ECosmicFoliageAlignmentType::AlignToPlanetNormal, 8.0f, true),
            MakeFoliageMesh(RockBoulder,  ECosmicFoliageLayer::Far, 650,  1.4f, 3.4f, ECosmicFoliageAlignmentType::AlignToGround, 30.0f, true),
            MakeFoliageMesh(RockFlat,     ECosmicFoliageLayer::Medium, 5000, 0.8f, 2.5f, ECosmicFoliageAlignmentType::AlignToGround, 15.0f, true),
            MakeFoliageMesh(RockCluster,  ECosmicFoliageLayer::Medium, 3800, 0.6f, 2.2f, ECosmicFoliageAlignmentType::AlignToGround, 20.0f, true),
            MakeFoliageMesh(PlantFern,    ECosmicFoliageLayer::Near, 15000, 0.4f, 1.0f, ECosmicFoliageAlignmentType::AlignToGround, 8.0f, false)
        },
        PackagesToSave);

    // 3. Create Multiple FoliageCollections per Planet Type

    // --- A. Earth-Like / Lush Oceanic Planet Collections ( alongside BigPlanetFoliage ) ---
    FindOrCreateCollectionAsset(
        TEXT("FC_EarthLike_LushContinental"),
        {
            MakeEntry(BiomeBeach ? BiomeBeach : BiomeTropicalJungle, 0.070f, 0.083f, 0.20f, 1.00f, 0.00f, 1.00f, 0.0f, 26.0f),
            MakeEntry(BiomeBroadleafForest,                          0.081f, 1.500f, 0.22f, 0.78f, 0.35f, 1.00f, 0.0f, 32.0f),
            MakeEntry(BiomeSavannaScrub,                             0.081f, 1.400f, 0.32f, 0.90f, 0.14f, 0.42f, 0.0f, 30.0f),
            MakeEntry(BiomeDesert ? BiomeDesert : BiomeAridCanyon,   0.081f, 1.200f, 0.40f, 1.00f, 0.00f, 0.18f, 0.0f, 28.0f),
            MakeEntry(BiomeTaigaPine,                                0.085f, 1.600f, 0.12f, 0.30f, 0.20f, 1.00f, 0.0f, 34.0f),
            MakeEntry(BiomeSnow ? BiomeSnow : BiomeGlacialRock,      0.115f, 2.500f, 0.00f, 0.22f, 0.05f, 1.00f, 0.0f, 35.0f)
        },
        PackagesToSave);

    FindOrCreateCollectionAsset(
        TEXT("FC_EarthLike_TropicalJungle"),
        {
            MakeEntry(BiomeBeach ? BiomeBeach : BiomeTropicalJungle, 0.070f, 0.085f, 0.18f, 1.00f, 0.00f, 1.00f, 0.0f, 28.0f),
            MakeEntry(BiomeTropicalJungle,                           0.081f, 1.600f, 0.24f, 1.00f, 0.34f, 1.00f, 0.0f, 34.0f),
            MakeEntry(BiomeSavannaScrub,                             0.081f, 1.400f, 0.25f, 1.00f, 0.10f, 0.38f, 0.0f, 30.0f),
            MakeEntry(BiomeBroadleafForest,                          0.085f, 1.600f, 0.15f, 0.34f, 0.28f, 1.00f, 0.0f, 32.0f),
            MakeEntry(BiomeSnow ? BiomeSnow : BiomeGlacialRock,      0.125f, 2.500f, 0.00f, 0.18f, 0.00f, 1.00f, 0.0f, 35.0f)
        },
        PackagesToSave);

    FindOrCreateCollectionAsset(
        TEXT("FC_EarthLike_AutumnHighlands"),
        {
            MakeEntry(BiomeBeach ? BiomeBeach : BiomeAutumnForest,   0.070f, 0.082f, 0.20f, 1.00f, 0.00f, 1.00f, 0.0f, 25.0f),
            MakeEntry(BiomeAutumnForest,                             0.080f, 1.400f, 0.18f, 0.78f, 0.30f, 1.00f, 0.0f, 32.0f),
            MakeEntry(BiomeForest ? BiomeForest : BiomeBroadleafForest, 0.080f, 1.400f, 0.24f, 0.88f, 0.42f, 1.00f, 0.0f, 30.0f),
            MakeEntry(BiomeSavannaScrub,                             0.080f, 1.200f, 0.30f, 1.00f, 0.08f, 0.32f, 0.0f, 28.0f),
            MakeEntry(BiomeTaigaPine,                                0.085f, 1.700f, 0.10f, 0.28f, 0.18f, 1.00f, 0.0f, 35.0f),
            MakeEntry(BiomeSnow ? BiomeSnow : BiomeGlacialRock,      0.110f, 2.500f, 0.00f, 0.22f, 0.00f, 1.00f, 0.0f, 35.0f)
        },
        PackagesToSave);

    // --- B. Arid / Desert / Savanna / Canyon Planet Collections ---
    FindOrCreateCollectionAsset(
        TEXT("FC_Desert_CanyonMesa"),
        {
            MakeEntry(BiomeDesert ? BiomeDesert : BiomeAridCanyon, -2.0f, 0.45f, 0.15f, 1.00f, 0.00f, 0.65f, 0.0f, 26.0f),
            MakeEntry(BiomeAridCanyon,                             -1.5f, 2.50f, 0.00f, 1.00f, 0.00f, 1.00f, 0.0f, 42.0f),
            MakeEntry(BiomeRockyBarren,                            -2.5f, 3.00f, 0.00f, 1.00f, 0.00f, 1.00f, 22.0f, 75.0f)
        },
        PackagesToSave);

    FindOrCreateCollectionAsset(
        TEXT("FC_Desert_SavannaBadlands"),
        {
            MakeEntry(BiomeSavannaScrub,                                 -1.5f, 0.80f, 0.15f, 1.00f, 0.24f, 1.00f, 0.0f, 28.0f),
            MakeEntry(BiomeRainForest ? BiomeRainForest : BiomeSavannaScrub, -1.5f, 1.20f, 0.20f, 1.00f, 0.10f, 0.55f, 0.0f, 30.0f),
            MakeEntry(BiomeAridCanyon,                                   -2.0f, 2.50f, 0.20f, 1.00f, 0.00f, 0.35f, 0.0f, 40.0f)
        },
        PackagesToSave);

    FindOrCreateCollectionAsset(
        TEXT("FC_Desert_DuneWasteland"),
        {
            MakeEntry(BiomeDesert ? BiomeDesert : BiomeAridCanyon, -2.0f, 1.50f, 0.10f, 1.00f, 0.00f, 0.55f, 0.0f, 25.0f),
            MakeEntry(BiomeRockyBarren,                            -2.5f, 3.00f, 0.00f, 1.00f, 0.00f, 1.00f, 0.0f, 65.0f)
        },
        PackagesToSave);

    // --- C. Frozen / Tundra / Glacial Planet Collections ---
    FindOrCreateCollectionAsset(
        TEXT("FC_Frozen_BorealTundra"),
        {
            MakeEntry(BiomeSnow ? BiomeSnow : BiomeGlacialRock, -1.5f, 2.50f, 0.00f, 0.45f, 0.10f, 1.00f, 0.0f, 35.0f),
            MakeEntry(BiomeTaigaPine,                           -1.0f, 1.50f, 0.15f, 0.70f, 0.22f, 1.00f, 0.0f, 32.0f),
            MakeEntry(BiomeGlacialRock,                         -2.0f, 3.00f, 0.00f, 1.00f, 0.00f, 1.00f, 0.0f, 55.0f)
        },
        PackagesToSave);

    FindOrCreateCollectionAsset(
        TEXT("FC_Frozen_GlacialWastes"),
        {
            MakeEntry(BiomeGlacialRock, -2.5f, 3.00f, 0.00f, 0.65f, 0.00f, 1.00f, 0.0f, 50.0f),
            MakeEntry(BiomeRockyBarren, -3.0f, 3.50f, 0.00f, 1.00f, 0.00f, 1.00f, 10.0f, 75.0f)
        },
        PackagesToSave);

    // --- D. Cratered / Barren / Volcanic Planet & Moon Collections ---
    FindOrCreateCollectionAsset(
        TEXT("FC_Cratered_RockyBarren"),
        {
            MakeEntry(BiomeRockyBarren, -5.0f, 5.00f, 0.00f, 1.00f, 0.00f, 1.00f, 0.0f, 75.0f)
        },
        PackagesToSave);

    FindOrCreateCollectionAsset(
        TEXT("FC_Volcanic_ScorchedWaste"),
        {
            MakeEntry(BiomeVolcanicWaste, -3.5f, 3.50f, 0.00f, 1.00f, 0.00f, 1.00f, 0.0f, 45.0f),
            MakeEntry(BiomeRockyBarren,   -5.0f, 5.00f, 0.00f, 1.00f, 0.00f, 1.00f, 18.0f, 80.0f)
        },
        PackagesToSave);

#if WITH_EDITOR
    if (!PackagesToSave.IsEmpty() && !FUObjectThreadContext::Get().IsRoutingPostLoad)
    {
        UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);
    }
#endif
}

TArray<UCosmicFoliageCollection*> FCosmicCelestialBodySpawner::GetDefaultFoliageCollectionsForArchetype(ECosmicPlanetArchetype Archetype)
{
    EnsureDefaultFoliageAssets();

    TArray<const TCHAR*> Paths;
    switch (Archetype)
    {
    case ECosmicPlanetArchetype::EarthLike:
        Paths = {
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/BigPlanetFoliage.BigPlanetFoliage"),
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/FC_EarthLike_LushContinental.FC_EarthLike_LushContinental"),
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/FC_EarthLike_TropicalJungle.FC_EarthLike_TropicalJungle"),
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/FC_EarthLike_AutumnHighlands.FC_EarthLike_AutumnHighlands")
        };
        break;

    case ECosmicPlanetArchetype::AridDesert:
        Paths = {
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/FC_Desert_CanyonMesa.FC_Desert_CanyonMesa"),
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/FC_Desert_SavannaBadlands.FC_Desert_SavannaBadlands"),
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/FC_Desert_DuneWasteland.FC_Desert_DuneWasteland")
        };
        break;

    case ECosmicPlanetArchetype::FrozenIce:
        Paths = {
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/FC_Frozen_BorealTundra.FC_Frozen_BorealTundra"),
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/FC_Frozen_GlacialWastes.FC_Frozen_GlacialWastes")
        };
        break;

    case ECosmicPlanetArchetype::VolcanicScorched:
        Paths = {
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/FC_Volcanic_ScorchedWaste.FC_Volcanic_ScorchedWaste"),
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/FC_Cratered_RockyBarren.FC_Cratered_RockyBarren")
        };
        break;

    case ECosmicPlanetArchetype::CrateredBarren:
    case ECosmicPlanetArchetype::Moon:
    default:
        Paths = {
            TEXT("/CosmicArchitect/CosmicArchitect/Resources/Foliage/FC_Cratered_RockyBarren.FC_Cratered_RockyBarren")
        };
        break;
    }

    TArray<UCosmicFoliageCollection*> Result;
    for (const TCHAR* Path : Paths)
    {
        if (UCosmicFoliageCollection* Col = LoadObject<UCosmicFoliageCollection>(nullptr, Path))
        {
            Result.Add(Col);
        }
    }
    return Result;
}

UCosmicFoliageCollection* FCosmicCelestialBodySpawner::SelectFoliageCollection(
    const TArray<TObjectPtr<UCosmicFoliageCollection>>& Candidates,
    ECosmicPlanetArchetype FallbackArchetype,
    FRandomStream& Stream)
{
    TArray<UCosmicFoliageCollection*> ValidCandidates;
    for (const TObjectPtr<UCosmicFoliageCollection>& Item : Candidates)
    {
        if (Item)
        {
            ValidCandidates.Add(Item.Get());
        }
    }

    if (ValidCandidates.IsEmpty())
    {
        ValidCandidates = GetDefaultFoliageCollectionsForArchetype(FallbackArchetype);
    }

    if (ValidCandidates.IsEmpty())
    {
        return nullptr;
    }

    const int32 Index = Stream.RandRange(0, ValidCandidates.Num() - 1);
    return ValidCandidates[Index];
}

void FCosmicCelestialBodySpawner::InitializePlanetAppearance(
    ACosmicPlanet* Planet,
    float RadiusKm,
    UCosmicNoiseClass* NoiseSettings,
    const FCosmicPlanetAppearanceParams& Appearance,
    UMaterialInstance* Material,
    bool bUseClipmap,
    int32 ClipResolution,
    int32 NumLevels,
    int32 MinTriangleSize,
    float HeightVisibility,
    bool bHasOcean,
    double OceanSeaLevel,
    int32 OceanResolution,
    UMaterialInstance* OceanMaterial,
    UCosmicFoliageCollection* FoliageCollection)
{
    if (!Planet)
    {
        return;
    }

    if (FoliageCollection)
    {
        // Configure streaming radii matching EarthPlanet so trees and rocks are visible from descent
        Planet->SetFoliageParams(64, 0.08f, 0.25f, 1.50f);
    }

    Planet->InitPlanet(
        RadiusKm,
        NoiseSettings,
        Appearance,
        Material,
        bUseClipmap,
        ClipResolution,
        NumLevels,
        MinTriangleSize,
        HeightVisibility,
        bHasOcean,
        OceanSeaLevel,
        OceanResolution,
        OceanMaterial,
        FoliageCollection
    );
}

ACosmicPlanet* FCosmicCelestialBodySpawner::SpawnStar(
    UWorld* World,
    AActor* OwnerActor,
    const FVector& Location,
    float StarRadiusKm,
    UMaterialInstance* StarMaterial,
    FRandomStream& Stream)
{
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ACosmicPlanet* Star = World->SpawnActor<ACosmicPlanet>(
        ACosmicPlanet::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
    if (!Star)
    {
        return nullptr;
    }

    const FCosmicPlanetAppearanceParams StarAppearance = FCosmicPlanetAppearanceParams::MakeStarAppearance(Stream);

    InitializePlanetAppearance(
        Star,
        StarRadiusKm,
        nullptr,
        StarAppearance,
        StarMaterial,
        false, 128, 0, 0, 0.0f,
        false, 0.0, 0, nullptr,
        nullptr
    );

    if (OwnerActor)
    {
        Star->AttachToActor(OwnerActor, FAttachmentTransformRules::KeepWorldTransform);
    }

    UCosmicGravityComponent* StarGravity = NewObject<UCosmicGravityComponent>(Star);
    StarGravity->RegisterComponent();
    StarGravity->IsPlanet = true;
    StarGravity->RadiusKm = StarRadiusKm;
    StarGravity->SurfaceGravity = 274.0f;
    StarGravity->GravityMode = ECosmicGravityMode::None;
    Star->AddInstanceComponent(StarGravity);

    return Star;
}

ADirectionalLight* FCosmicCelestialBodySpawner::SpawnStarDirectionalLight(
    UWorld* World,
    ACosmicPlanet* Star)
{
    if (!World || !Star)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ADirectionalLight* DirectionalLight = World->SpawnActor<ADirectionalLight>(
        ADirectionalLight::StaticClass(),
        Star->GetActorLocation(),
        Star->GetActorRotation(),
        SpawnParams
    );

    if (DirectionalLight)
    {
        DirectionalLight->AttachToActor(Star, FAttachmentTransformRules::KeepWorldTransform);
        if (UDirectionalLightComponent* DL = Cast<UDirectionalLightComponent>(DirectionalLight->GetLightComponent()))
        {
            DL->Intensity = 5.0f;
            DL->MarkRenderStateDirty();
        }
    }

    return DirectionalLight;
}

// APointLight* FCosmicCelestialBodySpawner::SpawnStarLight(
//     UWorld* World,
//     ACosmicPlanet* Star,
//     float SystemRadiusKm,
//     float MaxBodyRadiusKm,
//     float MoonOrbitDistanceFactorMax,
//     float MoonDiameterMaxKm)
// {
//     if (!World || !Star)
//     {
//         return nullptr;
//     }
//
//     FActorSpawnParameters SpawnParams;
//     SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
//
//     APointLight* PointLight = World->SpawnActor<APointLight>(
//         APointLight::StaticClass(),
//         Star->GetActorLocation(),
//         Star->GetActorRotation(),
//         SpawnParams
//     );
//
//     if (PointLight)
//     {
//         PointLight->AttachToActor(Star, FAttachmentTransformRules::KeepWorldTransform);
//         if (UPointLightComponent* PL = Cast<UPointLightComponent>(PointLight->GetLightComponent()))
//         {
//             const float MaxMoonReachKm = MaxBodyRadiusKm * MoonOrbitDistanceFactorMax + (MoonDiameterMaxKm * 0.5f);
//             const float MaxRingReachKm = MaxBodyRadiusKm * 2.8f;
//             const float LightRangeKm = FMath::Max(SystemRadiusKm + FMath::Max(MaxMoonReachKm, MaxRingReachKm), SystemRadiusKm * 1.1f);
//
//             PL->AttenuationRadius = LightRangeKm * 100000.0f;
//             PL->Intensity = 5.0f;
//             PL->bUseInverseSquaredFalloff = false;
//             PL->LightFalloffExponent = 1.0f;
//             PL->MarkRenderStateDirty();
//         }
//     }
//
//     return PointLight;
// }

ACosmicPlanet* FCosmicCelestialBodySpawner::SpawnPlanet(
    UWorld* World,
    AActor* OwnerActor,
    ACosmicPlanet* Star,
    const FVector& Location,
    float OrbitKm,
    float RadiusKm,
    const FCosmicBodyClassification& Classification,
    UCosmicNoiseClass* NoiseSettings,
    UMaterialInstance* PlanetMaterial,
    UMaterialInstance* OceanMaterial,
    UMaterialInstance* RingMaterial,
    const FCosmicPlanetAppearanceParams& Appearance,
    const FCosmicSystemGraphicsConfig& GraphicsConfig,
    const FVector2D& BodyDiameterRangeKm,
    const FVector2D& PlanetGravityRange,
    FRandomStream& Stream,
    UCosmicFoliageCollection* FoliageCollection)
{
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ACosmicPlanet* Planet = World->SpawnActor<ACosmicPlanet>(
        ACosmicPlanet::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
    if (!Planet)
    {
        return nullptr;
    }

    if (OwnerActor)
    {
        Planet->AttachToActor(OwnerActor, FAttachmentTransformRules::KeepWorldTransform);
    }

    const bool bIsGasGiant = (Classification.Type == ECosmicSystemPlanetType::GasGiant);
    const int32 ClipRes = bIsGasGiant ? GraphicsConfig.GasGiantClipResolution : GraphicsConfig.TelluricClipResolution;
    const int32 OceanRes = Classification.bHasOcean ? GraphicsConfig.OceanResolutionWithOcean : GraphicsConfig.OceanResolutionWithoutOcean;

    // Pre-configure natural ocean wave and color parameters before InitPlanet builds the ocean material
    if (Classification.bHasOcean && Planet->OceanComponent)
    {
        Planet->OceanComponent->WaveHeight = Stream.FRandRange(45.0f, 80.0f);
        Planet->OceanComponent->WaveLength = Stream.FRandRange(1800.0f, 2800.0f);
        Planet->OceanComponent->WaveSteepness = Stream.FRandRange(0.75f, 1.0f);
        Planet->OceanComponent->WaveSpeed = Stream.FRandRange(0.18f, 0.30f);
        Planet->OceanComponent->WaveChop = Stream.FRandRange(1.2f, 1.7f);
        Planet->OceanComponent->WaveSpread = Stream.FRandRange(1.4f, 2.0f);

        Planet->OceanComponent->WaterColor = FLinearColor(
            Stream.FRandRange(0.04f, 0.09f),
            Stream.FRandRange(0.28f, 0.42f),
            Stream.FRandRange(0.44f, 0.55f), 1.0f);
        Planet->OceanComponent->WaterScattering = FLinearColor(
            Stream.FRandRange(0.12f, 0.22f),
            Stream.FRandRange(0.38f, 0.50f),
            Stream.FRandRange(0.42f, 0.52f), 1.0f);
        Planet->OceanComponent->WaterAbsortion = FLinearColor(0.515f, 0.087f, 0.013f, 1.0f);
    }

    InitializePlanetAppearance(
        Planet,
        RadiusKm,
        bIsGasGiant ? nullptr : NoiseSettings,
        Appearance,
        PlanetMaterial,
        !bIsGasGiant,
        ClipRes,
        bIsGasGiant ? 1 : 7,
        100,
        bIsGasGiant ? 0.0f : 2.5f,
        Classification.bHasOcean,
        Classification.OceanSeaLevel,
        OceanRes,
        Classification.bHasOcean ? OceanMaterial : nullptr,
        bIsGasGiant ? nullptr : FoliageCollection
    );

    UCosmicGravityComponent* Gravity = NewObject<UCosmicGravityComponent>(Planet);
    Gravity->RegisterComponent();
    Gravity->IsPlanet = true;
    Gravity->RadiusKm = RadiusKm;
    Gravity->SurfaceGravity = FCosmicSystemLayoutManager::CalculateSurfaceGravity(
        RadiusKm, BodyDiameterRangeKm * 0.5f, PlanetGravityRange);
    Gravity->GravityMode = ECosmicGravityMode::None;
    Planet->AddInstanceComponent(Gravity);

    if (Star)
    {
        UCosmicOrbitComponent* Orbit = NewObject<UCosmicOrbitComponent>(Planet);
        Orbit->RegisterComponent();
        Orbit->ParentBody = Star;
        Orbit->SemiMajorAxisKm = OrbitKm;
        Orbit->Eccentricity = Stream.FRandRange(0.0f, 0.15f);
        Orbit->InclinationX = Stream.FRandRange(0.0f, 10.0f);
        Orbit->InitialPosition = Stream.FRandRange(0.0f, 1.0f);
        Orbit->OrbitalPeriod = FMath::Pow(OrbitKm, 2.0f);
        Orbit->InitOrbit(FCosmicPlanetAppearanceParams::GetRandomColor(Stream, 50, 255));
        Planet->AddInstanceComponent(Orbit);
    }

    if (Classification.bHasRings && RingMaterial)
    {
        UCosmicRingComponent* Ring = NewObject<UCosmicRingComponent>(Planet);
        Ring->RegisterComponent();
        Ring->AttachToComponent(Planet->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        Ring->InnerRadiusKM = RadiusKm * 1.4f;
        Ring->OuterRadiusKM = RadiusKm * 2.8f;
        Ring->BandFrequency = Stream.FRandRange(50.0f, 200.0f);
        Ring->RingRotation = FRotator(Stream.FRandRange(-15.0f, 15.0f), 0.0f, 0.0f);
        Ring->SectorAngleDegrees = 15;
        Ring->VisibleSectors = 3;
        Ring->MaxInstancesPerSecond = 500;
        Ring->MinScale = 0.1f;
        Ring->MaxScale = 0.3f;
        Ring->AsteroidActivationDistanceKM = (Ring->OuterRadiusKM - Ring->InnerRadiusKM) / 2.0f;
        Ring->RingColor = FLinearColor(
            Stream.FRandRange(0.4f, 0.9f),
            Stream.FRandRange(0.3f, 0.8f),
            Stream.FRandRange(0.1f, 0.5f), 1.0f);
        Ring->MacroRingMaterial = RingMaterial;
        Planet->AddInstanceComponent(Ring);
    }

    return Planet;
}

ACosmicPlanet* FCosmicCelestialBodySpawner::SpawnMoon(
    UWorld* World,
    ACosmicPlanet* ParentPlanet,
    float OrbitKm,
    float RadiusKm,
    UCosmicNoiseClass* NoiseSettings,
    UMaterialInstance* MoonMaterial,
    const FCosmicPlanetAppearanceParams& Appearance,
    const FVector2D& MoonDiameterRangeKm,
    const FVector2D& MoonGravityRange,
    FRandomStream& Stream,
    UCosmicFoliageCollection* FoliageCollection)
{
    if (!World || !ParentPlanet)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ACosmicPlanet* Moon = World->SpawnActor<ACosmicPlanet>(
        ACosmicPlanet::StaticClass(),
        ParentPlanet->GetActorLocation(),
        FRotator::ZeroRotator,
        SpawnParams
    );
    if (!Moon)
    {
        return nullptr;
    }

    Moon->AttachToActor(ParentPlanet, FAttachmentTransformRules::KeepWorldTransform);

    InitializePlanetAppearance(
        Moon,
        RadiusKm,
        NoiseSettings,
        Appearance,
        MoonMaterial,
        true, 128, 6, 100, 2.5f,
        false, 0.0, 64, nullptr,
        FoliageCollection
    );

    UCosmicGravityComponent* MoonGravity = NewObject<UCosmicGravityComponent>(Moon);
    MoonGravity->RegisterComponent();
    MoonGravity->SetIsPlanet(true);
    MoonGravity->RadiusKm = RadiusKm;
    MoonGravity->SurfaceGravity = FCosmicSystemLayoutManager::CalculateSurfaceGravity(
        RadiusKm, MoonDiameterRangeKm * 0.5f, MoonGravityRange);
    Moon->AddInstanceComponent(MoonGravity);

    UCosmicOrbitComponent* MoonOrbit = NewObject<UCosmicOrbitComponent>(Moon);
    MoonOrbit->RegisterComponent();
    MoonOrbit->ParentBody = ParentPlanet;
    MoonOrbit->SemiMajorAxisKm = OrbitKm;
    MoonOrbit->Eccentricity = Stream.FRandRange(0.0f, 0.1f);
    MoonOrbit->InitialPosition = Stream.FRandRange(0.0f, 1.0f);
    MoonOrbit->OrbitalPeriod = FMath::Pow(OrbitKm, 8.0f);
    MoonOrbit->InitOrbit(FCosmicPlanetAppearanceParams::GetRandomColor(Stream, 50, 255));
    Moon->AddInstanceComponent(MoonOrbit);

    return Moon;
}

UCosmicRingComponent* FCosmicCelestialBodySpawner::SpawnAsteroidBelt(
    ACosmicPlanet* Star,
    float OrbitKm,
    UMaterialInstance* RingMaterial,
    FRandomStream& Stream)
{
    if (!Star)
    {
        return nullptr;
    }

    UCosmicRingComponent* Belt = NewObject<UCosmicRingComponent>(Star);
    Belt->RegisterComponent();
    Belt->AttachToComponent(Star->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    Belt->InnerRadiusKM = OrbitKm * 0.9f;
    Belt->OuterRadiusKM = OrbitKm * 1.1f;
    Belt->BandFrequency = Stream.FRandRange(50.0f, 200.0f);
    Belt->RingThicknessKM = Stream.FRandRange(0.4f, 0.8f);
    Belt->SectorAngleDegrees = 4;
    Belt->VisibleSectors = 3;
    Belt->MaxInstancesPerSecond = 500;
    Belt->MinScale = 0.1f;
    Belt->MaxScale = 0.5f;
    Belt->AsteroidActivationDistanceKM = (Belt->OuterRadiusKM - Belt->InnerRadiusKM) / 4.0f;
    Belt->RingColor = FLinearColor(
        Stream.FRandRange(0.003f, 0.007f),
        Stream.FRandRange(0.002f, 0.005f),
        Stream.FRandRange(0.001f, 0.003f), 1.0f);
    Belt->MacroRingMaterial = RingMaterial;
    Star->AddInstanceComponent(Belt);

    return Belt;
}

void FCosmicCelestialBodySpawner::ClearBodies(TArray<AActor*>& Bodies)
{
    for (AActor* Actor : Bodies)
    {
        if (Actor)
        {
            Actor->Destroy();
        }
    }
    Bodies.Empty();
}
