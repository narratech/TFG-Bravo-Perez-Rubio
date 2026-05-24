// ============================================================
// Cosmic Architect
// Archivo: CosmicSpaceShip.h
// Módulo: CosmicArchitectRuntime
//
// Descripción:
//     Pawn espacial diseñado para navegación 6DOF.
//
// Responsabilidades:
//     - Movimiento espacial libre
//     - Rotación física de la nave
//     - Gestión de boost interestelar
//     - Integración con Enhanced Input
//     - Control de cámara del jugador
//
// Notas:
//     - Utiliza simulación física basada en fuerzas.
//     - El movimiento se implementa mediante thrust
//       y torque aplicados sobre el mesh principal.
//     - Diseñado para navegación espacial sin gravedad.
// ============================================================
 
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "CosmicSpaceShip.generated.h"

/**
 * Pawn principal utilizado para navegación espacial 6DOF.
 *
 * Este Pawn implementa un sistema de control espacial basado en físicas:
 *
 * - Movimiento mediante fuerzas (thrusters)
 * - Rotación mediante torque aplicado al cuerpo físico
 * - Sistema de boost con modificación temporal de parámetros físicos
 * - Cámara desacoplada mediante spring arm
 * - Integración completa con Enhanced Input
 *
 * Está diseñado específicamente para entornos de simulación espacial
 * sin gravedad o con gravedad mínima.
 */
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicSpaceShip : public APawn
{
	GENERATED_BODY()

public:

	// ============================================================
	// CONSTRUCTORA
	// ============================================================

	/**
	 * Constructor de la nave espacial.
	 *
	 * Inicializa componentes, configuración física y estado base del Pawn.
	 */
	ACosmicSpaceShip();

protected:

	/**
	 * Inicialización del juego.
	 *
	 * Se encarga de configurar el estado inicial del Pawn
	 * una vez comienza la simulación.
	 */
	virtual void BeginPlay() override;

	// ============================================================
	// SISTEMA DE INPUT
	// ============================================================

	/**
	 * Configura el sistema de input del jugador usando Enhanced Input.
	 *
	 * Responsabilidades:
	 * - Movimiento 6DOF
	 * - Control de orientación
	 * - Alabeo (roll)
	 * - Activación y gestión del boost
	 *
	 * @param PlayerInputComponent Componente de input del jugador.
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ============================================================
	// COMPONENTES
	// ============================================================

	/**
	 * Mesh principal de la nave.
	 *
	 * Responsable de:
	 * - Simulación física
	 * - Colisiones
	 * - Aplicación de fuerzas y torque
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UStaticMeshComponent* ShipMesh;

	/**
	 * Spring Arm para la cámara.
	 *
	 * Permite suavizar el movimiento de cámara
	 * y desacoplarla parcialmente de la nave.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class USpringArmComponent* SpringArmComp;

	/**
	 * Cámara principal del jugador.
	 *
	 * Representa la vista principal en primera/tercera persona espacial.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UCameraComponent* CameraComp;

	// ============================================================
	// VARIABLES DE MOVIMIENTO
	// ============================================================

	/**
	 * Fuerza base aplicada por los propulsores.
	 *
	 * A mayor valor, mayor aceleración lineal de la nave.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float ThrusterForce = 500000.0f;

	/**
	 * Torque base para rotación (pitch/yaw).
	 *
	 * Controla la sensibilidad de giro de la nave.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float RotationTorque = 300000.0f;

	/**
	 * Torque aplicado para el alabeo (roll).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float AlabeoTorque = 300000.0f;

	/**
	 * Multiplicador de potencia durante el boost.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float BoostIncreasePower = 50.0f;

	// ============================================================
	// CONFIGURACIÓN DE INPUT
	// ============================================================

	/**
	 * Contexto de input por defecto (Enhanced Input).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputMappingContext* DefaultMappingContext;

	/**
	 * Acción de input para traslación 3D.
	 *
	 * Ejes:
	 * - Forward / Backward
	 * - Right / Left
	 * - Up / Down
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Traslacion;

	/**
	 * Acción de input para orientación de la nave.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Orientacion;

	/**
	 * Acción de input para alabeo (roll).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Alabeo;

	/**
	 * Acción de input para activar boost.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Boost;

private:

	// ============================================================
	// SISTEMA DE MOVIMIENTO
	// ============================================================

	/**
	 * Aplica traslación mediante fuerzas físicas.
	 *
	 * @param Value Vector de entrada del jugador.
	 */
	void AplicarTraslacion(const FInputActionValue& Value);

	/**
	 * Aplica torque para orientación de la nave.
	 *
	 * @param Value Entrada de rotación del jugador.
	 */
	void AplicarOrientacion(const FInputActionValue& Value);

	/**
	 * Aplica torque de alabeo (roll).
	 *
	 * @param Value Entrada de roll del jugador.
	 */
	void AplicarAlabeo(const FInputActionValue& Value);

	/**
	 * Activa el sistema de boost.
	 */
	void StartBoost(const FInputActionValue& Value);

	/**
	 * Desactiva el boost y restaura parámetros físicos.
	 */
	void EndBoost(const FInputActionValue& Value);

	// ============================================================
	// SISTEMA DE BOOST
	// ============================================================

	/**
	 * Velocidad de frenado al salir del boost.
	 */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float BrakingSpeed;

	/**
	 * Damping lineal original para restauración tras boost.
	 */
	float OriginalLinearDamping;

	/**
	 * Gestiona el estado interno del sistema de boost.
	 */
	void SetBoost(const FInputActionValue& Value);

	/**
	 * Indica si el boost está activo.
	 */
	bool bBoostMode = false;
};