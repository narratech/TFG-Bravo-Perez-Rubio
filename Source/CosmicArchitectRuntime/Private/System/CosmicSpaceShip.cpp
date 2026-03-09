// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/CosmicSpaceShip.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

ACosmicSpaceShip::ACosmicSpaceShip()
{
	PrimaryActorTick.bCanEverTick = false; // No necesitamos Tick, todo va por físicas

	// E: Configuramos la malla como componente raíz y activamos las físicas.
	// I: We set the mesh as the root component and enable physics.
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	RootComponent = ShipMesh;

	ShipMesh->SetSimulatePhysics(true);
	ShipMesh->SetEnableGravity(false); // Estamos en el espacio
	ShipMesh->SetLinearDamping(0.0f);  // Sin fricción (inercia infinita al avanzar)
	ShipMesh->SetAngularDamping(0.0f); // Sin fricción al rotar

	// E: Configuramos el brazo de la cámara.
	// I: We configure the camera boom.
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(ShipMesh);
	SpringArmComp->TargetArmLength = 800.0f;
	SpringArmComp->bEnableCameraRotationLag = true;
	SpringArmComp->CameraRotationLagSpeed = 3.0f;
	SpringArmComp->bEnableCameraLag = true;
	SpringArmComp->CameraLagSpeed = 3.0f;
	SpringArmComp->bDoCollisionTest = false; // Evitamos que la cámara choque con la nave
	SpringArmComp->CameraLagMaxDistance = 0.0f;

	// E: Anclamos la cámara al brazo.
	// I: We attach the camera to the boom.
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);
}

void ACosmicSpaceShip::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Traslacion)
		{
			EnhancedInputComponent->BindAction(IA_Traslacion, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::AplicarTraslacion);
		}
		if (IA_Orientacion)
		{
			EnhancedInputComponent->BindAction(IA_Orientacion, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::AplicarOrientacion);
		}
		if (IA_Alabeo)
		{
			EnhancedInputComponent->BindAction(IA_Alabeo, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::AplicarAlabeo);
		}
	}
}

void ACosmicSpaceShip::AplicarTraslacion(const FInputActionValue& Value)
{
	// E: Recibimos un Vector 3D (X=Adelante, Y=Derecha, Z=Arriba).
	// I: We receive a 3D Vector (X=Forward, Y=Right, Z=Up).
	FVector MovementVector = Value.Get<FVector>();

	if (!MovementVector.IsNearlyZero())
	{
		// E: Aplicamos la fuerza propulsora en la dirección local de la nave. 'true' ignora la masa para estandarizar el control.
		// I: We apply the thruster force in the ship's local direction. 'true' ignores mass to standardize control.
		FVector ForceToApply = MovementVector * ThrusterForce * GetWorld()->GetDeltaSeconds();
		ShipMesh->AddRelativeForce(ForceToApply, NAME_None, true);
	}
}

void ACosmicSpaceShip::AplicarOrientacion(const FInputActionValue& Value)
{
	// E: Recibimos un Vector 2D del ratón o joystick.
	// I: We receive a 2D Vector from the mouse or joystick.
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (!LookAxisVector.IsNearlyZero())
	{
		// E: Unreal usa Pitch (Y) y Yaw (Z). El ratón X suele ser Yaw, el ratón Y suele ser Pitch.
		// I: Unreal uses Pitch (Y) and Yaw (Z). Mouse X is usually Yaw, Mouse Y is usually Pitch.
		FVector TorqueToApply = FVector(0.0f, LookAxisVector.Y * -1.0f, LookAxisVector.X) * RotationTorque * GetWorld()->GetDeltaSeconds();
		ShipMesh->AddRelativeTorque(TorqueToApply, NAME_None, true);
	}
}

void ACosmicSpaceShip::AplicarAlabeo(const FInputActionValue& Value)
{
	// E: Recibimos un valor de 1D (Roll).
	// I: We receive a 1D value (Roll).
	float RollValue = Value.Get<float>();

	if (RollValue != 0.0f)
	{
		// E: Aplicamos torsión sobre el eje X local.
		// I: We apply torque on the local X axis.
		FVector TorqueToApply = FVector(RollValue, 0.0f, 0.0f) * RotationTorque * GetWorld()->GetDeltaSeconds();
		ShipMesh->AddRelativeTorque(TorqueToApply, NAME_None, true);
	}
}