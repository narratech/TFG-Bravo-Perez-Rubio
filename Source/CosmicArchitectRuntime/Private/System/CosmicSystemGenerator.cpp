#include "System/CosmicSystemGenerator.h"
#include "Kismet/KismetMathLibrary.h"

ACosmicSystemGenerator::ACosmicSystemGenerator()
{
    PrimaryActorTick.bCanEverTick = false;

    GenerationVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("GenerationVolume"));
    RootComponent = GenerationVolume;

    VolumeSizeKm = FVector(20.0f, 20.0f, 5.0f);

    GenerationVolume->SetBoxExtent(VolumeSizeKm * 100000.0f);
    GenerationVolume->SetLineThickness(2000.0f);

    NumberOfBodies = 10;
    Seed = 12345;
}

void ACosmicSystemGenerator::GenerateBodies()
{
    ClearBodies();

    if (!ClassToGenerate)
    {
        UE_LOG(LogTemp, Warning, TEXT("No class selected to generate!"));
        return;
    }

    FRandomStream Stream(Seed);

    for (int32 i = 0; i < NumberOfBodies; i++)
    {
        FVector Origin = GenerationVolume->GetComponentLocation();
        FVector Extents = GenerationVolume->GetScaledBoxExtent();

        float RandX = Stream.FRandRange(-Extents.X, Extents.X);
        float RandY = Stream.FRandRange(-Extents.Y, Extents.Y);
        float RandZ = Stream.FRandRange(-Extents.Z, Extents.Z);

        FVector LocalPoint = FVector(RandX, RandY, RandZ);
        FVector WorldPoint = GetActorTransform().TransformPositionNoScale(LocalPoint);

        FRotator RandomRot = FRotator(Stream.FRandRange(0, 360), Stream.FRandRange(0, 360), 0);

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        AActor* NewBody = GetWorld()->SpawnActor<AActor>(ClassToGenerate, WorldPoint, RandomRot, SpawnParams);

        if (NewBody)
        {
            GeneratedBodies.Add(NewBody);
        }
    }
}

void ACosmicSystemGenerator::GenerateWithRandomSeed()
{
    int32 RandomSeed = 0;

    //Generar semilla aleatoria usando varias fuentes
    RandomSeed += static_cast<int32>(FDateTime::Now().GetTicks());
    RandomSeed += static_cast<int32>(FPlatformTime::Cycles());
    RandomSeed += reinterpret_cast<int64>(this);
    RandomSeed += FPlatformTLS::GetCurrentThreadId();

    Seed = HashCombine(GetTypeHash(RandomSeed), GetTypeHash(FMath::Rand()));

    GenerateBodies();
}

void ACosmicSystemGenerator::ClearBodies()
{
    for (AActor* Actor : GeneratedBodies)
    {
        if (Actor && IsValid(Actor))
        {
            Actor->Destroy();
        }
    }
    GeneratedBodies.Empty();
}

void ACosmicSystemGenerator::UpdateVolumeSize()
{
    if (GenerationVolume)
    {
        GenerationVolume->SetBoxExtent(VolumeSizeKm * 100000.0f);
    }
}
