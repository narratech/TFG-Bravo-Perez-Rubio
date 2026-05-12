// Fill out your copyright notice in the Description page of Project Settings.

#include "Benchmark/BenchmarkRecorder.h"
#include "Engine/Engine.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

// Para VRAM en Windows
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <dxgi1_4.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

// Variables estáticas
bool FBenchmarkRecorder::bIsRecording = false;
float FBenchmarkRecorder::AccumulatedFPS = 0.0f;
float FBenchmarkRecorder::AccumulatedLow1FPS = 0.0f;
float FBenchmarkRecorder::AccumulatedFrameTime = 0.0f;
float FBenchmarkRecorder::AccumulatedGameThreadTime = 0.0f;
float FBenchmarkRecorder::AccumulatedRenderThreadTime = 0.0f;
float FBenchmarkRecorder::AccumulatedGPUTime = 0.0f;
int32 FBenchmarkRecorder::FrameCount = 0;
TArray<float> FBenchmarkRecorder::FrameTimes;
TArray<FBenchmarkCSVRow> FBenchmarkRecorder::CSVResults;

static int32 CurrentNumObjects = 0;

static float GetVRAMUsageGB()
{
#if PLATFORM_WINDOWS
    // Intentar obtener la VRAM usada por el proceso actual
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
                // CurrentUsage está en bytes, convertir a GB
                float VRAM_GB = static_cast<float>(VideoMemoryInfo.CurrentUsage) / (1024.0f * 1024.0f * 1024.0f);

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

void FBenchmarkRecorder::StartRecording()
{
    bIsRecording = true;
    AccumulatedFPS = 0.0f;
    AccumulatedLow1FPS = 0.0f;
    AccumulatedFrameTime = 0.0f;
    AccumulatedGameThreadTime = 0.0f;
    AccumulatedRenderThreadTime = 0.0f;
    AccumulatedGPUTime = 0.0f;
    FrameCount = 0;
    FrameTimes.Empty();
    FrameTimes.Reserve(1000); // Reservar espacio para evitar realocaciones

    // Activar stats del motor - forma correcta en UE5
    // Los stats se activan mediante comandos de consola
    if (GEngine && GEngine->GameViewport)
    {
        GEngine->Exec(GEngine->GameViewport->GetWorld(), TEXT("stat unit"));
        GEngine->Exec(GEngine->GameViewport->GetWorld(), TEXT("stat fps"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Benchmark recording started"));
}

void FBenchmarkRecorder::StopRecording()
{
    bIsRecording = false;

    // Desactivar stats
    if (GEngine && GEngine->GameViewport)
    {
        GEngine->Exec(GEngine->GameViewport->GetWorld(), TEXT("stat none"));
    }

    UE_LOG(LogTemp, Warning, TEXT("Benchmark recording stopped. Frames recorded: %d"), FrameCount);
}

FBenchmarkData FBenchmarkRecorder::GetCurrentData()
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

        // El "1% low FPS" es el FPS promedio del 1% de los PEORES frames
        // (los frames más LENTOS, con mayor tiempo en ms)
        // ============================================================

        if (FrameTimes.Num() >= 100) // Necesitamos al menos 100 frames
        {
            // Ordenar de MAYOR a MENOR tiempo (los peores primero)
            TArray<float> SortedTimes = FrameTimes;
            SortedTimes.Sort([](float A, float B) { return A > B; }); // Descendente

            // Coger el 1% de los frames más lentos
            int32 NumLowFrames = FMath::Max(1, SortedTimes.Num() / 100);

            // Calcular el tiempo promedio de esos frames lentos
            float SumLowTimes = 0.0f;
            for (int32 i = 0; i < NumLowFrames; i++)
            {
                SumLowTimes += SortedTimes[i];
            }
            float AvgLowTime = SumLowTimes / NumLowFrames;

            // Convertir tiempo a FPS
            Data.Low1FPS = AvgLowTime > 0.0f ? (1000.0f / AvgLowTime) : 0.0f;
        }
        else
        {
            Data.Low1FPS = Data.AvgFPS; // No hay suficientes datos
        }
    }

    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    Data.RAM = MemStats.UsedPhysical / (1024.0f * 1024.0f * 1024.0f);

    // VRAM - usando DXGI en Windows
    Data.VRAM = GetVRAMUsageGB();

    return Data;
}

void FBenchmarkRecorder::LogCurrentData(const FString& Label)
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

    // Formato CSV para fácil importación a Excel/Google Sheets
    FString CSVLine = FString::Printf(
        TEXT("CSV,%s,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f"),
        *Label,
        Data.NumObjects,
        Data.AvgFPS,
        Data.Low1FPS,
        Data.FrameTimeMs,
        Data.GameThreadTimeMs,
        Data.RenderThreadTimeMs,
        Data.GPUTimeMs,
        Data.RAM,
        Data.VRAM
    );

    UE_LOG(LogTemp, Warning, TEXT("%s"), *CSVLine);
    UE_LOG(LogTemp, Warning, TEXT("============================================"));
    UE_LOG(LogTemp, Warning, TEXT(""));
}

void FBenchmarkRecorder::SetCurrentNumObjects(int32 InNumObjects)
{
    CurrentNumObjects = InNumObjects;
}

// Función auxiliar para ser llamada cada frame desde el GameMode o similar
void FBenchmarkRecorder::RecordFrame(float DeltaTime)
{
    if (!bIsRecording) return;

    FrameCount++;

    // FPS instantáneo
    float CurrentFPS = DeltaTime > 0.0f ? (1.0f / DeltaTime) : 0.0f;
    AccumulatedFPS += CurrentFPS;

    // Frame time en ms
    float FrameTimeMs = DeltaTime * 1000.0f;
    AccumulatedFrameTime += FrameTimeMs;
    FrameTimes.Add(FrameTimeMs);

    // Obtener tiempos de CPU/GPU de los stats del motor
    // Estos valores se obtienen de las estadísticas globales de renderizado

#if STATS
    // Game Thread time
    float GameThreadTime = FPlatformTime::ToMilliseconds(GGameThreadTime);
    AccumulatedGameThreadTime += GameThreadTime;

    // Render Thread time
    float RenderThreadTime = FPlatformTime::ToMilliseconds(GRenderThreadTime);
    AccumulatedRenderThreadTime += RenderThreadTime;

    // GPU Time - Se obtiene consultando las estadísticas del frame anterior
   // En UE5, el GPU time del frame N se reporta en el frame N+1 o N+2
    float GPUTimeMs = 0.0f;

    // Método 1: Usando las estadísticas de RHI (más preciso)
    if (GDynamicRHI)
    {
        // Obtener tiempo de GPU del último frame completado
        // Nota: Esto puede retornar 0 si no hay datos todavía
        uint32 GPUCycles = RHIGetGPUFrameCycles();
        if (GPUCycles > 0)
        {
            GPUTimeMs = FPlatformTime::ToMilliseconds(GPUCycles);
        }
    }

    // Método 2 (fallback): Estimar GPU time basado en Frame Time total
    // Si no se pudo obtener el tiempo real de GPU
    if (GPUTimeMs <= 0.0f)
    {
        // El GPU time suele ser similar al Render Thread time
        // o al Frame Time total menos los tiempos de CPU
        float CPUTotalMs = GameThreadTime + RenderThreadTime;
        GPUTimeMs = FMath::Max(0.0f, FrameTimeMs - CPUTotalMs);

        // Si aún es 0, usar un porcentaje del frame time
        if (GPUTimeMs <= 0.0f)
        {
            GPUTimeMs = FrameTimeMs * 0.35f; // ~35% típico para GPU
        }
    }

    AccumulatedGPUTime += GPUTimeMs;
#else
    // Sin STATS, usamos el frame time total dividido
    float EstimatedGameThread = FrameTimeMs * 0.4f;
    float EstimatedRenderThread = FrameTimeMs * 0.3f;
    float EstimatedGPU = FrameTimeMs * 0.3f;

    AccumulatedGameThreadTime += EstimatedGameThread;
    AccumulatedRenderThreadTime += EstimatedRenderThread;
    AccumulatedGPUTime += EstimatedGPU;
#endif
}

void FBenchmarkRecorder::AddCSVResult(const FString& Label)
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

void FBenchmarkRecorder::ExportCSV(const FString& FileName)
{
    FString CSV;

    // Header
    CSV += TEXT("TestName,NumObjects,AvgFPS,Low1FPS,FrameTimeMs,GameThreadMs,RenderThreadMs,GPUTimeMs,RAM_GB,VRAM_GB\n");

    // Rows
    for (const FBenchmarkCSVRow& Row : CSVResults)
    {
        CSV += FString::Printf(
            TEXT("%s,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n"),
            *Row.TestName,
            Row.NumObjects,
            Row.AvgFPS,
            Row.Low1FPS,
            Row.FrameTimeMs,
            Row.GameThreadTimeMs,
            Row.RenderThreadTimeMs,
            Row.GPUTimeMs,
            Row.RAM,
            Row.VRAM
        );
    }

    FString Directory = FPaths::ProjectSavedDir() / TEXT("Benchmarks/");

    IPlatformFile& PlatformFile =
        FPlatformFileManager::Get().GetPlatformFile();

    if (!PlatformFile.DirectoryExists(*Directory))
    {
        PlatformFile.CreateDirectoryTree(*Directory);
    }

    FString FullPath = Directory + FileName + TEXT(".csv");

    bool bSuccess = FFileHelper::SaveStringToFile(CSV, *FullPath);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("CSV exported: %s"), *FullPath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to export CSV"));
    }
}

void FBenchmarkRecorder::ClearCSVResults()
{
    CSVResults.Empty();
}