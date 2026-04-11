// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CosmicGravitySubsystem.generated.h"

// E: Declaración anticipada de la clase UGravityComponent.
// I: Forward declaration of the UGravityComponent class.
class UCosmicGravityComponent;

// E: Subsistema del mundo encargado de gestionar, calcular y aplicar la simulación de gravedad a todos los cuerpos.
// I: World subsystem in charge of managing, calculating, and applying the gravity simulation to all bodies.
UCLASS()
class COSMICARCHITECTRUNTIME_API UCosmicGravitySubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    // E: Inicializa el subsistema cuando el mundo (nivel) se crea o carga.
    // I: Initializes the subsystem when the world (level) is created or loaded.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // E: Limpia y reinicia las listas del subsistema cuando el mundo se destruye.
    // I: Cleans up and resets the subsystem's lists when the world is destroyed.
    virtual void Deinitialize() override;

    // E: Función principal de actualización llamada cada frame para calcular las fuerzas físicas.
    // I: Main update function called every frame to calculate physics forces.
    virtual void Tick(float DeltaTime) override;

    // E: Determina si este subsistema debe ejecutar su función Tick (útil para optimización).
    // I: Determines if this subsystem should execute its Tick function (useful for optimization).
    virtual bool IsTickable() const override;

    // E: Obtiene el ID de estadísticas utilizado por el profiler de rendimiento de Unreal.
    // I: Gets the stat ID used by the Unreal performance profiler.
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UGravitySubsystem, STATGROUP_Tickables);
    }

    // E: Devuelve la lista optimizada de planetas registrados para lectura.
    // I: Returns the optimized list of registered planets for reading.
    const TArray<UCosmicGravityComponent*>& GetPlanets() const { return Planets; }

    // E: Registra un nuevo cuerpo en la simulación (lo añade a las listas internas).
    // I: Registers a new body into the simulation (adds it to the internal lists).
    void RegisterBody(UCosmicGravityComponent* Body);

    // E: Elimina un cuerpo de la simulación para que deje de ser afectado por la gravedad.
    // I: Removes a body from the simulation so it is no longer affected by gravity.
    void UnregisterBody(UCosmicGravityComponent* Body);

    // E: Devuelve el valor de la constante gravitacional base de la simulación.
    // I: Returns the value of the base gravitational constant of the simulation.
    double GetGravityConstant() const;

    // E: Devuelve el contexto del mundo en el que este objeto tickeable está operando.
    // I: Returns the world context in which this tickable object is operating.
    virtual UWorld* GetTickableGameObjectWorld() const override;

private:
    // E: Calcula y aplica la fuerza gravitacional del cuerpo B sobre el cuerpo A (unidireccional).
    // I: Calculates and applies the gravitational force of body B onto body A (unidirectional).
    void BodyAddForce(UCosmicGravityComponent* BodyA, UCosmicGravityComponent* BodyB);

    // E: Calcula y aplica las fuerzas gravitacionales entre dos cuerpos mutuamente (N-Cuerpos).
    // I: Calculates and applies gravitational forces between two bodies mutually (N-Body).
    void ApplyMutualForce(UCosmicGravityComponent* BodyA, UCosmicGravityComponent* BodyB);

    // E: Lista general de todos los cuerpos con gravedad registrados en este nivel.
    // I: General list of all gravity bodies registered in this level.
    UPROPERTY()
    TArray<UCosmicGravityComponent*> Bodies;

    // E: Sublista optimizada que contiene exclusivamente los cuerpos clasificados como planetas.
    // I: Optimized sublist containing exclusively the bodies classified as planets.
    TArray<UCosmicGravityComponent*> Planets;
};