#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMeshActor.h" 
#include "CosmicSystemGenerator.generated.h"

class UCosmicNoiseClass;
class UMaterialInstance;

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

    UPROPERTY(EditAnywhere, Category = "Materials")
    TArray<UTexture2D*> PosiblesTexturas;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* BaseMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* MoonMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* OceanMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* StarMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* GasGiantMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* RingMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    float LineWidth = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FColor BoxColor = FColor::Blue;

protected:

    UPROPERTY(VisibleDefaultsOnly, Category = "Root", BlueprintReadOnly)
    USceneComponent* Root;

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

#if WITH_EDITOR
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    virtual void Tick(float DeltaTime) override;
#endif

private:
    enum class EPlanetType { GasGiant, Telluric, AsteroidBelt };

    struct FPlanetClassification
    {
        EPlanetType Type;
        bool bHasOcean;
        float OceanSeaLevel;
        bool bHasRings;
        bool bHasMoons;
        int32 MaxMoons;
    };

    FPlanetClassification ClassifyPlanet(
        float OrbitDistanceKm, float PlanetRadiusKm,
        float SystemRadiusKm, FRandomStream& Stream) const;

    UCosmicNoiseClass* CreateRandomNoiseSettings(FRandomStream& Stream, float PlanetRadius);
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