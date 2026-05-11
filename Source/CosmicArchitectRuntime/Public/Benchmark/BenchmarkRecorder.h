// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


struct FBenchmarkCSVRow
{
    FString TestName;
    int32 NumObjects = 0;

    float AvgFPS = 0.0f;
    float Low1FPS = 0.0f;

    float FrameTimeMs = 0.0f;
    float GameThreadTimeMs = 0.0f;
    float RenderThreadTimeMs = 0.0f;
    float GPUTimeMs = 0.0f;

    float RAM = 0.0f;
    float VRAM = 0.0f;
};

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

class FBenchmarkRecorder
{
public:
    static void StartRecording();
    static void StopRecording();
    static FBenchmarkData GetCurrentData();
    static void LogCurrentData(const FString& Label = TEXT(""));
    static void RecordFrame(float DeltaTime);
    static void AddCSVResult(const FString& Label);
    static void ExportCSV(const FString& FileName);
    static void ClearCSVResults();

private:
    static bool bIsRecording;
    static float AccumulatedFPS;
    static float AccumulatedLow1FPS;
    static float AccumulatedFrameTime;
    static float AccumulatedGameThreadTime;
    static float AccumulatedRenderThreadTime;
    static float AccumulatedGPUTime;
    static int32 FrameCount;
    static TArray<float> FrameTimes;
    static TArray<FBenchmarkCSVRow> CSVResults;
};
