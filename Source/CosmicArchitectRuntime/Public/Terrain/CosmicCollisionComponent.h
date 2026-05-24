#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "PhysicsEngine/BodySetup.h"
#include "CosmicCollisionComponent.generated.h"

class ICosmicNoiseStrategy;

/**
 * Componente encargado de generar y actualizar la colisión procedural
 * utilizada sobre la superficie planetaria.
 *
 * Implementa un proveedor de datos de colisión dinámico compatible
 * con Chaos/Physics mediante triángulos generados proceduralmente.
 *
 * Funcionalidades principales:
 * - Generación de malla base de colisión.
 * - Actualización dinámica con ruido procedural.
 * - Cooking síncrono y asíncrono.
 * - Visualización debug de la colisión. 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
    HideCategories = (Rendering, Lighting, Navigation, Replication, Physics, LOD, TextureStreaming,
        Activation, AssetUserData, HLOD, Cooking, Tags, ComponentReplication, Mobile, RayTracing))
    class COSMICARCHITECTRUNTIME_API UCosmicCollisionComponent :
    public UPrimitiveComponent,
    public IInterface_CollisionDataProvider
{
    GENERATED_BODY()

public:

    /**
     * Constructor por defecto del componente.
     */
    UCosmicCollisionComponent();

    /** Tamaño de cada triángulo utilizado para la colisión */
    UPROPERTY(EditAnywhere, Category = "Collision")
    float CollisionTriangleSize = 250.f;

    /** Resolución de la cuadrícula de colisión */
    UPROPERTY(EditAnywhere, Category = "Collision")
    int32 CollisionResolution = 12;

    /** Distancia máxima a la que se genera colisión */
    UPROPERTY(EditAnywhere, Category = "Collision")
    double MaxCollisionDistance = 30000.f;

    /** Mostrar la malla de colisión en el editor para depuración */
    UPROPERTY(EditAnywhere, Category = "Collision")
    bool bShowCollisionMesh = false;

    /** Color utilizado para visualizar la malla de colisión */
    UPROPERTY(EditAnywhere, Category = "Collision", meta = (EditCondition = "bShowCollisionMesh"))
    FColor DebugColor = FColor::Green;

    /** Grosor de las líneas debug de la colisión */
    UPROPERTY(EditAnywhere, Category = "Collision", meta = (EditCondition = "bShowCollisionMesh", ClampMin = "0"))
    float DebugLineWidth = 20.f;

    /** Utilizar la colisión compleja como colisión simple */
    UPROPERTY(EditAnywhere, Category = "Collision")
    bool bUseComplexAsSimpleCollision = true;

    /** Utilizar cooking asíncrono para físicas */
    UPROPERTY(EditAnywhere, Category = "Collision")
    bool bUseAsyncCooking = true;

    /**
     * Fuerza una reconstrucción completa de la colisión.
     */
    UFUNCTION(CallInEditor, Category = "Collision")
    void RebuildCollision();

    /**
     * Genera la malla base de colisión.
     *
     * @param Radius Radio del planeta.
     */
    void GenerateCollisionMesh(double Radius);

    /**
     * Actualiza los vértices de colisión utilizando ruido procedural.
     *
     * @param NoiseGenerationStrategy Estrategia de ruido activa.
     * @param PlanetCenter Centro actual del planeta.
     */
    void UpdateCollisionMesh(TSharedPtr<ICosmicNoiseStrategy> NoiseGenerationStrategy, const FVector& PlanetCenter);

    /**
     * Limpia completamente la colisión activa.
     */
    void ClearCollision();

    /**
     * Indica si la colisión ya fue construida.
     *
     * @return True si existe una colisión válida.
     */
    bool IsBuilt() const;

protected:

    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR

    /**
     * Se ejecuta automáticamente al modificar propiedades
     * desde el panel de detalles del editor.
     */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

    /** Implementación del proveedor de datos de colisión */
    virtual bool GetPhysicsTriMeshData(FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;

    /** Indica si existen datos de colisión válidos */
    virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;

    /** No se requiere generación negativa en X */
    virtual bool WantsNegXTriMesh() override { return false; }

    /** Estimaciones del tamaño de la malla de colisión */
    virtual bool GetTriMeshSizeEstimates(FTriMeshCollisionDataEstimates& OutTriMeshEstimates, bool bInUseAllTriData) const override;

    /** Obtiene el BodySetup utilizado por físicas */
    virtual UBodySetup* GetBodySetup() override;

private:

    /**
     * Construye o reconstruye la colisión física.
     */
    void BuildCollision();

    /**
     * Actualiza únicamente los vértices de colisión.
     */
    void UpdateCollisionVertices();

    /** BodySetup principal utilizado por el componente */
    UPROPERTY(Transient)
    UBodySetup* BodySetup = nullptr;

    /** Cola de BodySetup utilizados para cooking asíncrono */
    UPROPERTY()
    TArray<UBodySetup*> AsyncBodySetupQueue;

    /** Centro actual de la colisión */
    FVector CurrentCollisionCenter;

    /** Radio actual de la colisión */
    float CurrentCollisionRadius = 0;

    /** Radio del planeta */
    double PlanetRadius = 0;

    /** Indica si es necesario reconstruir la colisión */
    bool bNeedsRebuild = false;

    /** Indica si la colisión está activa */
    bool bIsActive = false;

    /** Vértices base sin deformación */
    TArray<FVector> BaseVertices;

    /** Normales base utilizadas para deformación */
    TArray<FVector> BaseNormals;

    /** Vértices finales deformados */
    TArray<FVector> Verts;

    /** Índices de triángulos */
    TArray<int32> Tris;

    /**
     * Dibuja la malla debug de colisión.
     */
    void DrawDebugCollisionMesh();

    /**
     * Crea un nuevo BodySetup auxiliar.
     *
     * @return Nuevo BodySetup configurado.
     */
    UBodySetup* CreateBodySetupHelper();

    /**
     * Crea el BodySetup principal procedural.
     */
    void CreateProcMeshBodySetup();

    /**
     * Callback ejecutado al finalizar el cooking asíncrono.
     *
     * @param bSuccess True si el cooking fue exitoso.
     * @param FinishedBodySetup BodySetup finalizado.
     */
    void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup);
};