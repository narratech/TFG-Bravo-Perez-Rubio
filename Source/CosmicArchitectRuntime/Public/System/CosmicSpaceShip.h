// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "CosmicSpaceShip.generated.h"

/**
 * Main Pawn used for 6DOF space navigation.
 *
 * This Pawn implements a physics-based space control system:
 *
 * - Movement using forces (thrusters)
 * - Rotation using torque applied to the physical body
 * - Boost system with temporary modification of physical parameters
 * - Decoupled camera using a spring arm
 * - Full integration with Enhanced Input
 *
 * It is specifically designed for space simulation environments
 * with zero or minimal gravity.
 */
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicSpaceShip : public APawn
{
	GENERATED_BODY()

public:

	// ============================================================
	// CONSTRUCTOR
	// ============================================================

	/**
	 * Space ship constructor.
	 *
	 * Initializes components, physics configuration, and Pawn base state.
	 */
	ACosmicSpaceShip();

protected:

	/**
	 * Game initialization.
	 *
	 * Sets up the initial state of the Pawn
	 * once simulation begins.
	 */
	virtual void BeginPlay() override;

	// ============================================================
	// INPUT SYSTEM
	// ============================================================

	/**
	 * Sets up player input using Enhanced Input.
	 *
	 * Responsibilities:
	 * - 6DOF movement
	 * - Orientation control
	 * - Roll
	 * - Boost activation and management
	 *
	 * @param PlayerInputComponent Player input component.
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ============================================================
	// COMPONENTS
	// ============================================================

	/**
	 * Main mesh of the ship.
	 *
	 * Responsible for:
	 * - Physics simulation
	 * - Collisions
	 * - Force and torque application
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UStaticMeshComponent* ShipMesh;

	/**
	 * Spring Arm for the camera.
	 *
	 * Allows smoothing camera movement
	 * and partially decoupling it from the ship.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class USpringArmComponent* SpringArmComp;

	/**
	 * Main player camera.
	 *
	 * Represents the main first/third person space view.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UCameraComponent* CameraComp;

	// ============================================================
	// MOVEMENT VARIABLES
	// ============================================================

	/**
	 * Base force applied by the thrusters.
	 *
	 * Higher values result in greater linear acceleration of the ship.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float ThrusterForce = 500000.0f;

	/**
	 * Base torque for rotation (pitch/yaw).
	 *
	 * Controls the turn sensitivity of the ship.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float RotationTorque = 300000.0f;

	/**
	 * Torque applied for roll.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float AlabeoTorque = 300000.0f;

	/**
	 * Power multiplier during boost.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float BoostIncreasePower = 50.0f;

	// ============================================================
	// INPUT CONFIGURATION
	// ============================================================

	/**
	 * Default input context (Enhanced Input).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputMappingContext* DefaultMappingContext;

	/**
	 * Input action for 3D translation.
	 *
	 * Axes:
	 * - Forward / Backward
	 * - Right / Left
	 * - Up / Down
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Traslacion;

	/**
	 * Input action for ship orientation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Orientacion;

	/**
	 * Input action for roll.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Alabeo;

	/**
	 * Input action to activate boost.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Boost;

private:

	// ============================================================
	// MOVEMENT SYSTEM
	// ============================================================

	/**
	 * Applies translation using physics forces.
	 *
	 * @param Value Player input vector.
	 */
	void AplicarTraslacion(const FInputActionValue& Value);

	/**
	 * Applies torque for ship orientation.
	 */
	void AplicarOrientacion(const FInputActionValue& Value);

	/**
	 * Applies roll torque.
	 *
	 * @param Value Player roll input.
	 */
	void AplicarAlabeo(const FInputActionValue& Value);

	/**
	 * Activates the boost system.
	 */
	void StartBoost(const FInputActionValue& Value);

	/**
	 * Deactivates boost and restores physical parameters.
	 */
	void EndBoost(const FInputActionValue& Value);

	// ============================================================
	// BOOST SYSTEM
	// ============================================================

	/**
	 * Braking speed when exiting boost.
	 */
	UPROPERTY(EditAnywhere, Category = "Movement")
	float BrakingSpeed;

	/**
	 * Original linear damping for restoration after boost.
	 */
	float OriginalLinearDamping;

	/**
	 * Manages the internal state of the boost system.
	 */
	void SetBoost(const FInputActionValue& Value);

	/**
	 * Indicates whether boost is active.
	 */
	bool bBoostMode = false;
};