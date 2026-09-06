// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

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
 * Main Pawn used for planetary navigation.
 *
 * This Pawn implements a spherical surface movement system:
 * 
 * - Movement on planets with custom gravity
 * - Automatic alignment with the gravity vector
 * - Orbital-type decoupled camera system
 * - Dynamic parenting to planetary bodies
 * - Integration with custom gravity system
 *
 * The architecture is designed to avoid rotation issues
 * on curved surfaces or dynamic planets.
 */
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicSpherePlayer : public APawn
{
	GENERATED_BODY()

public:

	/**
	 * Planetary player constructor.
	 *
	 * Initializes component hierarchy and base configuration.
	 */
	ACosmicSpherePlayer();

	/**
	 * Main Pawn tick.
	 *
	 * Responsible for:
	 * - Updating alignment with gravity
	 * - Adjusting visual orientation
	 * - Managing player runtime logic
	 *
	 * @param DeltaTime Time between frames.
	 */
	virtual void Tick(float DeltaTime) override;

protected:

	/**
	 * Pawn initialization when game starts.
	 */
	virtual void BeginPlay() override;

	/**
	 * Enhanced Input system configuration.
	 *
	 * @param PlayerInputComponent Player input component.
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// MOVEMENT STATE

	/**
	 * Indicates whether the player is grounded on a valid surface.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "CosmicArchitect|Movement")
	bool bIsGroundedState;

	/**
	 * Accumulated vertical velocity along the gravity axis.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "CosmicArchitect|Movement")
	float VerticalVelocity;

	// COMPONENTS

	/**
	 * Main player collision component.
	 *
	 * Responsible for:
	 * - Physics collisions
	 * - Interaction with environment
	 * - Movement base
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	UCapsuleComponent* CapsuleComp;

	/**
	 * Visual root aligned with local gravity.
	 *
	 * Keeps the Z axis oriented to the gravitational Up vector.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USceneComponent* VisualRoot;

	/**
	 * Player mesh orientation node.
	 *
	 * Responsible for adjusting character visual direction.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USceneComponent* MeshRoot;

	/**
	 * Skeletal mesh of the player character.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USkeletalMeshComponent* PlayerMesh;

	/**
	 * Spring arm for camera control.
	 *
	 * Allows smoothing and separation between camera and character.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USpringArmComponent* SpringArmComp;

	/**
	 * Main player camera.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	UCameraComponent* CameraComp;

	/**
	 * Custom gravity component.
	 *
	 * Calculates gravitational force applied to player.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	UCosmicGravityComponent* GravityComp;

	// MOVEMENT PARAMETERS

	/**
	 * Base movement force on surface.
	 *
	 * Must be scaled according to character mass.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Movement")
	float MovementForce = 350000.0f;

	/**
	 * Gravity intensity applied to player.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Movement")
	float GravityAcceleration = 9.8f;

	// INPUT CONFIGURATION

	/**
	 * Mouse sensitivity for camera.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Input")
	float MouseSensitivity = 1.0f;

	/**
	 * Maximum distance to activate planetary parenting.
	 *
	 * Allows inheriting:
	 * - Orbital movement
	 * - Planet rotation
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Parenting")
	float ParentingDistanceThreshold = 150.0f;

	/**
	 * Planet currently assigned as parenting reference.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Parenting")
	AActor* CurrentParentPlanet;

	// ENHANCED INPUT SYSTEM

	/**
	 * Main input context (Enhanced Input).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputMappingContext* DefaultMappingContext;

	/**
	 * Player movement action.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerMove;

	/**
	 * Camera control action.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerLook;

	/**
	 * Player jump action.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerJump;

private:

	// MOVEMENT SYSTEM

	/**
	 * Applies movement on the planetary surface.
	 *
	 * @param Value Player input.
	 */
	void Move(const FInputActionValue& Value);

	/**
	 * Controls camera rotation and player orientation.
	 *
	 * @param Value Player input.
	 */
	void Look(const FInputActionValue& Value);

	/**
	 * Executes jump based on gravitational axis.
	 *
	 * @param Value Player input.
	 */
	void Jump(const FInputActionValue& Value);

	/**
	 * Checks whether the player is in contact with the ground.
	 *
	 * @return True if on a valid surface.
	 */
	bool IsGrounded() const;

	// CAMERA STATE

	/**
	 * Accumulated rotation on camera Yaw axis.
	 */
	float CameraYaw;

	/**
	 * Accumulated rotation on camera Pitch axis.
	 */
	float CameraPitch;

	/**
	 * Direction in which the character is oriented.
	 */
	FVector TargetFacingDirection;
};