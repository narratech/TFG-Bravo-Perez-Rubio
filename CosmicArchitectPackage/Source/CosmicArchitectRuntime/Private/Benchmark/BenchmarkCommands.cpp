// Fill out your copyright notice in the Description page of Project Settings.

#include "HAL/IConsoleManager.h"
#include "Benchmark/BenchmarkManager.h"
#include "Benchmark/BenchmarkRecorder.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

// COMANDOS BÁSICOS

static FAutoConsoleCommandWithWorldAndArgs CmdRunAllTests(
    TEXT("bm.run_all"),
    TEXT("Run ALL benchmark tests in sequence (9 tests total)"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;

            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->RunAllTests();
            }
        }
    )
);

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

static FAutoConsoleCommandWithWorldAndArgs CmdSetClipmap(
    TEXT("bm.set_clipmap"),
    TEXT("Set clipmap config: bm.set_clipmap baseRes=128 levels=4"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;
            int32 BaseRes = 128;
            int32 Levels = 4;
            for (const FString& Arg : Args)
            {
                FString Key, Value;
                if (Arg.Split(TEXT("="), &Key, &Value))
                {
                    if (Key == "baseRes") BaseRes = FCString::Atoi(*Value);
                    if (Key == "levels") Levels = FCString::Atoi(*Value);
                }
            }
            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->SetClipmapConfig(BaseRes, Levels);
            }
        }
    )
);

static FAutoConsoleCommandWithWorldAndArgs CmdClear(
    TEXT("bm.clear"),
    TEXT("Clear all planets"),
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

// COMANDOS DE PRUEBAS 

static FAutoConsoleCommandWithWorldAndArgs CmdRunPlanetTest(
    TEXT("bm.run_planet_test"),
    TEXT("Run planet scaling benchmark: 1,2,4,8,16,32 planets"),
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

static FAutoConsoleCommandWithWorldAndArgs CmdRunClosePlanetTest(
    TEXT("bm.run_close_test"),
    TEXT("Run close planets benchmark: planets near camera at max detail"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;
            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->RunClosePlanetTest();
            }
        }
    )
);

// COMANDOS DE FOLIAGE 

static FAutoConsoleCommandWithWorldAndArgs CmdFoliagePerFrameTest(
    TEXT("bm.foliage_per_frame"),
    TEXT("Test foliage instances per frame. Usage:\n")
    TEXT("  bm.foliage_per_frame      - Run sequential test (10, 50, 100, 200, 500, 1000)\n")
    TEXT("  bm.foliage_per_frame 200  - Test with 200 instances/frame"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;
            int32 MaxPerFrame = (Args.Num() > 0) ? FCString::Atoi(*Args[0]) : 0;
            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->RunFoliagePerFrameTest(MaxPerFrame);
            }
        }
    )
);

static FAutoConsoleCommandWithWorldAndArgs CmdFoliageRadiusTest(
    TEXT("bm.foliage_radius"),
    TEXT("Run foliage view distance test with different layer radii:\n")
    TEXT("  Tests: Near(0.01-0.5), Medium(0.05-2.0), Far(0.1-5.0) km"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;
            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->RunFoliageRadiusTest();
            }
        }
    )
);

// COMANDOS DE CLIPMAP 

static FAutoConsoleCommandWithWorldAndArgs CmdClipmapResolutionTest(
    TEXT("bm.clipmap_res"),
    TEXT("Test clipmap resolution. Usage:\n")
    TEXT("  bm.clipmap_res         - Run sequential test (8, 16, 32, 64, 128, 256)\n")
    TEXT("  bm.clipmap_res 256     - Test single resolution 256"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;

            int32 Resolution = 0;
            if (Args.Num() > 0)
            {
                Resolution = FCString::Atoi(*Args[0]);
            }

            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->RunClipmapResolutionTest(Resolution);
            }
        }
    )
);

static FAutoConsoleCommandWithWorldAndArgs CmdClipmapLevelsTest(
    TEXT("bm.clipmap_levels"),
    TEXT("Test clipmap levels. Usage:\n")
    TEXT("  bm.clipmap_levels      - Run sequential test (1, 2, 4, 6, 8)\n")
    TEXT("  bm.clipmap_levels 6    - Test single config with 6 levels"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;

            int32 Levels = (Args.Num() > 0) ? FCString::Atoi(*Args[0]) : 0;

            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->RunClipmapLevelsTest(Levels);
            }
        }
    )
);

// COMANDOS DE SIMULACIÓN 

static FAutoConsoleCommandWithWorldAndArgs CmdOrbitSimTest(
    TEXT("bm.orbit_sim"),
    TEXT("Test orbit simulation. Usage:\n")
    TEXT("  bm.orbit_sim        - Run sequential test (10, 50, 100, 200, 500)\n")
    TEXT("  bm.orbit_sim 50     - Test with 50 bodies"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;
            int32 NumBodies = (Args.Num() > 0) ? FCString::Atoi(*Args[0]) : 0;
            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->RunOrbitSimulationTest(NumBodies);
            }
        }
    )
);

static FAutoConsoleCommandWithWorldAndArgs CmdNBodySimTest(
    TEXT("bm.nbody_sim"),
    TEXT("Test N-Body simulation. Usage:\n")
    TEXT("  bm.nbody_sim        - Run sequential test (10, 20, 50, 100, 200)\n")
    TEXT("  bm.nbody_sim 50     - Test with 50 bodies"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;
            int32 NumBodies = (Args.Num() > 0) ? FCString::Atoi(*Args[0]) : 0;
            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->RunNBodySimulationTest(NumBodies);
            }
        }
    )
);

// COMANDOS DE GENERADOR 

static FAutoConsoleCommandWithWorldAndArgs CmdSystemGenTest(
    TEXT("bm.system_gen"),
    TEXT("Test System Generation. Usage:\n")
    TEXT("  bm.system_gen       - Run sequential test (1, 2, 5, 10, 20, 50)\n")
    TEXT("  bm.system_gen 10     - Test with 10 bodies"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;
            int32 NumBodies = (Args.Num() > 0) ? FCString::Atoi(*Args[0]) : 0;
            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->RunSystemGeneratorTest(NumBodies);
            }
        }
    )
);

// COMANDOS DE CAPTURA 

static FAutoConsoleCommandWithWorldAndArgs CmdBeginCapture(
    TEXT("bm.capture_start"),
    TEXT("Start benchmark capture: bm.capture_start 5 (seconds)"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;
            float Duration = 5.0f;
            if (Args.Num() > 0) Duration = FCString::Atof(*Args[0]);
            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->BeginCapture(Duration);
            }
        }
    )
);

static FAutoConsoleCommandWithWorldAndArgs CmdEndCapture(
    TEXT("bm.capture_end"),
    TEXT("Stop benchmark capture and log results"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!World) return;
            if (UBenchmarkManager* Manager = UBenchmarkManager::Get(World))
            {
                Manager->EndCapture();
            }
        }
    )
);

// COMANDO DE EXPORTACIÓN 

static FAutoConsoleCommandWithWorldAndArgs CmdExportResults(
    TEXT("bm.export"),
    TEXT("Export benchmark results to CSV file"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World)
        {
            FBenchmarkData Data = FBenchmarkRecorder::GetCurrentData();

            // Formato CSV
            FString CSVLine = FString::Printf(
                TEXT("%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f"),
                Data.NumObjects, Data.AvgFPS, Data.Low1FPS, Data.FrameTimeMs,
                Data.GameThreadTimeMs, Data.RenderThreadTimeMs, Data.GPUTimeMs,
                Data.RAM, Data.VRAM
            );

            // Guardar a archivo
            FString FilePath = FPaths::ProjectSavedDir() / TEXT("BenchmarkResults.csv");
            FFileHelper::SaveStringToFile(CSVLine + TEXT("\n"), *FilePath,
                FFileHelper::EEncodingOptions::ForceUTF8, &IFileManager::Get(),
                FILEWRITE_Append);

            UE_LOG(LogTemp, Warning, TEXT("Results exported to: %s"), *FilePath);
            UE_LOG(LogTemp, Warning, TEXT("CSV: %s"), *CSVLine);
        }
    )
);

// AYUDA 

static FAutoConsoleCommand CmdHelp(
    TEXT("bm.help"),
    TEXT("Show all benchmark commands"),
    FConsoleCommandDelegate::CreateStatic(
        []()
        {
            UE_LOG(LogTemp, Warning, TEXT("=== Benchmark Commands ==="));
            UE_LOG(LogTemp, Warning, TEXT("bm.spawn_planets <N>        - Spawn N planets"));
            UE_LOG(LogTemp, Warning, TEXT("bm.set_clipmap <res> <lvl>  - Set clipmap config"));
            UE_LOG(LogTemp, Warning, TEXT("bm.clear                    - Clear all planets"));
            UE_LOG(LogTemp, Warning, TEXT("bm.run_planet_test          - Run planet scaling test"));
            UE_LOG(LogTemp, Warning, TEXT("bm.run_close_test           - Run close planets test"));
            UE_LOG(LogTemp, Warning, TEXT("bm.foliage_density <N>      - Test foliage density"));
            UE_LOG(LogTemp, Warning, TEXT("bm.foliage_per_frame <N>    - Test foliage per frame"));
            UE_LOG(LogTemp, Warning, TEXT("bm.clipmap_res <N>          - Test clipmap resolution"));
            UE_LOG(LogTemp, Warning, TEXT("bm.clipmap_levels <N>       - Test clipmap levels"));
            UE_LOG(LogTemp, Warning, TEXT("bm.clipmap_refresh <T>      - Test clipmap refresh time"));
            UE_LOG(LogTemp, Warning, TEXT("bm.orbit_sim <N>            - Test orbit simulation"));
            UE_LOG(LogTemp, Warning, TEXT("bm.nbody_sim <N>            - Test N-body simulation"));
            UE_LOG(LogTemp, Warning, TEXT("bm.system_gen <N>           - Test system generator"));
            UE_LOG(LogTemp, Warning, TEXT("bm.capture_start <T>        - Start capture for T seconds"));
            UE_LOG(LogTemp, Warning, TEXT("bm.capture_end              - Stop capture"));
            UE_LOG(LogTemp, Warning, TEXT("bm.export                   - Export results to CSV"));
        }
    )
);