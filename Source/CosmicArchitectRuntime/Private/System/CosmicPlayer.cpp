// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/CosmicPlayer.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

ACosmicPlayer::ACosmicPlayer()
{
	// E: Activamos el Tick porque necesitamos aplicar la gravedad esférica frame a frame.
	// I: We enable Tick because we need to apply spherical gravity frame by frame.
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// E: Configuración de la cápsula física.
	// I: Physical capsule setup.
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	RootComponent = CapsuleComp;
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
	CapsuleComp->SetSimulatePhysics(true);
	CapsuleComp->SetEnableGravity(false); // E: Desactivamos la gravedad Z de Unreal. I: Disable Unreal's Z gravity.

	// E: Bloqueamos la rotación física para que la cápsula no "vuelque" como una pelota.
	// I: We lock physical rotation so the capsule doesn't "roll" like a ball.
	CapsuleComp->BodyInstance.bLockXRotation = true;
	CapsuleComp->BodyInstance.bLockYRotation = true;
	CapsuleComp->BodyInstance.bLockZRotation = true;
	CapsuleComp->SetLinearDamping(2.0f); // E: Fricción simulada. I: Simulated friction.

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(CapsuleComp);
	SpringArmComp->TargetArmLength = 300.0f;
	SpringArmComp->bUsePawnControlRotation = true; // E: Crucial para mirar con el ratón. I: Crucial for mouse look.

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;
}

void ACosmicPlayer::BeginPlay()
{
	Super::BeginPlay();
}

void ACosmicPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// E: AQUÍ IRÁ LA LÓGICA DE GRAVEDAD ESFÉRICA.
	// Calcularemos la dirección hacia el CurrentPlanetCenter, aplicaremos fuerza de gravedad
	// y rotaremos el Actor (Slerp) para que su vector Up (Z) apunte en dirección opuesta al centro del planeta.
	// I: SPHERICAL GRAVITY LOGIC GOES HERE.
	// We will calculate direction to CurrentPlanetCenter, apply gravity force
	// and rotate the Actor (Slerp) so its Up vector (Z) points away from the planet's center.
}

void ACosmicPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

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

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_PlayerMove) { EnhancedInputComponent->BindAction(IA_PlayerMove, ETriggerEvent::Triggered, this, &ACosmicPlayer::Move); }
		if (IA_PlayerLook) { EnhancedInputComponent->BindAction(IA_PlayerLook, ETriggerEvent::Triggered, this, &ACosmicPlayer::Look); }
		if (IA_PlayerJump) { EnhancedInputComponent->BindAction(IA_PlayerJump, ETriggerEvent::Started, this, &ACosmicPlayer::Jump); }
	}
}

void ACosmicPlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller && CapsuleComp)
	{
		// E: Obtenemos las direcciones basadas en la rotación actual de control, 
		// ignorando el Pitch para no movernos hacia el cielo o el suelo.
		// I: We get directions based on the current control rotation, 
		// ignoring Pitch so we don't move into the sky or ground.
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// E: Calculamos los vectores Forward y Right relativos al jugador.
		// I: We calculate Forward and Right vectors relative to the player.
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// E: Aplicamos fuerza física en lugar de "AddMovementInput" tradicional.
		// I: We apply physical force instead of traditional "AddMovementInput".
		FVector ForceToApply = (ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X) * MovementForce;
		CapsuleComp->AddForce(ForceToApply, NAME_None, true);
	}
}

void ACosmicPlayer::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller)
	{
		// E: El SpringArm usa esta rotación para mover la cámara.
		// I: The SpringArm uses this rotation to move the camera.
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ACosmicPlayer::Jump(const FInputActionValue& Value)
{
	if (CapsuleComp)
	{
		// E: El salto debe aplicarse en la dirección "Arriba" LOCAL del jugador, no en el eje Z global.
		// I: Jump must be applied in the player's LOCAL "Up" direction, not the global Z axis.
		FVector UpDirection = GetActorUpVector();
		CapsuleComp->AddImpulse(UpDirection * JumpForce, NAME_None, true);
	}
}