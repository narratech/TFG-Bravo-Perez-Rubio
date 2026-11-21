// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/CosmicSystemTypes.h"

class AActor;
class ACosmicPlanet;
class APointLight;
class UCosmicNoiseClass;
class UCosmicRingComponent;
class UCosmicFoliageCollection;
class UMaterialInstance;

/**
 * Handles creation, component composition, material resolution, foliage asset provisioning,
 * and appearance initialization of celestial bodies (stars, planets, moons, asteroid belts).
 */
class COSMICARCHITECTRUNTIME_API FCosmicCelestialBodySpawner
{
public:
    /** Resolves and loads the first valid Material Instance from a list of candidate package paths. */
    static UMaterialInstance* ResolveDefaultMaterial(const TArray<const TCHAR*>& CandidatePaths);

    /** Loads default MI_CosmicEarthV2 (or MI_CosmicEarth2 / fallbacks). */
    static UMaterialInstance* GetDefaultEarthMaterial();

    /** Loads default MI_CosmicMoonV2 (or fallbacks). */
    static UMaterialInstance* GetDefaultMoonMaterial();

    /** Loads default Star material. */
    static UMaterialInstance* GetDefaultStarMaterial();

    /** Loads default Gas Giant material. */
    static UMaterialInstance* GetDefaultGasGiantMaterial();

    /** Loads default Ring material. */
    static UMaterialInstance* GetDefaultRingMaterial();

    /**
     * Ensures all built-in biome and foliage collection DataAssets exist in
     * /CosmicArchitect/CosmicArchitect/Resources/Foliage/ and returns the collections for a planet archetype.
     */
    static void EnsureDefaultFoliageAssets();

    /** Returns the list of default FoliageCollections for the specified planetary archetype. */
    static TArray<UCosmicFoliageCollection*> GetDefaultFoliageCollectionsForArchetype(ECosmicPlanetArchetype Archetype);

    /** Randomly selects a FoliageCollection from the candidate list (or archetype defaults if empty). */
    static UCosmicFoliageCollection* SelectFoliageCollection(
        const TArray<TObjectPtr<UCosmicFoliageCollection>>& Candidates,
        ECosmicPlanetArchetype FallbackArchetype,
        FRandomStream& Stream
    );

    /**
     * Initializes planet visual appearance, terrain shader parameters, and procedural foliage collection.
     */
    static void InitializePlanetAppearance(
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
        UCosmicFoliageCollection* FoliageCollection = nullptr
    );

    /** Spawns and configures the central Star planet and its gravity component. */
    static ACosmicPlanet* SpawnStar(
        UWorld* World,
        AActor* OwnerActor,
        const FVector& Location,
        float StarRadiusKm,
        UMaterialInstance* StarMaterial,
        FRandomStream& Stream
    );

    /** Spawns and configures the Star's point light with sufficient radius to illuminate the system. */
    static APointLight* SpawnStarLight(
        UWorld* World,
        ACosmicPlanet* Star,
        float SystemRadiusKm,
        float MaxBodyRadiusKm,
        float MoonOrbitDistanceFactorMax,
        float MoonDiameterMaxKm
    );

    /** Spawns a planetary body with noise, clipmap, optional ocean, foliage collection, gravity, orbit, and rings. */
    static ACosmicPlanet* SpawnPlanet(
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
        UCosmicFoliageCollection* FoliageCollection = nullptr
    );

    /** Spawns a moon orbiting a parent planet with noise, clipmap, rock foliage collection, gravity, and orbit component. */
    static ACosmicPlanet* SpawnMoon(
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
        UCosmicFoliageCollection* FoliageCollection = nullptr
    );

    /** Spawns an asteroid belt ring around the central star. */
    static UCosmicRingComponent* SpawnAsteroidBelt(
        ACosmicPlanet* Star,
        float OrbitKm,
        UMaterialInstance* RingMaterial,
        FRandomStream& Stream
    );

    /** Destroys all generated celestial actors and empties the list. */
    static void ClearBodies(TArray<AActor*>& Bodies);
};
