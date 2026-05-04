// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

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
};
