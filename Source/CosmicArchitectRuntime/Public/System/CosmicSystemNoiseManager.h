// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"
#include "System/CosmicSystemTypes.h"

class UCosmicDefaultNoiseSettings;
class UCosmicMultiNoiseSettings;
class UCosmicErosionMultifractalNoiseSettings;
class UCosmicCraterNoiseSettings;

/**
 * Manages procedural noise generation and optional editor asset persistence for planetary systems.
 * Supports MultiNoise, ErosionMultifractal, Crater, and Default noise strategies tailored per planet archetype.
 */
class COSMICARCHITECTRUNTIME_API FCosmicSystemNoiseManager
{
public:
    FCosmicSystemNoiseManager() = default;

    /** Resets the asset counter and in-memory asset cache. */
    void Reset();

    /** Generates random procedural noise settings for a planet or moon (backward compatible). */
    UCosmicNoiseClass* CreateRandomNoiseSettings(
        FRandomStream& Stream,
        float PlanetRadius,
        UWorld* World,
        const FString& FolderId,
        const FString& BaseAssetFolder,
        bool bSavePersistentAssets
    );

    /** Generates high-quality procedural noise settings (MultiNoise, ErosionMultifractal, or Crater) for a specific planetary archetype. */
    UCosmicNoiseClass* CreateNoiseSettingsForArchetype(
        FRandomStream& Stream,
        float PlanetRadius,
        ECosmicPlanetArchetype Archetype,
        bool bHasOcean,
        UWorld* World,
        const FString& FolderId,
        const FString& BaseAssetFolder,
        bool bSavePersistentAssets
    );

#if WITH_EDITOR
    /** Ensures a unique persistent folder ID for this generator instance. */
    static void EnsureGeneratedNoiseSettingsFolderId(
        FString& InOutFolderId,
        const FString& ActorName,
        UPackage* PackageToDirty
    );

    /** Resolves the complete long package path for saving generated noise assets. */
    static FString GetGeneratedNoiseSettingsFolder(
        const FString& FolderId,
        const FString& BaseAssetFolder
    );

    /** Generates a deterministic asset name from index (e.g. Noise_000). */
    static FString MakeNoiseAssetName(int32 AssetIndex);

    /** Loads previously generated noise assets from disk for reuse. */
    void LoadGeneratedNoiseSettingsAssets(
        const FString& FolderId,
        const FString& BaseAssetFolder
    );

    /** Saves a dirty noise settings asset to disk. */
    static void SaveGeneratedNoiseSettingsAsset(UCosmicNoiseClass* NoiseSettings);

    /** Finds or creates a persistent UCosmicNoiseClass asset of the requested subclass in the content browser. */
    UCosmicNoiseClass* CreateOrReusePersistentNoiseSettingsAsset(
        const FString& FolderId,
        const FString& BaseAssetFolder,
        TSubclassOf<UCosmicNoiseClass> SettingsClass
    );

    /** Sanitizes string characters to be valid Unreal asset/package name tokens. */
    static void SanitizeObjectName(FString& Name);

    /** Clears transient noise assets cache on actor duplication. */
    void OnPostDuplicate(FString& InOutFolderId);
#endif

private:
    int32 GeneratedNoiseAssetCounter = 0;

#if WITH_EDITOR
    TArray<TObjectPtr<UCosmicNoiseClass>> GeneratedNoiseSettingsAssets;
#endif
};
