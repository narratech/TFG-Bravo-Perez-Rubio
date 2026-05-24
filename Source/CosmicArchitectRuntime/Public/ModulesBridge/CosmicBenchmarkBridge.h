// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct COSMICARCHITECTRUNTIME_API FCosmicBenchmarkBridge
{
    static TFunction<void(const FString&, const FString&, float)> OnRecordEvent;

    static void RecordEvent(const FString& Name,
        const FString& Desc = TEXT(""),
        float          Value = 0.f)
    {
        if (OnRecordEvent)
            OnRecordEvent(Name, Desc, Value);
    }
}; 