// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "CosmicFoliageCollection.h"

void UCosmicFoliageCollection::PostLoad()
{
    Super::PostLoad();

#if WITH_EDITOR
    BindBiomeDelegates();
#endif
}

void UCosmicFoliageCollection::BeginDestroy()
{
#if WITH_EDITOR
    UnbindBiomeDelegates();
#endif

    Super::BeginDestroy();
}

#if WITH_EDITOR
void UCosmicFoliageCollection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent); 

    BindBiomeDelegates();
    OnFoliageCollectionChanged.Broadcast();
}

void UCosmicFoliageCollection::BindBiomeDelegates()
{
    UnbindBiomeDelegates();

    for (const FCosmicFoliageCollectionEntry& Entry : FoliageEntries)
    {
        if (Entry.FoliageBiome && !BoundBiomes.Contains(Entry.FoliageBiome))
        {
            Entry.FoliageBiome->OnFoliageBiomeChanged.AddUObject(this, &UCosmicFoliageCollection::HandleBiomeChanged);
            BoundBiomes.Add(Entry.FoliageBiome);
        }
    }
}

void UCosmicFoliageCollection::UnbindBiomeDelegates()
{
    for (const TWeakObjectPtr<UCosmicFoliageBiome>& BiomePtr : BoundBiomes)
    {
        if (BiomePtr.IsValid())
        {
            BiomePtr->OnFoliageBiomeChanged.RemoveAll(this);
        }
    }
    BoundBiomes.Empty();
}

void UCosmicFoliageCollection::HandleBiomeChanged()
{
    OnFoliageCollectionChanged.Broadcast();
}
#endif