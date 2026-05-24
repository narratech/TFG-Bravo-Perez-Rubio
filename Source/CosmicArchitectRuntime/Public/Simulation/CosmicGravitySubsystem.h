// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CosmicGravitySubsystem.generated.h"

class UCosmicGravityComponent;

/**
 * Subsistema de mundo responsable de gestionar la simulacion gravitacional completa del nivel.
 *
 * Actua como coordinador central de todos los cuerpos registrados, calculando y distribuyendo
 * fuerzas gravitacionales cada frame segun el modo configurado en cada UCosmicGravityComponent.
 *
 * Responsabilidades principales:
 *   - Mantener dos listas internas: Bodies (todos los cuerpos) y Planets (solo planetas).
 *   - Calcular fuerzas gravitacionales en Tick() y acumularlas en cada componente.
 *   - Invocar Integrate() en cada cuerpo activo tras acumular todas las fuerzas del frame.
 *   - Proveer RegisterBody() y UnregisterBody() como punto de entrada para los componentes. 
 *
 * Restricciones y contratos de uso:
 *   - Solo se activa el Tick cuando hay cuerpos registrados (optimizacion automatica).
 *   - No debe modificarse Bodies ni Planets directamente desde fuera del subsistema.
 *   - Los componentes deben registrarse en BeginPlay y desregistrarse en EndPlay.
 */
UCLASS()
class COSMICARCHITECTRUNTIME_API UCosmicGravitySubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    /**
     * Inicializa el subsistema al crearse o cargarse el nivel.
     * Punto de entrada para inicializaciones futuras que dependan del World.
     */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /**
     * Vacia las listas internas y libera referencias al destruirse el nivel.
     * Garantiza que no queden referencias colgantes a componentes destruidos.
     */
    virtual void Deinitialize() override;

    /**
     * Nucleo de la simulacion. Se ejecuta una vez por frame mientras haya cuerpos registrados.
     *
     * Fase 1: Acumulacion de fuerzas.
     *   Recorre todos los cuerpos y calcula las fuerzas gravitacionales segun su GravityMode,
     *   acumulandolas en AccumulatedForce de cada componente afectado.
     *
     * Fase 2: Integracion.
     *   Invoca Integrate(DeltaTime) en cada cuerpo activo para aplicar el movimiento resultante.
     *
     * @param DeltaTime Tiempo en segundos transcurrido desde el ultimo frame.
     */
    virtual void Tick(float DeltaTime) override;

    /**
     * Indica si el subsistema debe ejecutar Tick este frame.
     * Retorna false si no hay cuerpos registrados o si el World no es valido,
     * evitando coste de CPU cuando la simulacion esta inactiva.
     */
    virtual bool IsTickable() const override;

    /**
     * Identificador de estadisticas requerido por FTickableGameObject.
     * Permite al profiler de Unreal medir el coste de este subsistema por separado.
     */
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UGravitySubsystem, STATGROUP_Tickables);
    }

    /**
     * Devuelve la lista de planetas registrados para lectura externa.
     * Permite a otros sistemas consultar los planetas activos sin acceder a Bodies completo.
     */
    const TArray<UCosmicGravityComponent*>& GetPlanets() const { return Planets; }

    /**
     * Registra un componente en la simulacion gravitacional.
     * Si el componente es un planeta, tambien se anade a la sublista Planets.
     * Usa AddUnique para evitar duplicados en caso de llamadas multiples.
     *
     * @param Body Componente a registrar. Se ignora si es nullptr.
     */
    void RegisterBody(UCosmicGravityComponent* Body);

    /**
     * Elimina un componente de la simulacion gravitacional.
     * Si el componente era un planeta, tambien se elimina de la sublista Planets.
     *
     * @param Body Componente a desregistrar. Se ignora si es nullptr.
     */
    void UnregisterBody(UCosmicGravityComponent* Body);

    /**
     * Devuelve la constante gravitacional G en unidades SI (m3 / kg * s2).
     * Usada por UCosmicGravityComponent para calcular la masa de los planetas en BeginPlay.
     */
    double GetGravityConstant() const;

    /**
     * Devuelve el World en el que opera este objeto tickeable.
     * Requerido por la interfaz FTickableGameObject para validar el contexto de ejecucion.
     */
    virtual UWorld* GetTickableGameObjectWorld() const override;

private:
    /**
     * Calcula la fuerza gravitacional que BodyB ejerce sobre BodyA y la acumula en BodyA.
     * Operacion unidireccional: solo BodyA recibe la fuerza resultante.
     * Usada por los modos NearestPlanet, SpecificPlanet y AllPlanets.
     *
     * @param BodyA Cuerpo que recibe la fuerza gravitacional.
     * @param BodyB Cuerpo que actua como fuente de gravedad.
     */
    void BodyAddForce(UCosmicGravityComponent* BodyA, UCosmicGravityComponent* BodyB);

    /**
     * Calcula y aplica fuerzas gravitacionales mutuas entre dos cuerpos (Tercera Ley de Newton).
     * BodyA recibe fuerza hacia BodyB y BodyB recibe fuerza hacia BodyA en el mismo calculo.
     * Usada exclusivamente por el modo NBody para evitar calculos redundantes (j > i).
     *
     * @param BodyA Primer cuerpo del par.
     * @param BodyB Segundo cuerpo del par.
     */
    void ApplyMutualForce(UCosmicGravityComponent* BodyA, UCosmicGravityComponent* BodyB);

    /**
     * Lista de todos los cuerpos activos registrados en la simulacion del nivel actual.
     * Incluye tanto planetas como cuerpos orbitales. Se marca con UPROPERTY para
     * que el recolector de basura de Unreal no invalide las referencias.
     */
    UPROPERTY()
    TArray<UCosmicGravityComponent*> Bodies;

    /**
     * Sublista optimizada que contiene unicamente los cuerpos con IsPlanet == true.
     * Permite a los modos NearestPlanet, SpecificPlanet y AllPlanets iterar solo sobre
     * planetas sin filtrar Bodies completo en cada frame.
     */
    TArray<UCosmicGravityComponent*> Planets;
};