// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UBenchmarkManager;

class FBenchmarkRunner
{
public:
    // Test principal de escalado de planetas 
    static void RunPlanetScaling(UBenchmarkManager* Manager);

    // Test de planetas cercanos (máximo detalle) 
    static void RunClosePlanetScaling(UBenchmarkManager* Manager);

    // Tests de Foliage 
    static void RunFoliageBenchmark(UBenchmarkManager* Manager);

    // Tests de Clipmap 
    static void RunClipmapBenchmark(UBenchmarkManager* Manager);

    // Tests de Simulación
    static void RunSimulationBenchmark(UBenchmarkManager* Manager);

    // Test de Generador de Sistemas 
    static void RunSystemGeneratorBenchmark(UBenchmarkManager* Manager);

    // Suite completa de benchmarks 
    static void RunFullBenchmarkSuite(UBenchmarkManager* Manager);

private:
    // Funciones internas para ejecución secuencial 
    static void ExecuteNextStep();
    static void OnTestComplete();
};