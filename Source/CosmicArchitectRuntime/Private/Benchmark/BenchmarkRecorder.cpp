// Fill out your copyright notice in the Description page of Project Settings.

#include "Benchmark/BenchmarkRecorder.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "RenderingThread.h"
#include "EngineGlobals.h"
#include "HAL/PlatformMemory.h"
#include "RHI.h"
#include "Stats/Stats.h"
#include "ProfilingDebugging/CsvProfiler.h"

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

    if (FrameCount > 0)
    {
        Data.AvgFPS = AccumulatedFPS / FrameCount;
        Data.FrameTimeMs = AccumulatedFrameTime / FrameCount;
        Data.GameThreadTimeMs = AccumulatedGameThreadTime / FrameCount;
        Data.RenderThreadTimeMs = AccumulatedRenderThreadTime / FrameCount;
        Data.GPUTimeMs = AccumulatedGPUTime / FrameCount;

        // Calcular 1% low
        if (FrameTimes.Num() > 0)
        {
            FrameTimes.Sort();
            int32 Low1Index = FMath::Max(0, FrameTimes.Num() / 100);
            float Low1FrameTime = FrameTimes[Low1Index];
            Data.Low1FPS = Low1FrameTime > 0.0f ? (1000.0f / Low1FrameTime) : 0.0f;
        }
    }

    // Obtener estadísticas de memoria del sistema
    FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();
    Data.RAM = static_cast<float>(MemStats.UsedPhysical) / (1024.0f * 1024.0f * 1024.0f); // Bytes -> GB

    // Obtener VRAM - forma correcta en UE5
    // Usamos la interfaz de RHI para consultar memoria de GPU
    if (GDynamicRHI)
    {
        // Obtener estadísticas de textura como aproximación de VRAM
        // Nota: No hay una API directa para VRAM total en UE5 pública
        // Usamos el contador de memoria de texturas como estimación
        Data.VRAM = 0.0f;

        // Alternativa: usar GCurrentRendertargetMemorySize o similar
#if STATS
// Los stats de memoria están disponibles cuando STATS está habilitado
// Estos se pueden leer desde el sistema de stats de UE
#endif
    }

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

    // GPU time - disponible a través del profiler de GPU
    float GPUTime = 0.0f;
    AccumulatedGPUTime += GPUTime;
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