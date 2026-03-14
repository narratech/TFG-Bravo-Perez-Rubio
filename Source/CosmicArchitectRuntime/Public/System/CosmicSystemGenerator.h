#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMeshActor.h" 
#include "CosmicNoiseSettings.h"
#include "CosmicSystemGenerator.generated.h"

// E: Configuramos la clase ocultando categorías innecesarias para mantener el editor limpio.
// I: Configuring the class by hiding unnecessary categories to keep the editor clean.
UCLASS(HideCategories = (
    Replication, Input, Collision, Actor, LOD, Cooking, Networking,
    Physics, Navigation, Tags, DataLayers, LevelInstance
    ), AutoExpandCategories = ("Configuration", "Generation Rules", "Actions"))
    class COSMICARCHITECTRUNTIME_API ACosmicSystemGenerator : public AActor
{
    GENERATED_BODY()

public:
    // E: Constructor por defecto.
    // I: Default constructor.
    ACosmicSystemGenerator();

    // E: Se llama cuando una propiedad cambia en el editor. Permite ver cambios en tiempo real.
    // I: Called when a property is changed in the editor. Allows real-time updates.
    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    // E: Componente visual para definir el área de generación.
    // I: Visual component to define the generation area.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visualization")
    UBoxComponent* GenerationVolume;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInterface* BaseMaterial;

    // --- CONFIGURATION ---

    // E: Semilla para la generación de números aleatorios (Determinismo).
    // I: Seed for random number generation (Determinism).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    int32 Seed;

    // E: Cantidad de cuerpos (esferas) a generar.
    // I: Amount of bodies (spheres) to generate.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "1", ClampMax = "200"))
    int32 NumberOfBodies;

    // E: Tamaño del volumen de generación en Kilómetros.
    // I: Size of the generation volume in Kilometers.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.1"))
    FVector VolumeSizeKm;

    // E: Malla estática usada para las esferas.
    // I: Static mesh used for the spheres.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    UStaticMesh* SphereMesh;

    // E: Rango de diámetro aleatorio en Kilómetros (Mín, Máx) para cada cuerpo.
        // I: Random diameter range in Kilometers (Min, Max) for each body.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.001"))
    FVector2D BodyDiameterRangeKm;

    // --- GENERATION RULES ---

    // E: Distancia mínima entre cuerpos para evitar solapamientos.
    // I: Minimum distance between bodies to prevent overlapping.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules", meta = (ClampMin = "0.0"))
    float MinDistanceBetweenBodies;

    // E: Distancia máxima al vecino más cercano. Si es > 0, fuerza a los cuerpos a agruparse.
    // I: Maximum distance to the nearest neighbor. If > 0, forces bodies to cluster together.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules", meta = (ClampMin = "0.0"))
    float MaxDistanceToNearest;

    // E: Número máximo de intentos para encontrar una posición válida antes de cancelar un cuerpo.
    // I: Maximum number of attempts to find a valid position before skipping a body.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules", AdvancedDisplay)
    int32 MaxGenerationAttempts;

    // E: Array para almacenar referencias a los actores generados y poder borrarlos luego.
    // I: Array to store references to generated actors to allow later deletion.
    UPROPERTY()
    TArray<AActor*> GeneratedBodies;



    void GenerateStar();

private:
    UCosmicNoiseSettings* CreateRandomNoiseSettings(FRandomStream& Stream, const float PlanetRadius);

    FColor GetRandomColor(FRandomStream& Stream, int min, int max);

public:
    // E: Genera los cuerpos basándose en la configuración actual.
    // I: Generates bodies based on the current configuration.
    UFUNCTION(CallInEditor, Category = "Actions")
    void GenerateBodies();

    // E: Crea una nueva semilla aleatoria y regenera los cuerpos inmediatamente.
    // I: Creates a new random seed and regenerates bodies immediately.
    UFUNCTION(CallInEditor, Category = "Actions")
    void GenerateWithRandomSeed();

    // E: Elimina todos los actores generados previamente.
    // I: Destroys all previously generated actors.
    UFUNCTION(CallInEditor, Category = "Actions")
    void ClearBodies();
};