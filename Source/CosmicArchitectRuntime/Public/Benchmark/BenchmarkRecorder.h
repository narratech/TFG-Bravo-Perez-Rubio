// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct FBenchmarkData
{
    int32 NumObjects;
    float AvgFPS;
    float Low1FPS;
    float FrameTimeMs;
    float RAM;
    float VRAM;
};

class FBenchmarkRecorder
{
public:

    static void StartRecording();
    static void StopRecording();

    static FBenchmarkData GetCurrentData();
};
