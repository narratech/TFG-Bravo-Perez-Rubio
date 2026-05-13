#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMeshActor.h" 
#include "CosmicSystemGenerator.generated.h"

class UCosmicNoiseClass;
class UMaterialInstance;

/**
 * E: Generador procedural de sistemas planetarios.
 * Responsable de crear cuerpos celestes, órbitas,
 * materiales y configuraciones orbitales.
 *

 */
UCLASS(HideCategories = (
    Replication, Input, Collision, Actor, LOD, Cooking, Networking,
    Physics, Navigation, Tags, DataLayers, LevelInstance
    ), AutoExpandCategories = ("Configuration", "Generation Rules", "Actions"))
    class COSMICARCHITECTRUNTIME_API ACosmicSystemGenerator : public AActor
{
    GENERATED_BODY()

public:

    /**
     * E: Constructor principal del generador.
     *
     */
    ACosmicSystemGenerator();

    /**
     * E: Conjunto de texturas utilizadas para
     * variaciones visuales procedurales.
     *

     */
    UPROPERTY(EditAnywhere, Category = "Materials")
    TArray<UTexture2D*> PosiblesTexturas;

    /**
     * E: Material base utilizado para
     * planetas terrestres genéricos.
     *

     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* BaseMaterial;

    /**
     * E: Material utilizado para lunas.
     *
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* MoonMaterial;

    /**
     * E: Material oceánico utilizado para
     * planetas con masas de agua.
     *

     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* OceanMaterial;

    /**
     * E: Material utilizado para estrellas.
     *
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* StarMaterial;

    /**
     * E: Material específico para
     * gigantes gaseosos.
     *

     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* GasGiantMaterial;

    /**
     * E: Material utilizado para
     * anillos planetarios.
     *

     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* RingMaterial;

    /**
     * E: Grosor visual utilizado para
     * líneas de depuración.
     *

     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    float LineWidth = 100;

    /**
     * E: Color utilizado para
     * depuración visual.
     *

     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FColor BoxColor = FColor::Blue;

protected:

    /**
     * E: Componente raíz del actor.
     *
     */
    UPROPERTY(VisibleDefaultsOnly, Category = "Root", BlueprintReadOnly)
    USceneComponent* Root;

    // =========================================================================
    // CONFIGURATION
    // =========================================================================

    /**
     * E: Semilla determinista utilizada
     * para generación procedural.
     *
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration")
    int32 Seed = 1337;

    /**
     * E: Multiplicador global aplicado
     * a velocidades orbitales.
     *
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration",
        meta = (ClampMin = "0.0", ClampMax = "100000.0"))
    float OrbitSpeedMultiplier = 1.0f;

    /**
     * E: Indica si la simulación orbital
     * está activa en editor.
     *
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Configuration")
    bool bIsSimulatingOrbits = false;

    /**
     * E: Número total de cuerpos
     * a generar proceduralmente.
     *
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "1", ClampMax = "200"))
    int32 NumberOfBodies;

    /**
     * E: Tamaño del volumen procedural
     * expresado en kilómetros.
     *
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.1"))
    FVector VolumeSizeKm;

    /**
     * E: Rango permitido de diámetros
     * para cuerpos generados.
     *
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Configuration", meta = (ClampMin = "0.001"))
    FVector2D BodyDiameterRangeKm;

    // =========================================================================
    // GENERATION RULES
    // =========================================================================

    /**
     * E: Distancia mínima permitida
     * entre cuerpos celestes.
     *
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules", meta = (ClampMin = "0.0"))
    float MinDistanceBetweenBodies;

    /**
     * E: Distancia máxima permitida
     * respecto al vecino más cercano.
     *
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules", meta = (ClampMin = "0.0"))
    float MaxDistanceToNearest;

    /**
     * E: Máximo número de intentos
     * para encontrar posiciones válidas.
     *
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation Rules", AdvancedDisplay)
    int32 MaxGenerationAttempts;

    /**
     * E: Referencias a cuerpos
     * actualmente generados.
     *
     */
    UPROPERTY()
    TArray<AActor*> GeneratedBodies;

#if WITH_EDITOR

    /**
     * E: Permite Tick fuera
     * del modo Play.
     *
     */
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }

    /**
     * E: Tick utilizado para
     * simulación orbital en editor.
     *
     */
    virtual void Tick(float DeltaTime) override;

#endif

private:

    /**
     * E: Tipos planetarios soportados
     * por el generador procedural.
     *
     */
    enum class EPlanetType
    {
        GasGiant,
        Telluric,
        AsteroidBelt
    };

    /**
     * E: Estructura utilizada para
     * clasificar planetas proceduralmente.
     *
     */
    struct FPlanetClassification
    {
        EPlanetType Type;

        bool bHasOcean;
        float OceanSeaLevel;

        bool bHasRings;

        bool bHasMoons;
        int32 MaxMoons;
    };

    /**
     * E: Clasifica un planeta según
     * tamaño y distancia orbital.
     *
     */
    FPlanetClassification ClassifyPlanet(
        float OrbitDistanceKm,
        float PlanetRadiusKm,
        float SystemRadiusKm,
        FRandomStream& Stream) const;

    /**
     * E: Genera configuración procedural
     * de ruido planetario.
     *
     */
    UCosmicNoiseClass* CreateRandomNoiseSettings(FRandomStream& Stream, float PlanetRadius);

    /**
     * E: Devuelve un color aleatorio
     * dentro del rango especificado.
     *
     */
    FColor GetRandomColor(FRandomStream& Stream, int min, int max);

public:

    /**
     * E: Actualiza la cantidad de
     * cuerpos generados.
     *
     */
    void SetNumBodies(int32 NumBodies);

    /**
     * E: Genera cuerpos celestes usando
     * la configuración actual.
     *
     */
    UFUNCTION(CallInEditor, Category = "Actions")
    void GenerateBodies();

    /**
     * E: Genera una nueva semilla
     * y reconstruye el sistema.
     *
     */
    UFUNCTION(CallInEditor, Category = "Actions")
    void GenerateWithRandomSeed();

    /**
     * E: Elimina todos los cuerpos
     * actualmente generados.
     *
     */
    UFUNCTION(CallInEditor, Category = "Actions")
    void ClearBodies();

    /**
     * E: Inicia simulación orbital
     * en tiempo de editor.
     *
     */
    UFUNCTION(CallInEditor, Category = "Actions")
    void StartOrbitSimulation();

    /**
     * E: Detiene simulación orbital
     * ejecutada en editor.
     *
     */
    UFUNCTION(CallInEditor, Category = "Actions")
    void StopOrbitSimulation();
};