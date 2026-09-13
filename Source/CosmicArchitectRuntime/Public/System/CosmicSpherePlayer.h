// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Terrain/ICosmicCollisionTarget.h"
#include "CosmicSpherePlayer.generated.h"

class UCosmicGravityComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

/**
 * Main Character used for planetary navigation.
 *
 * Inherits from ACharacter to leverage Unreal Engine 5.7 native arbitrary gravity direction
 * (UCharacterMovementComponent::SetGravityDirection) and networked prediction/replication.
 */
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicSpherePlayer : public ACharacter, public ICosmicCollisionTarget
{
	GENERATED_BODY()

public:

	/**
	 * Planetary character constructor.
	 * Initializes components, movement parameters, and decoupled camera.
	 */
	ACosmicSpherePlayer();

	/**
	 * Main character tick.
	 * Updates gravity direction from CosmicGravityComponent and camera orientation.
	 *
	 * @param DeltaTime Time between frames.
	 */
	virtual void Tick(float DeltaTime) override;

	// ~ICosmicCollisionTarget interface
	virtual bool IsCollisionRelevant() const override;
	virtual float GetCollisionPriority() const override;
	// ~End ICosmicCollisionTarget interface

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/**
	 * Visual root aligned with local gravity.
	 * Keeps the local Z axis oriented strictly to the planet surface normal (TargetUp).
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	TObjectPtr<USceneComponent> VisualRoot;

	/**
	 * Spring arm for decoupled orbital camera.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	TObjectPtr<USpringArmComponent> SpringArmComp;

	/**
	 * Main player camera.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	TObjectPtr<UCameraComponent> CameraComp;

	/**
	 * Custom gravity component.
	 * Queries gravitational field from planetary bodies registered in CosmicGravitySubsystem.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	TObjectPtr<UCosmicGravityComponent> GravityComp;

	/**
	 * Indicates whether the player is grounded on a valid surface.
	 * Read by ABP_CosmicPlayer for transition logic.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "CosmicArchitect|Movement")
	bool bIsGroundedState = false;

	/**
	 * Accumulated vertical velocity along the local gravity axis.
	 * Read by ABP_CosmicPlayer for jump/fall blend spaces.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "CosmicArchitect|Movement")
	float VerticalVelocity = 0.0f;

	/**
	 * Returns whether the player is currently on valid ground.
	 */
	UFUNCTION(BlueprintPure, Category = "CosmicArchitect|Movement")
	bool IsGrounded() const { return bIsGroundedState; }

	/**
	 * Returns the current planetary gravity magnitude in cm/s².
	 */
	UFUNCTION(BlueprintPure, Category = "CosmicArchitect|Movement")
	float GetCurrentGravityMagnitude() const;

	/** Mouse sensitivity for camera control */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Input")
	float MouseSensitivity = 1.0f;

	/** Base walking speed on planetary surface (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Movement")
	float BaseWalkSpeed = 600.0f;

	/** Base jump velocity along the local gravity Up axis (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Movement")
	float BaseJumpVelocity = 600.0f;

	/** Accumulated yaw rotation for camera (degrees) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Camera")
	float CameraYaw = 0.0f;

	/** Accumulated pitch rotation for camera (degrees) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Camera")
	float CameraPitch = -20.0f;

	/** Player movement action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	TObjectPtr<UInputAction> IA_PlayerMove;

	/** Camera look action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	TObjectPtr<UInputAction> IA_PlayerLook;

	/** Player jump action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	TObjectPtr<UInputAction> IA_PlayerJump;

private:

	/** Handles WASD movement input projected onto planet surface */
	void Move(const FInputActionValue& Value);

	/** Handles mouse look input for orbital camera */
	void Look(const FInputActionValue& Value);
};