#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMeshActor.h" 
#include "CosmicSystemGenerator.generated.h"

class UCosmicNoiseClass;
class UMaterialInstance;

/**
 * Generador procedural de sistemas planetarios.
 * Responsable de crear cuerpos celestes, órbitas,
 * materiales y configuraciones orbitales.
 */
UCLASS(HideCategories = (
    Replication, Input, Collision, Actor, LOD, Cooking, Networking,
    Physics, Navigation, Tags, DataLayers, LevelInstance
    ), AutoExpandCategories = ("Configuration", "Generation Rules", "Actions"))
    class COSMICARCHITECTRUNTIME_API ACosmicSystemGenerator : public AActor
{
    GENERATED_BODY()

public:
    ACosmicSystemGenerator();

    /* Texturas base para variación visual procedural. */
    UPROPERTY(EditAnywhere, Category = "Materials")
    TArray<UTexture2D*> PosiblesTexturas;

    /* Material base para planetas terrestres genéricos. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* BaseMaterial;

    /* Material para lunas. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* MoonMaterial;

    /* Material oceánico para planetas con masas de agua. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* OceanMaterial;

    /* Material para estrellas. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* StarMaterial;

    /* Material para gigantes gaseosos. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* GasGiantMaterial;

    /* Material para anillos planetarios. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* RingMaterial;

    /* Grosor de líneas de depuración. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    float LineWidth = 100;

    /* Color de la caja de depuración. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FColor BoxColor = FColor::Blue;

protected:
    UPROPERTY(VisibleDefaultsOnly, Category = "Root", BlueprintReadOnly)
    USceneComponent* Root;

    // CONFIGURATION

    /* Semilla determinista para generación procedural. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    int32 Seed = 1337;

    /* Multiplicador global de velocidades orbitales. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration",
        meta = (ClampMin = "0.0", ClampMax = "100000.0"))
    float OrbitSpeedMultiplier = 1.0f;

    /* ¿Simulación orbital activa en editor? */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Configuration")
    bool bIsSimulatingOrbits = false;

    /** Número total de cuerpos a generar (planetas + lunas, sin contar la estrella). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "1", ClampMax = "200"))
    int32 NumberOfBodies = 5;

    /* Tamaño del volumen procedural en kilómetros. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.1"))
    FVector VolumeSizeKm = FVector(3000.0f, 3000.0f, 5.0f);

    /* Rango de diámetros para planetas (km). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.001"))
    FVector2D BodyDiameterRangeKm = FVector2D(8.f, 15.f);

    /* Rango de diámetros para lunas (km). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.001"))
    FVector2D MoonDiameterRangeKm = FVector2D(2.f, 7.f);

    /** Fracción del radio del sistema que ocupa la estrella. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration|Star", meta = (ClampMin = "0.01", ClampMax = "0.9"))
    float StarRadiusFraction = 0.1f;

    // GENERATION RULES – DISTANCIAS Y RADIOS

    /* Distancia mínima entre centros de cuerpos (km). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Distances", meta = (ClampMin = "0.0"))
    float MinDistanceBetweenBodies = 5.0f;

    /* Distancia máxima al vecino más cercano (0 = sin agrupamiento). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Distances", meta = (ClampMin = "0.0"))
    float MaxDistanceToNearest = 0.0f;

    /* Intentos máximos para encontrar posición válida. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Distances", AdvancedDisplay)
    int32 MaxGenerationAttempts = 100;

    /** Factor sobre el radio de la estrella para la distancia orbital mínima. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Orbits", meta = (ClampMin = "1.0"))
    float OrbitDistanceMinFactor = 3.0f;

    /** Factores para obtener el radio planetario a partir de la distancia orbital. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Planets", meta = (ClampMin = "0.001", ClampMax = "0.5"))
    float PlanetRadiusFactorMin = 0.01f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Planets", meta = (ClampMin = "0.001", ClampMax = "0.5"))
    float PlanetRadiusFactorMax = 0.06f;

    /** Factores para la órbita lunar (multiplican el radio del planeta). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "1.0"))
    float MoonOrbitDistanceFactorMin = 10.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "1.0"))
    float MoonOrbitDistanceFactorMax = 15.0f;

    /** Factores para el radio lunar (multiplican el radio del planeta). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float MoonRadiusFactorMin = 0.1f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float MoonRadiusFactorMax = 0.3f;

    // GENERATION RULES – CLASIFICACIÓN Y PROBABILIDADES

    /** Umbral de relación tamaño/distancia para clasificar como gigante gaseoso. */
    /** Fracción del total de cuerpos que deben quedar por generar para que puedan aparecer gigantes gaseosos.
 *  0.3 = solo cuando quede el 30% o menos de cuerpos por generar.
 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GasGiantAppearanceThreshold = 0.3f;

    /** Probabilidad de que un planeta sea gigante gaseoso cuando se cumple la condición de aparición. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GasGiantProbability = 0.7f;

    /** Factores de radio para gigantes gaseosos (multiplican la distancia orbital). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.001", ClampMax = "0.5"))
    float GasGiantRadiusFactorMin = 0.001f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.001", ClampMax = "0.5"))
    float GasGiantRadiusFactorMax = 0.5f;

    /** Rango de radio (km) para los gigantes gaseosos. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.1"))
    float GasGiantRadiusMin = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Gas Giants",
        meta = (ClampMin = "0.1"))
    float GasGiantRadiusMax = 50.0f;

    /** Zona habitable (fracción del radio del sistema). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HabitableZoneInnerFraction = 0.25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HabitableZoneOuterFraction = 0.65f;

    /** Zona de cinturón de asteroides. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BeltZoneInnerFraction = 0.55f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BeltZoneOuterFraction = 0.70f;

    /** Probabilidad de que un planeta en zona de cinturón sea un cinturón. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BeltProbability = 0.6f;

    /** Probabilidad de anillos para gigantes gaseosos. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GasGiantRingProbability = 0.65f;

    /** Probabilidad de océano en planetas telúricos ubicados en zona habitable. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Classification", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TelluricOceanProbability = 0.7f;

    // =========================================================================
    // GENERATION RULES – RANGOS DE LUNAS
    // =========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0", ClampMax = "20"))
    int32 GasGiantMoonMin = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0", ClampMax = "20"))
    int32 GasGiantMoonMax = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0", ClampMax = "20"))
    int32 TelluricMoonMin = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Moons", meta = (ClampMin = "0", ClampMax = "20"))
    int32 TelluricMoonMax = 3;

    // =========================================================================
    // GENERATION RULES – RESOLUCIONES
    // =========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Graphics", meta = (ClampMin = "16", ClampMax = "512"))
    int32 GasGiantClipResolution = 64;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Graphics", meta = (ClampMin = "16", ClampMax = "512"))
    int32 TelluricClipResolution = 128;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Graphics", meta = (ClampMin = "16", ClampMax = "512"))
    int32 OceanResolutionWithOcean = 128;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules|Graphics", meta = (ClampMin = "16", ClampMax = "512"))
    int32 OceanResolutionWithoutOcean = 64;

    /** Cuerpos generados (sólo referencia interna). */
    UPROPERTY()
    TArray<AActor*> GeneratedBodies;

#if WITH_EDITOR
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    virtual void Tick(float DeltaTime) override;
#endif

private:
    enum class EPlanetType
    {
        GasGiant,
        Telluric,
        AsteroidBelt
    };

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
        float OrbitDistanceKm,
        float PlanetRadiusKm,
        float SystemRadiusKm,
        FRandomStream& Stream,
        int32 RemainingBodies,
        int32 TotalBodies) const;

    UCosmicNoiseClass* CreateRandomNoiseSettings(FRandomStream& Stream, float PlanetRadius);

    FColor GetRandomColor(FRandomStream& Stream, int min, int max);

    /** Intenta colocar un planeta respetando distancias. Retorna true si se colocó. */
    bool TryPlacePlanet(
        FRandomStream& Stream,
        float SystemRadiusKm,
        float StarRadiusKm,
        const TArray<float>& ExistingOrbitDistances,
        const TArray<float>& ExistingPlanetRadii,
        float& OutOrbitDistance,
        float& OutPlanetRadius,
        bool bIsGasGiant) const;

    /** Verifica si la distancia orbital propuesta es válida frente a las existentes. */
    bool IsOrbitDistanceValid(
        float ProposedOrbitKm,
        float ProposedRadiusKm,
        const TArray<float>& ExistingOrbits,
        const TArray<float>& ExistingRadii) const;

public:
    void SetNumBodies(int32 NumBodies);

    UFUNCTION(CallInEditor, Category = "Actions")
    void GenerateBodies();

    UFUNCTION(CallInEditor, Category = "Actions")
    void GenerateWithRandomSeed();

    UFUNCTION(CallInEditor, Category = "Actions")
    void ClearBodies();

    UFUNCTION(CallInEditor, Category = "Actions")
    void StartOrbitSimulation();

    UFUNCTION(CallInEditor, Category = "Actions")
    void StopOrbitSimulation();
};