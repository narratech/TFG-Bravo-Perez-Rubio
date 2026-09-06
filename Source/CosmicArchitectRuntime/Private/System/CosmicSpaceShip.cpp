// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#include "System/CosmicSpaceShip.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h" 
#include "Components/PrimitiveComponent.h" 

ACosmicSpaceShip::ACosmicSpaceShip()
{
	// Allows continuous runtime update for:
	// - Physics simulation
	// - Space braking
	// - Boost systems
	PrimaryActorTick.bCanEverTick = true;

	// Automatically assigns control
	// to the primary local player.
	AutoPossessPlayer = EAutoReceiveInput::Player0; 

	// Main rigid body configuration
	// used for space navigation.
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	RootComponent = ShipMesh;

	// Enables full physics simulation.
	ShipMesh->SetSimulatePhysics(true);

	// Disables gravity for space movement.
	ShipMesh->SetEnableGravity(false);

	// Reduces linear drag to conserve inertia.
	ShipMesh->SetLinearDamping(0.0f);

	// Partially stabilizes angular rotation.
	ShipMesh->SetAngularDamping(0.5f);

	// Decoupled camera system
	// using spring arm.
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(ShipMesh);
	SpringArmComp->TargetArmLength = 800.0f;
	SpringArmComp->bEnableCameraRotationLag = true;

	// Prevents camera collisions in open space.
	SpringArmComp->bDoCollisionTest = false;

	// Main player-controlled camera.
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	// Initial configuration of braking system.
	BrakingSpeed = 5.0f;
}

void ACosmicSpaceShip::BeginPlay()
{
	Super::BeginPlay();

	// Removes global bounds restrictions
	// for large-scale navigation.
	if (UWorld* World = GetWorld())
	{
		if (AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			WorldSettings->bEnableWorldBoundsChecks = false;
		}
	}
}

void ACosmicSpaceShip::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Registers main input context
	// within player local subsystem.
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// Binds Enhanced Input actions
	// to runtime C++ logic.
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Traslacion) { EnhancedInputComponent->BindAction(IA_Traslacion, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::AplicarTraslacion); }
		if (IA_Orientacion) { EnhancedInputComponent->BindAction(IA_Orientacion, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::AplicarOrientacion); }
		if (IA_Alabeo) { EnhancedInputComponent->BindAction(IA_Alabeo, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::AplicarAlabeo); }

		if (IA_Boost)
		{
			EnhancedInputComponent->BindAction(IA_Boost, ETriggerEvent::Started, this, &ACosmicSpaceShip::StartBoost);
			EnhancedInputComponent->BindAction(IA_Boost, ETriggerEvent::Completed, this, &ACosmicSpaceShip::EndBoost);
		}
	}
}

void ACosmicSpaceShip::AplicarTraslacion(const FInputActionValue& Value)
{
	FVector MovementVector = Value.Get<FVector>();

	if (!MovementVector.IsNearlyZero() && ShipMesh)
	{
		// During boost only stabilized
		// forward acceleration is allowed.
		if (bBoostMode)
		{
			// Blocks lateral movement
			// and reverse during boost.
			if (MovementVector.X <= 0.5f || FMath::Abs(MovementVector.Y) > 0.5f)
			{
				return;
			}
		}

		// Local force applied proportionally
		// to input and delta time.
		FVector LocalForce = MovementVector * ThrusterForce * GetWorld()->GetDeltaSeconds();

		// Increases propulsion power
		// while boost is active.
		if (bBoostMode)
		{
			LocalForce *= BoostIncreasePower;
		}

		// Conversion from local space
		// to world coordinates.
		FVector WorldForce = ShipMesh->GetComponentRotation().RotateVector(LocalForce);

		// Final physics application on the rigid body.
		ShipMesh->AddForce(WorldForce, NAME_None, true);
	}
}

void ACosmicSpaceShip::AplicarOrientacion(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (!LookAxisVector.IsNearlyZero() && ShipMesh)
	{
		// Local torque used for:
		// - Pitch
		// - Yaw
		FVector LocalTorque = FVector(0.0f, LookAxisVector.Y * -1.0f, LookAxisVector.X) * RotationTorque * GetWorld()->GetDeltaSeconds();

		// Conversion from local space
		// to world space.
		FVector WorldTorque = ShipMesh->GetComponentRotation().RotateVector(LocalTorque);

		// Physics application of angular torque.
		ShipMesh->AddTorqueInDegrees(WorldTorque, NAME_None, true);
	}
}

void ACosmicSpaceShip::AplicarAlabeo(const FInputActionValue& Value)
{
	float RollValue = Value.Get<float>();

	if (FMath::Abs(RollValue) > 0.0f && ShipMesh)
	{
		// Roll is applied on the ship's
		// forward longitudinal axis.
		FVector LocalTorque = FVector(RollValue, 0.0f, 0.0f) * AlabeoTorque * GetWorld()->GetDeltaSeconds();

		// Conversion to world coordinates.
		FVector WorldTorque = ShipMesh->GetComponentRotation().RotateVector(LocalTorque);

		// Physics application of roll torque.
		ShipMesh->AddTorqueInDegrees(WorldTorque, NAME_None, true);
	}
}

void ACosmicSpaceShip::StartBoost(const FInputActionValue& Value)
{
	bBoostMode = true;

	// Increases linear stability during
	// high-speed navigation.
	if (ShipMesh)
	{
		// Saves original damping to restore it
		// upon boost completion.
		OriginalLinearDamping = ShipMesh->GetLinearDamping();

		// Reduces lateral drift during boost.
		ShipMesh->SetLinearDamping(0.1f);
	}
}

void ACosmicSpaceShip::EndBoost(const FInputActionValue& Value)
{
	bBoostMode = false;

	// Restores original physics configuration
	// after exiting boost.
	if (ShipMesh)
	{
		ShipMesh->SetLinearDamping(OriginalLinearDamping);
	}

	// Residual braking will be managed
	// later via Tick/runtime.
}