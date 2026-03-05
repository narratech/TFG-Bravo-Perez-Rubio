// Fill out your copyright notice in the Description page of Project Settings.


#include "CosmicFoliageCollection.h"

const FCosmicFoliageCollectionEntry* UCosmicFoliageCollection::GetRandomEntry(FRandomStream& Random) const
{
    if (FoliageEntries.Num() == 0) return nullptr;
    if (FoliageEntries.Num() == 1) return &FoliageEntries[0];

    // Calcular peso total
    float TotalWeight = 0.0f;
    for (const auto& Entry : FoliageEntries)
    {
        TotalWeight += Entry.Weight;
    }

    // Selección aleatoria ponderada
    float RandomValue = Random.FRandRange(0.0f, TotalWeight);
    float Accumulated = 0.0f;

    for (const auto& Entry : FoliageEntries)
    {
        Accumulated += Entry.Weight;
        if (RandomValue <= Accumulated)
        {
            return &Entry;
        }
    }

    return &FoliageEntries.Last();
}
