// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

// 
//  Fila de métricas de rendimiento (CSV)
// 
struct FBenchmarkCSVRow
{
    FString TestName;
    int32   NumObjects = 0;

    float AvgFPS = 0.0f;
    float Low1FPS = 0.0f;

    float FrameTimeMs = 0.0f;
    float GameThreadTimeMs = 0.0f; 
    float RenderThreadTimeMs = 0.0f;
    float GPUTimeMs = 0.0f;

    float RAM = 0.0f;
    float VRAM = 0.0f;
};

// 
//  Evento personalizado registrable en tiempo real
//  Úsalo desde cualquier sistema con:
//    FBenchmarkRecorder::RecordEvent("MiEvento", "Descripción", 42.f);
//  o a través del manager:
//    UBenchmarkManager::Get(World)->RecordEvent(...);
// 
struct FBenchmarkEvent
{
    FString EventName;                  // Nombre corto (aparece en CSV)
    FString Description;                // Descripción libre
    float   NumericValue = 0.0f;     // Valor numérico opcional (ej: temperatura, distancia)
    float   TimestampSec = 0.0f;     // Segundos desde el inicio de la grabación activa
    FString TestContext;                // Nombre del test que estaba corriendo cuando se registró
};

// 
//  Snapshot de datos del frame actual
// 
struct FBenchmarkData
{
    int32 NumObjects;
    float AvgFPS;
    float Low1FPS;
    float FrameTimeMs;
    float GameThreadTimeMs;
    float RenderThreadTimeMs;
    float GPUTimeMs;
    float RAM;
    float VRAM;
};


class FCosmicBenchmarkRecorder
{
public:
    //  Grabación 
    static void StartRecording();
    static void StopRecording();
    static void RecordFrame(float DeltaTime);

    //  Datos 
    static FBenchmarkData GetCurrentData();
    static void           LogCurrentData(const FString& Label = TEXT(""));
    static void           SetCurrentNumObjects(int32 InNumObjects);
    static void           SetCurrentTestName(const FString& InTestName);

    //  Eventos personalizados 
    //  Llama a este método desde CUALQUIER sistema para anotar un evento.
    //  Los eventos se exportan en una sección separada del CSV.
    static void RecordEvent(const FString& EventName,
        const FString& Description = TEXT(""),
        float          NumericValue = 0.0f);
    static void ClearEvents();
    static const TArray<FBenchmarkEvent>& GetEvents() { return Events; }

    //  CSV 
    static void AddCSVResult(const FString& Label);
    static void ExportCSV(const FString& FileName);
    static void ClearCSVResults();

private:
    // Grabación
    static bool  bIsRecording;
    static float AccumulatedFPS;
    static float AccumulatedLow1FPS;
    static float AccumulatedFrameTime;
    static float AccumulatedGameThreadTime;
    static float AccumulatedRenderThreadTime;
    static float AccumulatedGPUTime;
    static int32 FrameCount;
    static float RecordingElapsedTime;   // Tiempo acumulado desde StartRecording

    static TArray<float>             FrameTimes;
    static TArray<FBenchmarkCSVRow>  CSVResults;
    static TArray<FBenchmarkEvent>   Events;

    // Contexto actual (para etiquetar eventos)
    static FString CurrentTestNameContext;
};