// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "CosmicSpherePlayer.generated.h"

class UCosmicGravityComponent;
class UCapsuleComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class UCameraComponent;

/**
 * Pawn principal utilizado para navegación planetaria.
 *
 * Este Pawn implementa un sistema de movimiento basado en superficies esféricas:
 * 
 * - Movimiento sobre planetas con gravedad personalizada
 * - Alineación automática con el vector de gravedad
 * - Sistema de cámara desacoplada tipo orbital
 * - Parenting dinámico a cuerpos planetarios
 * - Integración con sistema de gravedad propio
 *
 * La arquitectura está diseñada para evitar problemas de rotación
 * en superficies curvas o planetas dinámicos.
 */
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicSpherePlayer : public APawn
{
	GENERATED_BODY()

public:

	/**
	 * Constructor del jugador planetario.
	 *
	 * Inicializa la jerarquía de componentes y configuración base.
	 */
	ACosmicSpherePlayer();

	/**
	 * Tick principal del Pawn.
	 *
	 * Se encarga de:
	 * - Actualizar alineación con gravedad
	 * - Ajustar orientación visual
	 * - Gestionar lógica runtime del jugador
	 *
	 * @param DeltaTime Tiempo entre frames.
	 */
	virtual void Tick(float DeltaTime) override;

protected:

	/**
	 * Inicialización del Pawn al comenzar el juego.
	 */
	virtual void BeginPlay() override;

	/**
	 * Configuración del sistema Enhanced Input.
	 *
	 * @param PlayerInputComponent Componente de input del jugador.
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ESTADO DEL MOVIMIENTO

	/**
	 * Indica si el jugador está apoyado sobre una superficie válida.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "CosmicArchitect|Movement")
	bool bIsGroundedState;

	/**
	 * Velocidad vertical acumulada sobre el eje de gravedad.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "CosmicArchitect|Movement")
	float VerticalVelocity;

	// COMPONENTES

	/**
	 * Componente de colisión principal del jugador.
	 *
	 * Responsable de:
	 * - Colisiones físicas
	 * - Interacción con el entorno
	 * - Base del movimiento
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	UCapsuleComponent* CapsuleComp;

	/**
	 * Raíz visual alineada con la gravedad local.
	 *
	 * Mantiene el eje Z orientado al vector Up gravitacional.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USceneComponent* VisualRoot;

	/**
	 * Nodo de orientación del mesh del jugador.
	 *
	 * Responsable de ajustar la dirección visual del personaje.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USceneComponent* MeshRoot;

	/**
	 * Malla esquelética del personaje jugador.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USkeletalMeshComponent* PlayerMesh;

	/**
	 * Spring arm para control de cámara.
	 *
	 * Permite suavizado y separación entre cámara y personaje.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USpringArmComponent* SpringArmComp;

	/**
	 * Cámara principal del jugador.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	UCameraComponent* CameraComp;

	/**
	 * Componente de gravedad personalizada.
	 *
	 * Calcula la fuerza gravitacional aplicada al jugador.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	UCosmicGravityComponent* GravityComp;

	// PARÁMETROS DE MOVIMIENTO

	/**
	 * Fuerza base de movimiento en superficie.
	 *
	 * Debe escalarse según la masa del personaje.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Movement")
	float MovementForce = 350000.0f;

	/**
	 * Intensidad de la gravedad aplicada al jugador.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Movement")
	float GravityAcceleration = 9.8f;

	// CONFIGURACIÓN DE INPUT

	/**
	 * Sensibilidad del mouse para cámara.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Input")
	float MouseSensitivity = 1.0f;

	/**
	 * Distancia máxima para activar parenting planetario.
	 *
	 * Permite heredar:
	 * - Movimiento orbital
	 * - Rotación del planeta
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Parenting")
	float ParentingDistanceThreshold = 150.0f;

	/**
	 * Planeta actualmente asignado como referencia de parenting.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Parenting")
	AActor* CurrentParentPlanet;

	// ENHANCED INPUT SYSTEM

	/**
	 * Contexto principal de input (Enhanced Input).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputMappingContext* DefaultMappingContext;

	/**
	 * Acción de movimiento del jugador.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerMove;

	/**
	 * Acción de control de cámara.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerLook;

	/**
	 * Acción de salto del jugador.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerJump;

private:

	// SISTEMA DE MOVIMIENTO

	/**
	 * Aplica movimiento sobre la superficie planetaria.
	 *
	 * @param Value Entrada del jugador.
	 */
	void Move(const FInputActionValue& Value);

	/**
	 * Controla la rotación de cámara y orientación del jugador.
	 *
	 * @param Value Entrada del jugador.
	 */
	void Look(const FInputActionValue& Value);

	/**
	 * Ejecuta salto basado en el eje gravitacional.
	 *
	 * @param Value Entrada del jugador.
	 */
	void Jump(const FInputActionValue& Value);

	/**
	 * Comprueba si el jugador está en contacto con el suelo.
	 *
	 * @return True si está en superficie válida.
	 */
	bool IsGrounded() const;

	// ESTADO DE CÁMARA

	/**
	 * Rotación acumulada en eje Yaw de la cámara.
	 */
	float CameraYaw;

	/**
	 * Rotación acumulada en eje Pitch de la cámara.
	 */
	float CameraPitch;

	/**
	 * Dirección hacia la que se orienta el personaje.
	 */
	FVector TargetFacingDirection;
};