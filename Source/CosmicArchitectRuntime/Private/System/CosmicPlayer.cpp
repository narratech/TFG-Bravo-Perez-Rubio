// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/CosmicPlayer.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Simulation/CosmicGravityComponent.h"

ACosmicPlayer::ACosmicPlayer()
{
	// E: Activamos el Tick para aplicar la gravedad esférica frame a frame.
	// I: We enable Tick to apply spherical gravity frame by frame.
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// E: Configuración de la cápsula física (Raíz).
	// I: Physical capsule setup (Root).
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	RootComponent = CapsuleComp;
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);

	// E: Habilitamos las físicas y deshabilitamos la gravedad por defecto (Z-).
	// I: Enable physics and disable default gravity (Z-).
	CapsuleComp->SetSimulatePhysics(true);
	CapsuleComp->SetEnableGravity(false);

	// E: Bloqueamos la rotación física para que la cápsula no vuelque, nosotros controlaremos el "Arriba" por código.
	// I: We lock physical rotation so the capsule doesn't roll, we will control "Up" via code.
	CapsuleComp->BodyInstance.bLockXRotation = true;
	CapsuleComp->BodyInstance.bLockYRotation = true;
	CapsuleComp->BodyInstance.bLockZRotation = true;

	// E: Amortiguación lineal alta para simular fricción con el suelo y que no resbale como hielo.
	// I: High linear damping to simulate ground friction so it doesn't slide like ice.
	CapsuleComp->SetLinearDamping(4.0f);

	// E: Configuración de la cámara y su brazo elástico.
	// I: Setup for camera and its spring arm.
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(CapsuleComp);
	SpringArmComp->TargetArmLength = 0.0f; // E: 0.0f para primera persona o 300.0f para tercera persona. / I: 0.0f for first person or 300.0f for third.
	SpringArmComp->bUsePawnControlRotation = true;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	// E: Instanciamos el componente gravitacional.
	// I: Instantiate the gravitational component.
	GravityComp = CreateDefaultSubobject<UCosmicGravityComponent>(TEXT("GravityComp"));
	GravityComp->IsPlanet = false;
}

void ACosmicPlayer::BeginPlay()
{
	Super::BeginPlay();
}

void ACosmicPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// E: 1. Asegurarnos de que tenemos el componente de gravedad
	// I: 1. Make sure we have the gravity component
	if (!GravityComp) return;

	// E: 2. Obtenemos la dirección hacia donde tira la gravedad (Hacia abajo)
	// I: 2. Get the direction gravity is pulling towards (Downwards)
	FVector GravityDown = GravityComp->CurrentGravityDirection;

	// Si no hay gravedad actuando, no alteramos la rotación
	if (GravityDown.IsNearlyZero()) return;

	// E: 3. El "Arriba" al que queremos que apunte la cabeza del jugador es lo contrario a la gravedad
	// I: 3. The "Up" we want the player's head to point to is the opposite of gravity
	FVector TargetUp = -GravityDown;

	// E: 4. Proyectamos el frente actual del jugador sobre el nuevo plano del suelo.
	// Esto evita que el personaje gire bruscamente perdiendo la dirección en la que estabas mirando.
	// I: 4. Project the current forward of the player onto the new ground plane.
	FVector CurrentForward = CapsuleComp->GetForwardVector();
	FVector NewForward = FVector::VectorPlaneProject(CurrentForward, TargetUp).GetSafeNormal();

	// E: 5. Creamos la rotación deseada diciéndole a Unreal cuál es nuestro Frente (X) y nuestro Arriba (Z)
	// I: 5. Create the desired rotation telling Unreal our Forward (X) and Up (Z)
	FQuat TargetQuat = FRotationMatrix::MakeFromXZ(NewForward, TargetUp).ToQuat();
	FQuat CurrentQuat = CapsuleComp->GetComponentQuat();

	// E: 6. Interpolación Esférica (Slerp) para que el ajuste sea suave y no un "chasquido"
	// I: 6. Spherical Interpolation (Slerp) so the adjustment is smooth and not a snap
	// El valor '5.0f' es la velocidad de rotación. Puedes ajustarlo a tu gusto.
	FQuat NewQuat = FMath::QInterpTo(CurrentQuat, TargetQuat, DeltaTime, 5.0f);

	// E: 7. Aplicamos la nueva rotación a la cápsula
	// I: 7. Apply the new rotation to the capsule
	CapsuleComp->SetWorldRotation(NewQuat, false, nullptr, ETeleportType::TeleportPhysics);
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
		// E: Obtenemos las direcciones relativas ignorando la inclinación (Pitch/Roll) para no volar ni enterrarnos.
		// I: We get relative directions ignoring pitch/roll so we don't fly or dig into the ground.
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// E: Calculamos la fuerza física basada en los inputs y la variable configurable MovementForce. Sin DeltaTime.
		// I: Calculate physical force based on inputs and the configurable MovementForce variable. No DeltaTime.
		FVector ForceToApply = (ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X) * MovementForce;

		// E: LÍNEA DE DEBUG - Imprime los valores X e Y del teclado en color verde.
		// I: DEBUG LINE - Prints the X and Y keyboard values in green.
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Green, FString::Printf(TEXT("x: %f, y: %f"), ForceToApply.X,ForceToApply.Y));
		}

		// E: Aplicamos la fuerza. bAccelChange = false para que respete la masa del jugador.
		// I: Apply force. bAccelChange = false so it respects the player's mass.
		CapsuleComp->AddForce(ForceToApply, NAME_None, false);
	}
}

void ACosmicPlayer::Look(const FInputActionValue& Value)
{
	// E: Obtenemos el input del ratón y le aplicamos el multiplicador configurable desde el Editor.
	// I: Get mouse input and apply the configurable multiplier from the Editor.
	FVector2D LookAxisVector = Value.Get<FVector2D>() * MouseSensitivity;

	if (Controller)
	{
		// E: Inyectamos el movimiento escalado en el controlador de cámara.
		// I: Inject the scaled movement into the camera controller.
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ACosmicPlayer::Jump(const FInputActionValue& Value)
{
	if (CapsuleComp)
	{
		// E: El salto es un impulso instantáneo hacia el "Arriba" local del actor, escalado por JumpForce.
		// I: Jump is an instant impulse towards the actor's local "Up", scaled by JumpForce.
		FVector UpDirection = GetActorUpVector();

		// E: bVelChange = true para el impulso, ignora la masa y asegura un salto consistente independientemente del peso.
		// I: bVelChange = true for the impulse, ignores mass and ensures a consistent jump regardless of weight.
		CapsuleComp->AddImpulse(UpDirection * JumpForce, NAME_None, true);
	}
}