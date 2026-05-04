// Fill out your copyright notice in the Description page of Project Settings.


#include "Benchmark/BenchmarkRunner.h"
#include "Benchmark/BenchmarkManager.h"

void FBenchmarkRunner::RunPlanetScaling(UBenchmarkManager* Manager)
{
    if (!Manager) return;

    TArray<int32> Steps = { 1, 2, 4, 8, 16, 32 };

    for (int32 Num : Steps)
    {
        Manager->ClearPlanets();
        Manager->SpawnPlanets(Num);

        Manager->BeginCapture();

        // TODO: esperar X segundos (timers)

        Manager->EndCapture();
    }
}