// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "CosmicGravityComponent.generated.h"

/**
 * Define los modos de simulación gravitacional disponibles para el componente.
 *
 * Cada modo determina qué cuerpos celestes influyen sobre el actor propietario
 * y cómo se calculan las fuerzas resultantes en cada frame de simulación.
 *
 * Modos disponibles:
 *   - None          → Sin ningún efecto gravitacional activo.
 *   - NearestPlanet → Atracción exclusiva hacia el planeta más cercano en escena.
 *   - SpecificPlanet→ Atracción hacia un actor planeta definido manualmente.
 *   - AllPlanets    → Suma de fuerzas gravitacionales de todos los planetas en escena.
 *   - NBody         → Simulación N-cuerpos: todos los objetos registrados se atraen mutuamente.
 */
UENUM(BlueprintType)
enum class ECosmicGravityMode : uint8
{
    None           UMETA(DisplayName = "None"),
    NearestPlanet  UMETA(DisplayName = "Nearest Planet"),
    SpecificPlanet UMETA(DisplayName = "Specific Planet"),
    AllPlanets     UMETA(DisplayName = "All Planets"),
    NBody          UMETA(DisplayName = "N-Body")
};

/**
 * Gestiona la física gravitacional personalizada de un actor dentro del sistema Cosmic Architect.
 *
 * Este componente puede actuar en dos roles excluyentes según el valor de IsPlanet:
 *
 *   ROL PLANETA:
 *     Genera un campo gravitacional basado en su masa, calculada a partir de
 *     RadiusKm y SurfaceGravity. No aplica físicas de motor sobre sí mismo.
 *
 *   ROL CUERPO ORBITAL:
 *     Reacciona a los campos gravitacionales de los planetas registrados.
 *     Activa la simulación de físicas del motor y desactiva la gravedad interna de Unreal.
 *
 * Responsabilidades principales:
 *   - Registrarse y desregistrarse en UCosmicGravitySubsystem al iniciar y terminar.
 *   - Acumular fuerzas gravitacionales por frame mediante AccumulatedForce.
 *   - Integrar velocidad y posición (o aplicar AddForce si usa físicas del motor).
 *   - Exponer configuración editable desde el editor para diseño de niveles.
 *
 * Restricciones y contratos de uso:
 *   - Requiere que UCosmicGravitySubsystem esté activo en el World.
 *   - El actor propietario debe tener un UPrimitiveComponent como componente raíz.
 *   - No debe usarse junto con la gravedad interna de Unreal Engine (SetEnableGravity = false).
 *   - SetIsPlanet() debe usarse en runtime para cambiar de rol de forma segura.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent),
    HideCategories = (Activation, AssetUserData, Cooking, Tags, Navigation))
    class COSMICARCHITECTRUNTIME_API UCosmicGravityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCosmicGravityComponent();

    /**
     * Devuelve la transformación espacial completa del actor propietario.
     * Utilizada por el subsistema para calcular distancias y direcciones gravitacionales.
     */
    FTransform getTransform() const { return GetOwner()->GetActorTransform(); }

    /**
     * Modo de gravedad activo para este componente.
     * Determina qué planetas se tienen en cuenta al calcular las fuerzas sobre este cuerpo.
     * Siempre visible en el editor independientemente del rol del objeto.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config")
    ECosmicGravityMode GravityMode = ECosmicGravityMode::NearestPlanet;

    /**
     * Masa del cuerpo orbital en kilogramos.
     * Se usa para calcular la aceleración resultante (F = m·a) en cada integración.
     * Solo visible en el editor cuando IsPlanet es false.
     * En planetas, la masa se calcula automáticamente desde RadiusKm y SurfaceGravity.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "!IsPlanet", EditConditionHides))
    double Mass = 100.0f;

    /**
     * Radio físico del planeta expresado en kilómetros.
     * Se combina con SurfaceGravity para calcular la masa gravitacional del planeta.
     * Solo visible en el editor cuando IsPlanet es true.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "IsPlanet", EditConditionHides, ClampMin = "0.001",
            UIMin = "0.001", UIMax = "1000000",
            ToolTip = "Radio del planeta en kilómetros"))
    float RadiusKm = 1.0f;

    /**
     * Aceleración gravitacional en la superficie del planeta (m/s²).
     * Junto con RadiusKm, define la masa del planeta al iniciarse el juego.
     * Solo visible en el editor cuando IsPlanet es true.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "IsPlanet", EditConditionHides, ClampMin = "0.0"))
    float SurfaceGravity = 9.8f;

    /**
     * Indica si este objeto genera un campo gravitacional sobre otros cuerpos registrados.
     * Si es false, el subsistema ignorará este objeto como fuente de gravedad.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config")
    bool AffectsOthers = true;

    /**
     * Indica si este objeto reacciona a los campos gravitacionales generados por otros.
     * Si es false, el subsistema no aplicará fuerzas externas sobre este objeto.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config")
    bool IsAffectedByOthers = true;

    /**
     * Define si este actor actúa como cuerpo planetario dentro de la simulación.
     *
     * true  → Actúa como planeta: genera gravedad, no recibe físicas del motor.
     * false → Actúa como cuerpo orbital: recibe físicas y reacciona a campos gravitacionales.
     *
     * Para cambiar este valor en runtime, usar SetIsPlanet() en lugar de modificarlo directamente,
     * ya que es necesario actualizar el registro en el subsistema.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity Config")
    bool IsPlanet = false;

    /**
     * Actor que actúa como fuente de gravedad exclusiva cuando el modo es SpecificPlanet.
     * El actor referenciado debe tener también un UCosmicGravityComponent activo.
     * Solo visible en el editor cuando GravityMode == SpecificPlanet.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Config",
        meta = (EditCondition = "GravityMode == ECosmicGravityMode::SpecificPlanet", EditConditionHides))
    AActor* SpecificGravitySource = nullptr;

    /**
     * Referencia al componente primitivo raíz del actor propietario.
     * Se cachea en BeginPlay para aplicar físicas (AddForce, SetMass, etc.) sin búsquedas repetidas.
     */
    UPROPERTY()
    UPrimitiveComponent* RootPrimitive = nullptr;

    /** Velocidad actual del cuerpo en el espacio de simulación (cm/s en Unreal). */
    FVector Velocity = FVector::ZeroVector;

    /** Acumulador de fuerzas gravitacionales recibidas durante el frame actual (N). Se resetea tras cada integración. */
    FVector AccumulatedForce = FVector::ZeroVector;

    /**
     * Dirección normalizada de la gravedad dominante del frame anterior.
     * Puede ser leída por otros sistemas (orientación del personaje, efectos visuales, etc.)
     * para conocer hacia dónde "apunta" la gravedad desde este objeto.
     */
    FVector CurrentGravityDirection = FVector::DownVector;

    /**
     * Aplica las fuerzas acumuladas e integra la posición o velocidad del cuerpo.
     *
     * Si el RootPrimitive está simulando físicas (cuerpos orbitales estándar),
     * delega la aplicación en AddForce del motor. En caso contrario, integra
     * manualmente velocidad y posición usando integración de Euler.
     *
     * Debe llamarse una vez por frame desde el subsistema, después de acumular
     * todas las fuerzas gravitacionales del frame.
     *
     * @param DeltaTime Tiempo transcurrido desde el último frame, en segundos.
     */
    void Integrate(double DeltaTime);

    /**
     * Cambia el rol del objeto entre planeta y cuerpo orbital de forma segura en runtime.
     *
     * Gestiona automáticamente el desregistro y re-registro en el subsistema
     * para que las listas internas de planetas y cuerpos se mantengan coherentes.
     * No realiza ninguna operación si el valor nuevo es igual al actual.
     *
     * @param bNewIsPlanet true para convertir en planeta, false para cuerpo orbital.
     */
    void SetIsPlanet(bool bNewIsPlanet);

protected:
    /**
     * Inicializa el componente al comenzar el juego.
     *
     * Responsabilidades:
     *   - Cachear RootPrimitive y configurar su movilidad y físicas según el rol.
     *   - Calcular la masa del planeta si IsPlanet es true.
     *   - Registrar este componente en UCosmicGravitySubsystem.
     */
    virtual void BeginPlay() override;

    /**
     * Limpia el componente al destruirse el actor o finalizar el juego.
     * Desregistra este componente del subsistema para evitar referencias colgantes.
     *
     * @param EndPlayReason Motivo por el que se invoca EndPlay.
     */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /**
     * Calcula el radio aproximado del actor en unidades Unreal (cm) a partir de su bounding box.
     * Devuelve la mayor de las tres semi-extensiones del box, asumiendo geometría roughly esférica.
     * Usado para cálculos de colisión o detección de superficie en planetas no perfectamente esféricos.
     *
     * @return Radio aproximado en centímetros (unidades Unreal).
     */
    float GetObjectRadius() const;
};