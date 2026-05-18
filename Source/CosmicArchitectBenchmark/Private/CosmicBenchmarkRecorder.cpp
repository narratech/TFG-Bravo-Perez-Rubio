// Fill out your copyright notice in the Description page of Project Settings.

#include "CosmicBenchmarkRecorder.h"
#include "Engine/Engine.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "DynamicRHI.h"

// Para VRAM en Windows
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <dxgi1_4.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

// 
//  Variables estáticas
// 
bool                        FCosmicBenchmarkRecorder::bIsRecording = false;
float                       FCosmicBenchmarkRecorder::AccumulatedFPS = 0.0f;
float                       FCosmicBenchmarkRecorder::AccumulatedLow1FPS = 0.0f;
float                       FCosmicBenchmarkRecorder::AccumulatedFrameTime = 0.0f;
float                       FCosmicBenchmarkRecorder::AccumulatedGameThreadTime = 0.0f;
float                       FCosmicBenchmarkRecorder::AccumulatedRenderThreadTime = 0.0f;
float                       FCosmicBenchmarkRecorder::AccumulatedGPUTime = 0.0f;
int32                       FCosmicBenchmarkRecorder::FrameCount = 0;
float                       FCosmicBenchmarkRecorder::RecordingElapsedTime = 0.0f;
TArray<float>               FCosmicBenchmarkRecorder::FrameTimes;
TArray<FBenchmarkCSVRow>    FCosmicBenchmarkRecorder::CSVResults;
TArray<FBenchmarkEvent>     FCosmicBenchmarkRecorder::Events;
FString                     FCosmicBenchmarkRecorder::CurrentTestNameContext;

static int32 CurrentNumObjects = 0;

// 
//  VRAM (Windows / DXGI)
// 
static float GetVRAMUsageGB()
{
#if PLATFORM_WINDOWS
    IDXGIFactory4* DXGIFactory = nullptr;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&DXGIFactory);

    if (SUCCEEDED(hr) && DXGIFactory)
    {
        IDXGIAdapter3* Adapter = nullptr;
        hr = DXGIFactory->EnumAdapters(0, (IDXGIAdapter**)&Adapter);

        if (SUCCEEDED(hr) && Adapter)
        {
            DXGI_QUERY_VIDEO_MEMORY_INFO VideoMemoryInfo;
            hr = Adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &VideoMemoryInfo);

            if (SUCCEEDED(hr))
            {
                float VRAM_GB = static_cast<float>(VideoMemoryInfo.CurrentUsage)
                    / (1024.0f * 1024.0f * 1024.0f);
                Adapter->Release();
                DXGIFactory->Release();
                return VRAM_GB;
            }
            Adapter->Release();
        }
        DXGIFactory->Release();
    }
#endif
    return 0.0f;
}

// 
//  Grabación
// 
void FCosmicBenchmarkRecorder::StartRecording()
{
    bIsRecording = true;
    AccumulatedFPS = 0.0f;
    AccumulatedLow1FPS = 0.0f;
    AccumulatedFrameTime = 0.0f;
    AccumulatedGameThreadTime = 0.0f;
    AccumulatedRenderThreadTime = 0.0f;
    AccumulatedGPUTime = 0.0f;
    FrameCount = 0;
    RecordingElapsedTime = 0.0f;
    FrameTimes.Empty();
    FrameTimes.Reserve(1000);

    if (GEngine && GEngine->GameViewport)
    {
        GEngine->Exec(GEngine->GameViewport->GetWorld(), TEXT("stat unit"));
        GEngine->Exec(GEngine->GameViewport->GetWorld(), TEXT("stat fps"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Benchmark recording started"));
}

void FCosmicBenchmarkRecorder::StopRecording()
{
    bIsRecording = false;

    if (GEngine && GEngine->GameViewport)
    {
        GEngine->Exec(GEngine->GameViewport->GetWorld(), TEXT("stat none"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Benchmark recording stopped. Frames recorded: %d"), FrameCount);
}

void FCosmicBenchmarkRecorder::RecordFrame(float DeltaTime)
{
    if (!bIsRecording) return;

    FrameCount++;
    RecordingElapsedTime += DeltaTime;

    float CurrentFPS = DeltaTime > 0.0f ? (1.0f / DeltaTime) : 0.0f;
    AccumulatedFPS += CurrentFPS;

    float FrameTimeMs = DeltaTime * 1000.0f;
    AccumulatedFrameTime += FrameTimeMs;
    FrameTimes.Add(FrameTimeMs);

#if STATS
    float GameThreadTime = FPlatformTime::ToMilliseconds(GGameThreadTime);
    float RenderThreadTime = FPlatformTime::ToMilliseconds(GRenderThreadTime);
    AccumulatedGameThreadTime += GameThreadTime;
    AccumulatedRenderThreadTime += RenderThreadTime;

    float GPUTimeMs = 0.0f;
    if (GDynamicRHI)
    {
        uint32 GPUCycles = RHIGetGPUFrameCycles();
        if (GPUCycles > 0)
            GPUTimeMs = FPlatformTime::ToMilliseconds(GPUCycles);
    }

    if (GPUTimeMs <= 0.0f)
    {
        float CPUTotalMs = GameThreadTime + RenderThreadTime;
        GPUTimeMs = FMath::Max(0.0f, FrameTimeMs - CPUTotalMs);
        if (GPUTimeMs <= 0.0f)
            GPUTimeMs = FrameTimeMs * 0.35f;
    }
    AccumulatedGPUTime += GPUTimeMs;
#else
    AccumulatedGameThreadTime += FrameTimeMs * 0.4f;
    AccumulatedRenderThreadTime += FrameTimeMs * 0.3f;
    AccumulatedGPUTime += FrameTimeMs * 0.3f;
#endif
}

// 
//  Datos
// 
FBenchmarkData FCosmicBenchmarkRecorder::GetCurrentData()
{
    FBenchmarkData Data;
    FMemory::Memzero(&Data, sizeof(FBenchmarkData));
    Data.NumObjects = CurrentNumObjects;

    if (FrameCount > 0)
    {
        Data.AvgFPS = AccumulatedFPS / FrameCount;
        Data.FrameTimeMs = AccumulatedFrameTime / FrameCount;
        Data.GameThreadTimeMs = AccumulatedGameThreadTime / FrameCount;
        Data.RenderThreadTimeMs = AccumulatedRenderThreadTime / FrameCount;
        Data.GPUTimeMs = AccumulatedGPUTime / FrameCount;

        if (FrameTimes.Num() >= 100)
        {
            TArray<float> SortedTimes = FrameTimes;
            SortedTimes.Sort([](float A, float B) { return A > B; });

            int32 NumLowFrames = FMath::Max(1, SortedTimes.Num() / 100);
            float SumLowTimes = 0.0f;
            for (int32 i = 0; i < NumLowFrames; i++)
                SumLowTimes += SortedTimes[i];

            float AvgLowTime = SumLowTimes / NumLowFrames;
            Data.Low1FPS = AvgLowTime > 0.0f ? (1000.0f / AvgLowTime) : 0.0f;
        }
        else
        {
            Data.Low1FPS = Data.AvgFPS;
        }
    }

    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    Data.RAM = MemStats.UsedPhysical / (1024.0f * 1024.0f * 1024.0f);
    Data.VRAM = GetVRAMUsageGB();

    return Data;
}

void FCosmicBenchmarkRecorder::SetCurrentNumObjects(int32 InNumObjects)
{
    CurrentNumObjects = InNumObjects;
}

void FCosmicBenchmarkRecorder::SetCurrentTestName(const FString& InTestName)
{
    CurrentTestNameContext = InTestName;
}

void FCosmicBenchmarkRecorder::LogCurrentData(const FString& Label)
{
    FBenchmarkData Data = GetCurrentData();

    UE_LOG(LogTemp, Warning, TEXT(""));
    UE_LOG(LogTemp, Warning, TEXT("============================================"));
    UE_LOG(LogTemp, Warning, TEXT("=== Benchmark Data [%s] ==="), *Label);
    UE_LOG(LogTemp, Warning, TEXT("============================================"));
    UE_LOG(LogTemp, Warning, TEXT("NumObjects:      %d"), Data.NumObjects);
    UE_LOG(LogTemp, Warning, TEXT("Avg FPS:         %.2f"), Data.AvgFPS);
    UE_LOG(LogTemp, Warning, TEXT("1%% Low FPS:      %.2f"), Data.Low1FPS);
    UE_LOG(LogTemp, Warning, TEXT("Frame Time:      %.2f ms"), Data.FrameTimeMs);
    UE_LOG(LogTemp, Warning, TEXT("  - Game Thread: %.2f ms"), Data.GameThreadTimeMs);
    UE_LOG(LogTemp, Warning, TEXT("  - Render Thrd: %.2f ms"), Data.RenderThreadTimeMs);
    UE_LOG(LogTemp, Warning, TEXT("  - GPU Time:    %.2f ms"), Data.GPUTimeMs);
    UE_LOG(LogTemp, Warning, TEXT("RAM:             %.2f GB"), Data.RAM);
    UE_LOG(LogTemp, Warning, TEXT("VRAM:            %.2f GB"), Data.VRAM);
    UE_LOG(LogTemp, Warning, TEXT("============================================"));

    FString CSVLine = FString::Printf(
        TEXT("CSV,%s,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f"),
        *Label, Data.NumObjects,
        Data.AvgFPS, Data.Low1FPS, Data.FrameTimeMs,
        Data.GameThreadTimeMs, Data.RenderThreadTimeMs, Data.GPUTimeMs,
        Data.RAM, Data.VRAM
    );
    UE_LOG(LogTemp, Warning, TEXT("%s"), *CSVLine);
    UE_LOG(LogTemp, Warning, TEXT("============================================"));
    UE_LOG(LogTemp, Warning, TEXT(""));
}

// 
//  Eventos personalizados
// 

/**
 * Registra un evento en tiempo real desde cualquier sistema.
 *
 * Ejemplo desde C++:
 *   FCosmicBenchmarkRecorder::RecordEvent(TEXT("PlanetLoaded"), TEXT("Terrain listo"), (float)NumVerts);
 *
 * Ejemplo a través del manager (recomendado para blueprints):
 *   UBenchmarkManager::Get(World)->RecordEvent(TEXT("MiEvento"), TEXT("Desc"), 0.f);
 */
void FCosmicBenchmarkRecorder::RecordEvent(const FString& EventName,
    const FString& Description,
    float          NumericValue)
{
    FBenchmarkEvent Evt;
    Evt.EventName = EventName;
    Evt.Description = Description;
    Evt.NumericValue = NumericValue;
    Evt.TimestampSec = RecordingElapsedTime;   // segundos desde el último StartRecording
    Evt.TestContext = CurrentTestNameContext;

    Events.Add(Evt);

    UE_LOG(LogTemp, Log, TEXT("[BenchmarkEvent] [%.2fs | %s] %s — %s (value=%.4f)"),
        Evt.TimestampSec, *Evt.TestContext, *Evt.EventName, *Evt.Description, Evt.NumericValue);
}

void FCosmicBenchmarkRecorder::ClearEvents()
{
    Events.Empty();
}

// 
//  CSV
// 
void FCosmicBenchmarkRecorder::AddCSVResult(const FString& Label)
{
    FBenchmarkData Data = GetCurrentData();

    FBenchmarkCSVRow Row;
    Row.TestName = Label;
    Row.NumObjects = Data.NumObjects;
    Row.AvgFPS = Data.AvgFPS;
    Row.Low1FPS = Data.Low1FPS;
    Row.FrameTimeMs = Data.FrameTimeMs;
    Row.GameThreadTimeMs = Data.GameThreadTimeMs;
    Row.RenderThreadTimeMs = Data.RenderThreadTimeMs;
    Row.GPUTimeMs = Data.GPUTimeMs;
    Row.RAM = Data.RAM;
    Row.VRAM = Data.VRAM;

    CSVResults.Add(Row);
}

void FCosmicBenchmarkRecorder::ExportCSV(const FString& FileName)
{
    FString CSV;

    //  Sección 1: Métricas 
    CSV += TEXT("# BENCHMARK METRICS\n");
    CSV += TEXT("TestName,NumObjects,AvgFPS,Low1FPS,FrameTimeMs,GameThreadMs,RenderThreadMs,GPUTimeMs,RAM_GB,VRAM_GB\n");

    for (const FBenchmarkCSVRow& Row : CSVResults)
    {
        CSV += FString::Printf(
            TEXT("%s,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n"),
            *Row.TestName, Row.NumObjects,
            Row.AvgFPS, Row.Low1FPS, Row.FrameTimeMs,
            Row.GameThreadTimeMs, Row.RenderThreadTimeMs, Row.GPUTimeMs,
            Row.RAM, Row.VRAM
        );
    }

    //  Sección 2: Eventos personalizados 
    if (Events.Num() > 0)
    {
        CSV += TEXT("\n# CUSTOM EVENTS\n");
        CSV += TEXT("EventName,TimestampSec,TestContext,Description,NumericValue\n");

        for (const FBenchmarkEvent& Evt : Events)
        {
            // Sanitize: quitar comas de campos de texto
            FString SafeName = Evt.EventName.Replace(TEXT(","), TEXT(";"));
            FString SafeDesc = Evt.Description.Replace(TEXT(","), TEXT(";"));
            FString SafeCtx = Evt.TestContext.Replace(TEXT(","), TEXT(";"));

            CSV += FString::Printf(
                TEXT("%s,%.3f,%s,%s,%.4f\n"),
                *SafeName, Evt.TimestampSec, *SafeCtx, *SafeDesc, Evt.NumericValue
            );
        }
    }

    //  Escritura a disco 
    FString Directory = FPaths::ProjectSavedDir() / TEXT("Benchmarks/");

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*Directory))
        PlatformFile.CreateDirectoryTree(*Directory);

    FString FullPath = Directory + FileName + TEXT(".csv");
    bool bSuccess = FFileHelper::SaveStringToFile(CSV, *FullPath);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("CSV exported: %s"), *FullPath);
    }    
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to export CSV: %s"), *FullPath);
    }
        
}

void FCosmicBenchmarkRecorder::ClearCSVResults()
{
    CSVResults.Empty();
}
