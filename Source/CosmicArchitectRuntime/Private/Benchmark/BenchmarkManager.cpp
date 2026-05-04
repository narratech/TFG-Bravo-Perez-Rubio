// Fill out your copyright notice in the Description page of Project Settings.


#include "Benchmark/BenchmarkManager.h"
#include "Engine/World.h"
#include "Planet/CosmicPlanet.h"
#include "Kismet/GameplayStatics.h"

UBenchmarkManager* UBenchmarkManager::Get(UWorld* World)
{
    if (!World) return nullptr;
    return World->GetSubsystem<UBenchmarkManager>();
}

void UBenchmarkManager::StartBenchmark()
{
    UE_LOG(LogTemp, Warning, TEXT("Benchmark started"));
}

void UBenchmarkManager::StopBenchmark()
{
    UE_LOG(LogTemp, Warning, TEXT("Benchmark finished"));
}

void UBenchmarkManager::SetClipmapConfig(int32 BaseRes, int32 Levels)
{
    CurrentClipmapConfig.BaseResolution = BaseRes;
    CurrentClipmapConfig.NumLevels = Levels;

    UE_LOG(LogTemp, Warning, TEXT("Clipmap updated: Res=%d Levels=%d"),
        BaseRes, Levels);
}

void UBenchmarkManager::SpawnPlanets(int32 NumPlanets)
{
    UWorld* World = GetWorld();
    if (!World) return;

    UE_LOG(LogTemp, Warning, TEXT("Spawning %d planets"), NumPlanets);

    for (int32 i = 0; i < NumPlanets; i++)
    {
        FVector Location = FVector(i * 500000.0f, 0.f, 0.f); // separarlos
        FRotator Rotation = FRotator::ZeroRotator;

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        ACosmicPlanet* Planet = World->SpawnActor<ACosmicPlanet>(
            ACosmicPlanet::StaticClass(),
            Location,
            Rotation,
            Params
        );

        if (!Planet) continue;

        // LLAMADA A INIT PLANET
        Planet->InitPlanet(
            10.0f,                 // RadiusKm
            nullptr,                 // Noise
            FColor::Red,
            FColor::Orange,
            FColor::White,
            FColor::Red,
            FColor::Black,
            100.f,                  // ScaleL
            3.f,                    // ScaleM
            1.f,                    // ScaleS
            nullptr,                // Material
            nullptr,                // Texture

            // Clipmap
            true,
            CurrentClipmapConfig.BaseResolution,
            CurrentClipmapConfig.NumLevels,
            100,
            5.0f,

            // Ocean
            true,
            0.0,
            128,
            nullptr,

            // Foliage
            nullptr
        );
    }
}

void UBenchmarkManager::ClearPlanets()
{
    UWorld* World = GetWorld();
    if (!World) return;

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, ACosmicPlanet::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        Actor->Destroy();
    }

    UE_LOG(LogTemp, Warning, TEXT("Cleared %d planets"), FoundActors.Num());
}

void UBenchmarkManager::RunPlanetScalingTest()
{
    UE_LOG(LogTemp, Warning, TEXT("RunPlanetScalingTest"));

    // TODO: implementar secuencia 1,2,4,8...
}

void UBenchmarkManager::RunFoliageTest()
{
    UE_LOG(LogTemp, Warning, TEXT("RunFoliageTest"));
}

void UBenchmarkManager::RunSimulationTest()
{
    UE_LOG(LogTemp, Warning, TEXT("RunSimulationTest"));
}

void UBenchmarkManager::BeginCapture()
{
    UE_LOG(LogTemp, Warning, TEXT("Capture Start"));
}

void UBenchmarkManager::EndCapture()
{
    UE_LOG(LogTemp, Warning, TEXT("Capture End"));
}