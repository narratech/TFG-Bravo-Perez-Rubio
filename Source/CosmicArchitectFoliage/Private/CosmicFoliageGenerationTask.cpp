// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicFoliageGenerationTask.h"

// TAREA ASINCRONA 
void FFoliageGenerationTask::DoWork()
{
    if (!Collection) return;

    //FRandomStream LocalRandom(Seed);

    //// PASO 1: Generar puntos de semilla en la esfera
    //GenerateSeedPoints(LocalRandom);

    //// PASO 2: Evaluar condiciones ambientales para cada punto
    //EvaluateEnvironmentalConditions(LocalRandom);

    //// PASO 3: Seleccionar y crear instancias basadas en las condiciones
    //CreateFoliageInstances(LocalRandom);
}

void FFoliageGenerationTask::GenerateSeedPoints(FRandomStream& Random)
{
}

void FFoliageGenerationTask::EvaluateEnvironmentalConditions(FRandomStream& Random)
{
}

float FFoliageGenerationTask::CalculateTerrainHeight(const FVector& Direction, FRandomStream& Random)
{
    return 0.0f;
}

float FFoliageGenerationTask::CalculateSlope(const FVector& Direction, FRandomStream& Random)
{
    return 0.0f;
}

void FFoliageGenerationTask::CreateFoliageInstances(FRandomStream& Random)
{
}

const FCosmicFoliageCollectionEntry* FFoliageGenerationTask::FindBestMatchingEntry(float Temperature, float Humidity, float Slope, float Height)
{
    return nullptr;
}

const FCosmicFoliageCollectionEntry* FFoliageGenerationTask::FindClosestMatchingEntry(float Temperature, float Humidity, float Slope, float Height)
{
    return nullptr;
}

FVector FFoliageGenerationTask::GetTerrainNormal(const FVector& Direction, FRandomStream& Random)
{
    return FVector();
}
