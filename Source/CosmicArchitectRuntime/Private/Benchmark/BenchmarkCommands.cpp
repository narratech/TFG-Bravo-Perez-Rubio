// Fill out your copyright notice in the Description page of Project Settings.


#include "HAL/IConsoleManager.h"
#include "Benchmark/BenchmarkManager.h"
#include "Engine/World.h"

static FAutoConsoleCommandWithWorldAndArgs CmdSpawnPlanets(
    TEXT("bm.spawn_planets"),
    TEXT("Spawn planets: bm.spawn_planets 10"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World || Args.Num() == 0) return;

            int32 Num = FCString::Atoi(*Args[0]);

            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->SpawnPlanets(Num);
            }
        }
    )
);

static FAutoConsoleCommandWithWorldAndArgs CmdRunPlanetTest(
    TEXT("bm.run_planet_test"),
    TEXT("Run planet scaling benchmark"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;

            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->RunPlanetScalingTest();
            }
        }
    )
);

static FAutoConsoleCommandWithWorldAndArgs CmdClear(
    TEXT("bm.clear"),
    TEXT("Clear planets"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;

            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->ClearPlanets();
            }
        }
    )
);
