// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "System/CosmicSystemNoiseManager.h"
#include "CosmicDefaultNoiseSettings.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "Math/RandomStream.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#endif

void FCosmicSystemNoiseManager::Reset()
{
    GeneratedNoiseAssetCounter = 0;
#if WITH_EDITOR
    GeneratedNoiseSettingsAssets.Empty();
#endif
}

#if WITH_EDITOR
void FCosmicSystemNoiseManager::OnPostDuplicate(FString& InOutFolderId)
{
    InOutFolderId.Empty();
    GeneratedNoiseSettingsAssets.Empty();
}

void FCosmicSystemNoiseManager::SanitizeObjectName(FString& Name)
{
    for (TCHAR& Character : Name)
    {
        if (!FChar::IsAlnum(Character) && Character != TEXT('_'))
        {
            Character = TEXT('_');
        }
    }
}

void FCosmicSystemNoiseManager::EnsureGeneratedNoiseSettingsFolderId(
    FString& InOutFolderId,
    const FString& ActorName,
    UPackage* PackageToDirty)
{
    if (!InOutFolderId.IsEmpty())
    {
        return;
    }

    FString ActorPart = ActorName;
    const FString GuidPart = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    SanitizeObjectName(ActorPart);
    InOutFolderId = FString::Printf(TEXT("%s_%s"), *ActorPart, *GuidPart);
    SanitizeObjectName(InOutFolderId);

    if (PackageToDirty)
    {
        PackageToDirty->MarkPackageDirty();
    }
}

FString FCosmicSystemNoiseManager::GetGeneratedNoiseSettingsFolder(
    const FString& FolderId,
    const FString& BaseAssetFolder)
{
    FString Folder = BaseAssetFolder;
    Folder.TrimStartAndEndInline();

    if (Folder.IsEmpty())
    {
        Folder = TEXT("/Game/CosmicArchitect/GeneratedNoiseSettings");
    }
    else if (!Folder.StartsWith(TEXT("/")))
    {
        Folder = FString::Printf(TEXT("/Game/%s"), *Folder);
    }

    while (Folder.EndsWith(TEXT("/")))
    {
        Folder.LeftChopInline(1);
    }

    if (!FPackageName::IsValidLongPackageName(Folder, true))
    {
        UE_LOG(LogTemp, Warning, TEXT("CosmicSystemNoiseManager: invalid noise asset folder '%s'. Using transient noise settings."), *Folder);
        return FString();
    }

    if (FolderId.IsEmpty())
    {
        return FString();
    }

    return FString::Printf(TEXT("%s/%s"), *Folder, *FolderId);
}

FString FCosmicSystemNoiseManager::MakeNoiseAssetName(int32 AssetIndex)
{
    return FString::Printf(TEXT("Noise_%03d"), AssetIndex);
}

void FCosmicSystemNoiseManager::LoadGeneratedNoiseSettingsAssets(
    const FString& FolderId,
    const FString& BaseAssetFolder)
{
    GeneratedNoiseSettingsAssets.Empty();

    const FString Folder = GetGeneratedNoiseSettingsFolder(FolderId, BaseAssetFolder);
    if (Folder.IsEmpty())
    {
        return;
    }

    FARFilter Filter;
    Filter.PackagePaths.Add(*Folder);
    Filter.ClassPaths.Add(UCosmicDefaultNoiseSettings::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = false;

    TArray<FAssetData> AssetDataList;
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
        const FString AssetName = AssetData.AssetName.ToString();
        if (!AssetName.StartsWith(TEXT("Noise_")))
        {
            continue;
        }

        int32 AssetIndex = INDEX_NONE;
        if (!LexTryParseString(AssetIndex, *AssetName.RightChop(6)) || AssetIndex < 0)
        {
            continue;
        }

        UCosmicDefaultNoiseSettings* NoiseSettings = Cast<UCosmicDefaultNoiseSettings>(AssetData.GetAsset());
        if (!NoiseSettings)
        {
            continue;
        }

        if (GeneratedNoiseSettingsAssets.Num() <= AssetIndex)
        {
            GeneratedNoiseSettingsAssets.SetNum(AssetIndex + 1);
        }

        GeneratedNoiseSettingsAssets[AssetIndex] = NoiseSettings;
    }
}

void FCosmicSystemNoiseManager::SaveGeneratedNoiseSettingsAsset(UCosmicDefaultNoiseSettings* NoiseSettings)
{
    if (!NoiseSettings || !NoiseSettings->IsAsset())
    {
        return;
    }

    NoiseSettings->MarkPackageDirty();
    TArray<UPackage*> PackagesToSave;
    PackagesToSave.Add(NoiseSettings->GetOutermost());
    UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false);
}

UCosmicDefaultNoiseSettings* FCosmicSystemNoiseManager::CreateOrReusePersistentRandomNoiseSettingsAsset(
    const FString& FolderId,
    const FString& BaseAssetFolder)
{
    const int32 AssetIndex = GeneratedNoiseAssetCounter++;

    if (GeneratedNoiseSettingsAssets.IsValidIndex(AssetIndex) && GeneratedNoiseSettingsAssets[AssetIndex])
    {
        return GeneratedNoiseSettingsAssets[AssetIndex].Get();
    }

    const FString Folder = GetGeneratedNoiseSettingsFolder(FolderId, BaseAssetFolder);
    if (Folder.IsEmpty())
    {
        return nullptr;
    }

    const FString AssetName = MakeNoiseAssetName(AssetIndex);
    const FString PackageName = FString::Printf(TEXT("%s/%s"), *Folder, *AssetName);
    const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackageName, *AssetName);

    if (UCosmicDefaultNoiseSettings* ExistingAsset = Cast<UCosmicDefaultNoiseSettings>(StaticLoadObject(UCosmicDefaultNoiseSettings::StaticClass(), nullptr, *ObjectPath)))
    {
        if (GeneratedNoiseSettingsAssets.Num() <= AssetIndex)
        {
            GeneratedNoiseSettingsAssets.SetNum(AssetIndex + 1);
        }
        GeneratedNoiseSettingsAssets[AssetIndex] = ExistingAsset;
        return ExistingAsset;
    }

    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        return nullptr;
    }

    Package->FullyLoad();

    if (UObject* ExistingObject = StaticFindObjectFast(UObject::StaticClass(), Package, *AssetName))
    {
        if (UCosmicDefaultNoiseSettings* ExistingNoiseSettings = Cast<UCosmicDefaultNoiseSettings>(ExistingObject))
        {
            if (GeneratedNoiseSettingsAssets.Num() <= AssetIndex)
            {
                GeneratedNoiseSettingsAssets.SetNum(AssetIndex + 1);
            }
            GeneratedNoiseSettingsAssets[AssetIndex] = ExistingNoiseSettings;
            return ExistingNoiseSettings;
        }

        UE_LOG(LogTemp, Warning, TEXT("CosmicSystemNoiseManager: object '%s' already exists but is not a UCosmicDefaultNoiseSettings asset. Using transient noise settings."), *ObjectPath);
        return nullptr;
    }

    UCosmicDefaultNoiseSettings* NewAsset = NewObject<UCosmicDefaultNoiseSettings>(
        Package,
        UCosmicDefaultNoiseSettings::StaticClass(),
        *AssetName,
        RF_Public | RF_Standalone | RF_Transactional);

    if (NewAsset)
    {
        FAssetRegistryModule::AssetCreated(NewAsset);
        if (GeneratedNoiseSettingsAssets.Num() <= AssetIndex)
        {
            GeneratedNoiseSettingsAssets.SetNum(AssetIndex + 1);
        }
        GeneratedNoiseSettingsAssets[AssetIndex] = NewAsset;
    }

    return NewAsset;
}
#endif

UCosmicNoiseClass* FCosmicSystemNoiseManager::CreateRandomNoiseSettings(
    FRandomStream& Stream,
    float PlanetRadius,
    UWorld* World,
    const FString& FolderId,
    const FString& BaseAssetFolder,
    bool bSavePersistentAssets)
{
    UCosmicDefaultNoiseSettings* NewSettings = nullptr;

#if WITH_EDITOR
    const bool bShouldPersist = bSavePersistentAssets
        && World
        && World->WorldType == EWorldType::Editor;

    if (bShouldPersist)
    {
        NewSettings = CreateOrReusePersistentRandomNoiseSettingsAsset(FolderId, BaseAssetFolder);
    }
#endif

    if (!NewSettings)
    {
        NewSettings = NewObject<UCosmicDefaultNoiseSettings>();
    }

    NewSettings->Seed = Stream.RandRange(0, 999999);

    const float FeatureScaleKm = FMath::Clamp(PlanetRadius, 5.0f, 500.0f);
    FCosmicNoiseLayer& Layer = NewSettings->LayerParameters;

    Layer.NoiseType = static_cast<ECosmicNoiseType>(Stream.RandRange(0, 3));
    Layer.FractalType = static_cast<ECosmicFractalType>(Stream.RandRange(1, 3));
    Layer.Frequency = Stream.FRandRange(2.0f, 5.0f);
    Layer.Octaves = Stream.RandRange(6, 8);
    Layer.Lacunarity = Stream.FRandRange(1.8f, 2.5f);
    Layer.Persistence = Stream.FRandRange(0.4f, 0.7f);
    Layer.Amplitude = PlanetRadius * 30000.0f * Stream.FRandRange(0.03f, 0.06f);

    FCosmicNoiseBiomeParameters& Biome = NewSettings->BiomeParameters;
    Biome.HumidityFrequency = Stream.FRandRange(0.3f, 1.5f) / FeatureScaleKm;
    Biome.HumidityOctaves = Stream.RandRange(2, 5);
    Biome.HumidityOffset = Stream.FRandRange(-0.2f, 0.2f);
    Biome.HumidityContrast = Stream.FRandRange(0.8f, 1.5f);
    Biome.TemperatureFrequency = Stream.FRandRange(0.3f, 1.5f) / FeatureScaleKm;
    Biome.LatitudeEffect = Stream.FRandRange(0.5f, 1.5f);
    Biome.AltitudeTemperaturePenalty = Stream.FRandRange(0.2f, 0.6f);

#if WITH_EDITOR
    SaveGeneratedNoiseSettingsAsset(NewSettings);
#endif

    return NewSettings;
}
