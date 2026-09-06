// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMeshActor.h" 
#include "CosmicSystemGenerator.generated.h"

class UCosmicNoiseClass;
class UCosmicDefaultNoiseSettings;
class UMaterialInstance;

/**
 * Procedural generator of planetary systems.
 * Responsible for creating celestial bodies, orbits,
 * materials, and orbital configurations.
 */
UCLASS(HideCategories = (
    Replication, Input, Collision, Actor, LOD, Cooking, Networking,
    Physics, Navigation, Tags, DataLayers, LevelInstance
    ), AutoExpandCategories = ("Configuration", "Generation Rules", "Actions"))
    class COSMICARCHITECTRUNTIME_API ACosmicSystemGenerator : public AActor 
{
    GENERATED_BODY()

public:
    ACosmicSystemGenerator();

    /* Base textures for procedural visual variation. */
    UPROPERTY(EditAnywhere, Category = "Materials")
    TArray<UTexture2D*> PosiblesTexturas;

    /* Base material for generic terrestrial planets. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* BaseMaterial;

    /* Material for moons. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* MoonMaterial;

    /* Ocean material for planets with water bodies. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* OceanMaterial;

    /* Material for stars. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* StarMaterial;

    /* Material for gas giants. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* GasGiantMaterial;

    /* Material for planetary rings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* RingMaterial;

    /* Debug line thickness. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    float LineWidth = 100;

    /* Debug box color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FColor BoxColor = FColor::Blue;

#if WITH_EDITORONLY_DATA
    /* Saves to disk Noise Settings generated from editor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistence")
    bool bSaveGeneratedNoiseSettingsAssets = true;

    /* Content Browser base folder where each generator creates its own subfolder. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Persistence", meta = (EditCondition = "bSaveGeneratedNoiseSettingsAssets"))
    FString GeneratedNoiseSettingsAssetFolder = TEXT("/Game/CosmicArchitect/GeneratedNoiseSettings");

    /* Persistent identifier to separate this generator's assets from others. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Persistence", meta = (EditCondition = "bSaveGeneratedNoiseSettingsAssets"))
    FString GeneratedNoiseSettingsFolderId;
#endif
protected:
    UPROPERTY(VisibleDefaultsOnly, Category = "Root", BlueprintReadOnly)
    USceneComponent* Root;

    // CONFIGURATION

    /* Deterministic seed for procedural generation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    int32 Seed = 1337;

    /* Global orbital speed multiplier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration",
        meta = (ClampMin = "0.0", ClampMax = "100000.0"))
    float OrbitSpeedMultiplier = 1.0f;

    /* Is orbital simulation active in editor? */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Configuration")
    bool bIsSimulatingOrbits = false;

    /** Total number of bodies to generate (planets + moons, not counting the star). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "1", ClampMax = "200"))
    int32 NumberOfBodies = 5;

    /* Size of procedural volume in kilometers. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.1"))
    FVector VolumeSizeKm = FVector(3000.0f, 3000.0f, 5.0f);

    /* Diameter range for planets (km). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.001"))
    FVector2D BodyDiameterRangeKm = FVector2D(8.f, 15.f);

    /* Diameter range for moons (km). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.001"))
    FVector2D MoonDiameterRangeKm = FVector2D(2.f, 7.f);

    /* Surface gravity assigned to min and max planet radius. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Gravity", meta = (ClampMin = "0.0"))
    FVector2D PlanetSurfaceGravityRange = FVector2D(3.f, 10.f);

    /* Surface gravity assigned to min and max moon radius. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Gravity", meta = (ClampMin = "0.0"))
    FVector2D MoonSurfaceGravityRange = FVector2D(1.f, 5.f);
    /** Fraction of system radius occupied by the star. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Star", meta = (ClampMin = "0.01", ClampMax = "0.9"))
    float StarRadiusFraction = 0.1f;

    // GENERATION RULES – DISTANCES AND RADII

    /* Minimum distance between body centers (km). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Distances", meta = (ClampMin = "0.0"))
    float MinDistanceBetweenBodies = 5.0f;

    /* Maximum distance to nearest neighbor (0 = no clustering). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Distances", meta = (ClampMin = "0.0"))
    float MaxDistanceToNearest = 0.0f;

    /* Maximum attempts to find a valid position. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Distances", AdvancedDisplay, meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxGenerationAttempts = 100;

    /** Factor over star radius for minimum orbital distance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Orbits", meta = (ClampMin = "1.0"))
    float OrbitDistanceMinFactor = 3.0f;

    /** Factors to obtain planetary radius from orbital distance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Planets", meta = (ClampMin = "0.001", ClampMax = "0.5"))
    float PlanetRadiusFactorMin = 0.01f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Planets", meta = (ClampMin = "0.001", ClampMax = "0.5"))
    float PlanetRadiusFactorMax = 0.06f;

    /** Factors for lunar orbit (multiplies planet radius). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "1.0"))
    float MoonOrbitDistanceFactorMin = 10.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "1.0"))
    float MoonOrbitDistanceFactorMax = 15.0f;

    /** Factors for lunar radius (multiplies planet radius). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float MoonRadiusFactorMin = 0.1f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float MoonRadiusFactorMax = 0.3f;

    // GENERATION RULES – CLASSIFICATION AND PROBABILITIES

    /** Size/distance ratio threshold to classify as gas giant. */
    /** Fraction of total bodies remaining to generate for gas giants to appear.
 *  0.3 = only when 30% or fewer bodies remain to be generated.
 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GasGiantAppearanceThreshold = 0.3f;

    /** Probability of a planet being a gas giant when spawn condition is met. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GasGiantProbability = 0.7f;

    /** Radius factors for gas giants (multiply orbital distance). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.001", ClampMax = "0.5"))
    float GasGiantRadiusFactorMin = 0.001f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.001", ClampMax = "0.5"))
    float GasGiantRadiusFactorMax = 0.5f;

    /** Radius range (km) for gas giants. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.1"))
    float GasGiantRadiusMin = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.1"))
    float GasGiantRadiusMax = 50.0f;

    /** Habitable zone (fraction of system radius). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HabitableZoneInnerFraction = 0.25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HabitableZoneOuterFraction = 0.65f;

    /** Asteroid belt zone. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BeltZoneInnerFraction = 0.55f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BeltZoneOuterFraction = 0.70f;

    /** Probability of a planet in belt zone being a belt. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BeltProbability = 0.6f;

    /** Ring probability for gas giants. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GasGiantRingProbability = 0.65f;

    /** Ocean probability on terrestrial planets located in habitable zone. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TelluricOceanProbability = 0.7f;

    // GENERATION RULES – MOON RANGES

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0", ClampMax = "20"))
    int32 GasGiantMoonMin = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0", ClampMax = "20"))
    int32 GasGiantMoonMax = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0", ClampMax = "20"))
    int32 TelluricMoonMin = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0", ClampMax = "20"))
    int32 TelluricMoonMax = 3;

    // GENERATION RULES – RESOLUTIONS

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Graphics", meta = (ClampMin = "16", ClampMax = "128"))
    int32 GasGiantClipResolution = 64;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Graphics", meta = (ClampMin = "16", ClampMax = "164"))
    int32 TelluricClipResolution = 128;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Graphics", meta = (ClampMin = "16", ClampMax = "128"))
    int32 OceanResolutionWithOcean = 128;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Graphics", meta = (ClampMin = "16", ClampMax = "128"))
    int32 OceanResolutionWithoutOcean = 64;

    /** Generated bodies (internal reference only). */
    UPROPERTY()
    TArray<AActor*> GeneratedBodies;

#if WITH_EDITOR
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    virtual void Tick(float DeltaTime) override;
    virtual void PostDuplicate(EDuplicateMode::Type Mode) override;
#endif

private:
    enum class EPlanetType
    {
        GasGiant,
        Telluric,
        AsteroidBelt
    };

    struct FPlanetClassification
    {
        EPlanetType Type;
        bool bHasOcean;
        float OceanSeaLevel;
        bool bHasRings;
        bool bHasMoons;
        int32 MaxMoons;
    };

    FPlanetClassification ClassifyPlanet(
        float OrbitDistanceKm,
        float PlanetRadiusKm,
        float SystemRadiusKm,
        FRandomStream& Stream,
        int32 RemainingBodies,
        int32 TotalBodies) const;

    UCosmicNoiseClass* CreateRandomNoiseSettings(FRandomStream& Stream, float PlanetRadius);

    FColor GetRandomColor(FRandomStream& Stream, int min, int max);

    float CalculateSurfaceGravity(float RadiusKm, const FVector2D& RadiusRangeKm, const FVector2D& GravityRange) const;
#if WITH_EDITOR
    UCosmicDefaultNoiseSettings* CreateOrReusePersistentRandomNoiseSettingsAsset();

    bool ShouldCreatePersistentNoiseSettingsAssets() const;

    void EnsureGeneratedNoiseSettingsFolderId();

    FString GetGeneratedNoiseSettingsFolder() const;

    FString MakeNoiseAssetName(int32 AssetIndex) const;

    void LoadGeneratedNoiseSettingsAssets();

    void SaveGeneratedNoiseSettingsAsset(UCosmicDefaultNoiseSettings* NoiseSettings) const;

    static void SanitizeObjectName(FString& Name);
#endif

    void UpdateBodiesOrbitalPeriod();

    int32 GeneratedNoiseAssetCounter = 0;

#if WITH_EDITORONLY_DATA
    UPROPERTY(Transient)
    TArray<TObjectPtr<UCosmicDefaultNoiseSettings>> GeneratedNoiseSettingsAssets;
#endif
    /** Tries to place a planet respecting distances. Returns true if placed. */
    bool TryPlacePlanet(
        FRandomStream& Stream,
        float SystemRadiusKm,
        float StarRadiusKm,
        const TArray<float>& ExistingOrbitDistances,
        const TArray<float>& ExistingPlanetRadii,
        float& OutOrbitDistance,
        float& OutPlanetRadius,
        bool bIsGasGiant) const;

    /** Checks whether proposed orbital distance is valid against existing ones. */
    bool IsOrbitDistanceValid(
        float ProposedOrbitKm,
        float ProposedRadiusKm,
        const TArray<float>& ExistingOrbits,
        const TArray<float>& ExistingRadii) const;

public:
    void SetNumBodies(int32 NumBodies);

    UFUNCTION(CallInEditor, Category = "Actions")
    void GenerateBodies();

    UFUNCTION(CallInEditor, Category = "Actions")
    void GenerateWithRandomSeed();

    UFUNCTION(CallInEditor, Category = "Actions")
    void ClearBodies();

    UFUNCTION(CallInEditor, Category = "Actions")
    void StartOrbitSimulation();

    UFUNCTION(CallInEditor, Category = "Actions")
    void StopOrbitSimulation();
};