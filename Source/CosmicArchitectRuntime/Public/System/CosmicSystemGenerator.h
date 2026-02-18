// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "CosmicSystemGenerator.generated.h"

UCLASS(HideCategories = (
    Rendering,          // Oculta mallas, sombras, visibilidad
    Replication,        // Oculta red
    Input,              // Oculta input
    Collision,          // Oculta colisiones
    Actor,              // Oculta Tick, Spawn
    LOD,                // Oculta Level of Detail
    Cooking,
    Networking,
    Physics,            // Oculta físicas (Gravity, Mass)
    Navigation,         // Oculta NavMesh
    Tags,               // Oculta etiquetas de actor
    DataLayers,         // Oculta capas de datos
    LevelInstance       // Oculta instancias de nivel
    ), AutoExpandCategories = ("Configuration", "Actions")) // Abre tus categorías automáticamente

    class COSMICARCHITECTRUNTIME_API ACosmicSystemGenerator : public AActor
{
    GENERATED_BODY()

public:
    ACosmicSystemGenerator();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generation")
    UBoxComponent* GenerationVolume;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    TSubclassOf<AActor> ClassToGenerate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "1", ClampMax = "50"))
    int32 NumberOfBodies;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    int32 Seed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.1"))
    FVector VolumeSizeKm;

    UPROPERTY()
    TArray<AActor*> GeneratedBodies;

public:
    UFUNCTION(CallInEditor, Category = "Actions")
    void GenerateBodies();

    UFUNCTION(CallInEditor, Category = "Actions")
    void GenerateWithRandomSeed();

    UFUNCTION(CallInEditor, Category = "Actions")
    void ClearBodies();

    UFUNCTION(CallInEditor, Category = "Actions")
    void UpdateVolumeSize();
};
