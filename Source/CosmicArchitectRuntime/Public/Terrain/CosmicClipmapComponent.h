// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CosmicClipmapComponent.generated.h"

class ICosmicNoiseStrategy;
class UCosmicMeshComponent;
class UCosmicFoliageSpawner;
class UCosmicCollisionComponent;
class UCosmicNoiseClass;

/**
 * Componente encargado de gestionar el sistema de clipmaps planetarios.
 *
 * Administra la creación, actualización y destrucción de niveles de detalle
 * dinámicos alrededor del jugador, incluyendo:
 * - Generación procedural de mallas.
 * - Transiciones entre modo normal y rendimiento. 
 * - Actualización de colisión cercana.
 * - Generación de foliage.
 * - Materiales dinámicos del planeta.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
    HideCategories = (Activation, Tags, AssetUserData, Navigation, Rendering, Replication, Input, Actor, Collision, Cooking))
    class UCosmicClipmapComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    /**
     * Constructor por defecto del componente.
     */
    UCosmicClipmapComponent();

    /**
     * Crea todos los niveles del sistema clipmap.
     */
    void CreateLevels();

    /**
     * Crea el nivel simplificado utilizado en modo rendimiento.
     *
     * @param bActive Indica si el nivel debe comenzar activo.
     */
    void CreatePerformanceLevel(bool bActive);

    /**
     * Elimina y destruye todos los niveles generados.
     */
    void ClearLevels();

    /**
     * Limpia referencias heredadas tras duplicación sin destruir componentes del actor original.
     *
     * @param NewRoot Componente raíz del nuevo actor.
     */
    void ResetPointersAfterDuplicate(USceneComponent* NewRoot);

    /**
     * Configura los parámetros visuales del material planetario.
     *
     * @param Color1 Color principal base.
     * @param Color2 Color secundario base.
     * @param ColorCold Color para zonas frías.
     * @param ColorHot Color para zonas cálidas.
     * @param ColorSlope Color aplicado a pendientes.
     * @param ScaleL Escala de ruido grande.
     * @param ScaleM Escala de ruido media.
     * @param ScaleS Escala de ruido pequeña.
     */
    void SetMaterialData(FColor Color1, FColor Color2, FColor ColorCold, FColor ColorHot,
        FColor ColorSlope, float ScaleL, float ScaleM, float ScaleS);

    /**
     * Solicita una regeneración completa de las mallas.
     */
    void RequestCompleteMeshUpdate();

    /**
     * Actualiza la estrategia de generación de ruido activa.
     */
    void UpdateNoiseEvaluator();

    /** Root al que se adjuntan los niveles generados */
    USceneComponent* ParentRoot;

    /** Clase encargada de generar la estrategia de ruido procedural */
    UCosmicNoiseClass* NoiseClass;

    /** Radio del planeta */
    double PlanetRadius;

    /** Material base utilizado para generar la instancia dinámica */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UMaterialInstance* BaseMaterial;

    /** Textura por defecto utilizada por el material */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
    UTexture2D* DefaultTexture;

    /** Resolución base de cada nivel del clipmap */
    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "8", ClampMax = "256"))
    int32 BaseResolution = 128;

    /** Número total de niveles del clipmap */
    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "1", ClampMax = "10"))
    int32 NumLevels = 4;

    /** Tamaño mínimo permitido para los triángulos */
    UPROPERTY(EditAnywhere, Category = "Clipmap", meta = (ClampMin = "10"))
    int32 MinTriangleSize = 100;

    /** Espaciado base actual de la cuadrícula */
    UPROPERTY(VisibleAnywhere, Category = "Clipmap")
    int64 BaseGridSpacing = 200;

    /** Altura a partir de la cual se activa el modo rendimiento */
    UPROPERTY(EditAnywhere, Category = "Clipmap")
    float HeightVisibility = 5.0f;

    /** Habilita o deshabilita el sistema clipmap */
    UPROPERTY(EditAnywhere, Category = "Clipmap")
    bool UseClipmap = true;

    /** Congela la generación dinámica de los niveles */
    UPROPERTY(EditAnywhere, Category = "Clipmap")
    bool FreezeGeneration = false;

    /**
     * Paso angular del marco tangente planetario. Dentro del mismo marco los
     * niveles solo desplazan sus caches; al cruzarlo se hace una regeneracion.
     */
    UPROPERTY(EditAnywhere, Category = "Clipmap|Spherical",
        meta = (ClampMin = "0.1", ClampMax = "45.0", UIMin = "0.5", UIMax = "15.0"))
    float PlanetGridSnapAngleDegrees = 5.0f;

    /** Componente encargado de la colisión dinámica */
    UCosmicCollisionComponent* CollisionComponent;

    /** Componente encargado del foliage procedural */
    UCosmicFoliageSpawner* FoliageSpawnerComponent;

protected:

    /** Niveles activos del sistema clipmap */
    TArray<UCosmicMeshComponent*> Levels;

    /** Nivel simplificado utilizado en modo rendimiento */
    UCosmicMeshComponent* FarLevel;

    /** Estrategia activa de generación procedural */
    TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy;

    /**
     * Fases de actualización distribuidas entre frames
     * para reducir el coste por tick.
     */
    enum class EUpdatePhase : uint8
    {
        Foliage,
        Collision,
        Mesh
    };

    /** Tiempo acumulado desde la última actualización */
    float ElapsedTime = 0;

    /** Tiempo de refresco actualmente utilizado */
    float TimeToRefreshActive;

    /** Indica si el sistema está en modo rendimiento */
    bool bPerformaceMode = false;

    /** Indica si los niveles normales han sido inicializados */
    bool bInit = false;

    /** Indica si el nivel de rendimiento ya fue generado */
    bool bPerformanceBuild = false;

    /** Indica si existen tareas pendientes activas */
    bool bPendingTasksRemaining = false;

    /** Esperando transición hacia modo normal */
    bool bWaitingForNormalTransition = false;

    /** Esperando transición hacia modo rendimiento */
    bool bWaitingForPerformanceTransition = false;

    /** Indica si actualmente se están construyendo niveles */
    bool bBuildingLevels = false;

    /** Indica si el sistema representa un planeta esférico */
    bool IsPlanet = true;

    /** Espaciado base original */
    int64 BaseSpacing = 200;

    /** Color principal del planeta */
    FColor PlanetMainColor1 = FColor::Green;

    /** Color secundario del planeta */
    FColor PlanetMainColor2 = FColor::Red;

    /** Color para zonas frías */
    FColor PlanetColdColor = FColor::Yellow;

    /** Color para zonas cálidas */
    FColor PlanetHotColor = FColor::Yellow;

    /** Color utilizado en pendientes */
    FColor PlanetSlopeColor = FColor::Yellow;

    /** Escala de ruido pequeña */
    float NoiseScaleSmall = 1.f;

    /** Escala de ruido media */
    float NoiseScaleMedium = 1.f;

    /** Escala de ruido grande */
    float NoiseScaleLarge = 1.f;

    /** Intervalo de actualización del sistema */
    float TimeToRefresh = 0.01f;

    /** Última posición conocida del jugador */
    FVector LastPlayerPos;

    /** Última posición usada para actualizar colisión */
    FVector LastMeshPlayerPos;

    /** Posición actual del actor propietario */
    FVector CurrentActorPosition;

    /** Delta acumulado en modo plano */
    FVector AccumulatedDelta = FVector::ZeroVector;

    /** Shift total acumulado del clipmap */
    FIntPoint TotalShift = FIntPoint::ZeroValue;

    /** Fase de actualización actual */
    EUpdatePhase CurrentPhase = EUpdatePhase::Mesh;

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR

    /**
     * Se ejecuta automáticamente al modificar propiedades
     * desde el panel de detalles del editor.
     */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

    /**
     * Actualiza el sistema de foliage procedural.
     *
     * @param DeltaTime Tiempo transcurrido desde el último frame.
     * @param ViewerPos Posición actual del jugador.
     * @param DistanceToSurface Distancia a la superficie.
     */
    void UpdateFoliagePhase(float DeltaTime, const FVector& ViewerPos, float DistanceToSurface);

    /**
     * Actualiza la colisión cercana al jugador.
     *
     * @return True si hubo actualización de colisión.
     */
    bool UpdateCollisionPhase(const FVector& ViewerPos, const FVector& SurfacePos,
        const FVector& N, float DistanceToSurface);

    /**
     * Actualiza los niveles del clipmap.
     */
    void UpdateMeshPhase(const FVector& ViewerPos, const FVector& SurfacePos,
        const FVector& N, float DistanceToSurface);

    /** Actualiza el marco tangente cuantizado si el observador cambia de celda angular. */
    bool UpdateSnappedProjectionFrame(const FVector& ViewerNormal);

    /** Proyecta una direccion de la esfera al plano tangente absoluto activo. */
    FVector2D ProjectDirectionToSnappedFrame(const FVector& Direction) const;

    /** Configura todos los niveles con centros enteros alineados entre LODs. */
    bool ConfigureLevelsForViewer(const FVector& ViewerNormal);

    /**
     * Genera o actualiza la colisión cercana al jugador.
     */
    bool UpdateCollisionNearPlayer(const FVector& SurfacePos, const FVector& SurfaceNormal, const double DistanceToSurface);

    /**
     * Construye la instancia dinámica del material planetario.
     */
    void BuildDynamicMaterial();

    /**
     * Calcula la rotación correcta de un patch
     * respecto a la normal de superficie.
     */
    FRotator GetPatchRotation(const FVector& SurfacePos) const;

    /**
     * Calcula la distancia real a la superficie utilizando ruido.
     */
    double GetDistanceToSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);

    /**
     * Calcula la distancia aproximada a la superficie sin ruido.
     */
    double GetFastDistanceToSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);

    /**
     * Calcula la distancia a una superficie plana.
     */
    float GetDistanceToPlainSurface(FVector& ViewerPos, FVector& SurfacePos, FVector& N);

    /**
     * Obtiene la posición actual del jugador o cámara.
     */
    FVector GetPlayerLocation();

    /**
     * Calcula el desplazamiento del grid en modo plano.
     */
    FIntPoint ComputeGridShiftPlanar(const FVector& PlayerPos, float GridSpacing);

    /**
     * Calcula el desplazamiento del grid sobre superficie esférica.
     */
    FIntPoint ComputeGridShiftSpherical(const FVector& PlayerPos, const FVector& CurrentSurfacePos, int64 GridSpacing);

    /**
     * Calcula el desplazamiento del grid según el tipo de superficie.
     */
    FIntPoint ComputeGridShift(const FVector& PlayerPos, const FVector& CurrentSurfacePos, float GridSpacing);

    /**
     * Obtiene los ángulos esféricos de una posición de superficie.
     */
    FVector2D GetSurfaceAngles(const FVector& SurfacePos);

    /**
     * Calcula cuántos niveles deben reducirse.
     */
    int32 CalculateDecreaseSteps(const double DistanceToSurface) const;

    /**
     * Calcula cuántos niveles deben incrementarse.
     */
    int32 CalculateIncreaseSteps(const double DistanceToSurface) const;

    /**
     * Comprueba si un anillo del clipmap es visible.
     */
    bool IsClipmapRingVisible(const int32 LevelIndex, const double DistanceToSurface) const;

    /**
     * Comprueba si un anillo del clipmap es visible utilizando spacing manual.
     */
    bool IsClipmapRingVisible(const int64 GridSpacing, const int64 Resolution, const double DistanceToSurface) const;

    /**
     * Reduce el detalle global del clipmap.
     */
    void DecreaseClipmapLevelFull(int32 Steps = 1);

    /**
     * Incrementa el detalle global del clipmap.
     */
    void IncreaseClipmapLevelFull(int32 Steps = 1);


private:

    /** Material dinámico utilizado por el planeta */
    UPROPERTY(Transient, DuplicateTransient)
    UMaterialInstanceDynamic* DynamicPlanetMat;

    /** Última posición conocida sobre la superficie */
    FVector PreviousSurfacePos = FVector::ZeroVector;

    /** Últimos ángulos esféricos registrados del jugador */
    FVector2D LastSurfaceAngles;

    /** Delta lineal acumulado sobre la superficie */
    FVector2D AccumulatedLinearDelta;

    /** Marco tangente fijo y celda angular que lo origino. */
    FTransform SnappedProjectionFrame = FTransform::Identity;
    FIntPoint SnappedProjectionKey = FIntPoint::ZeroValue;
    bool bSnappedProjectionValid = false;
    uint64 SnappedProjectionRevision = 0;

    /** Centro comun expresado en celdas del nivel mas grueso. */
    FIntPoint CoarsestGridCenter = FIntPoint::ZeroValue;
    bool bCoarsestGridCenterValid = false;
};
