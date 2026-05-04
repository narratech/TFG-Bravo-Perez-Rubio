// Fill out your copyright notice in the Description page of Project Settings.


#include "Benchmark/BenchmarkManager.h"
#include "Engine/World.h"

UBenchmarkManager* UBenchmarkManager::Get(UWorld* World)
{
    if (!World) return nullptr;
    return World->GetSubsystem<UBenchmarkManager>();
}

void UBenchmarkManager::StartBenchmark()
{
    UE_LOG(LogTemp, Warning, TEXT("Benchmark started"));
}

void UBenchmarkManager::StopBenchmark()
{
    UE_LOG(LogTemp, Warning, TEXT("Benchmark finished"));
}

void UBenchmarkManager::SpawnPlanets(int32 NumPlanets)
{
    UE_LOG(LogTemp, Warning, TEXT("SpawnPlanets: %d"), NumPlanets);

    // TODO: llamar a tu sistema de generación
}

void UBenchmarkManager::ClearPlanets()
{
    UE_LOG(LogTemp, Warning, TEXT("ClearPlanets"));

    // TODO: limpiar escena
}

void UBenchmarkManager::RunPlanetScalingTest()
{
    UE_LOG(LogTemp, Warning, TEXT("RunPlanetScalingTest"));

    // TODO: implementar secuencia 1,2,4,8...
}

void UBenchmarkManager::RunFoliageTest()
{
    UE_LOG(LogTemp, Warning, TEXT("RunFoliageTest"));
}

void UBenchmarkManager::RunSimulationTest()
{
    UE_LOG(LogTemp, Warning, TEXT("RunSimulationTest"));
}

void UBenchmarkManager::BeginCapture()
{
    UE_LOG(LogTemp, Warning, TEXT("Capture Start"));
}

void UBenchmarkManager::EndCapture()
{
    UE_LOG(LogTemp, Warning, TEXT("Capture End"));
}