// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CosmicNoiseClass.h"

class UCosmicDefaultNoiseSettings;

/**
 * Manages procedural noise generation and optional editor asset persistence for planetary systems.
 */
class COSMICARCHITECTRUNTIME_API FCosmicSystemNoiseManager
{
public:
    FCosmicSystemNoiseManager() = default;

    /** Resets the asset counter and in-memory asset cache. */
    void Reset();

    /** Generates random procedural noise settings for a planet or moon. */
    UCosmicNoiseClass* CreateRandomNoiseSettings(
        FRandomStream& Stream,
        float PlanetRadius,
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
    static void SaveGeneratedNoiseSettingsAsset(UCosmicDefaultNoiseSettings* NoiseSettings);

    /** Finds or creates a persistent UCosmicDefaultNoiseSettings asset in the content browser. */
    UCosmicDefaultNoiseSettings* CreateOrReusePersistentRandomNoiseSettingsAsset(
        const FString& FolderId,
        const FString& BaseAssetFolder
    );

    /** Sanitizes string characters to be valid Unreal asset/package name tokens. */
    static void SanitizeObjectName(FString& Name);

    /** Clears transient noise assets cache on actor duplication. */
    void OnPostDuplicate(FString& InOutFolderId);
#endif

private:
    int32 GeneratedNoiseAssetCounter = 0;

#if WITH_EDITOR
    TArray<TObjectPtr<UCosmicDefaultNoiseSettings>> GeneratedNoiseSettingsAssets;
#endif
};
