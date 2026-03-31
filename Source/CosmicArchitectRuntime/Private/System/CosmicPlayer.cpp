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

// E: Librería necesaria para dibujar formas de depuración (Líneas, flechas, colisionadores).
// I: Library needed to draw debug shapes (Lines, arrows, colliders).
#include "DrawDebugHelpers.h"

ACosmicPlayer::ACosmicPlayer()
{
	// E: Activamos el Tick para aplicar la alineación gravitacional frame a frame.
	// I: We enable Tick to apply gravitational alignment frame by frame.
	PrimaryActorTick.bCanEverTick = true;

	// E: Forzamos que nuestro Tick ocurra DESPUÉS de que el motor de físicas haya resuelto las colisiones.
	// Esto es crucial para evitar temblores (jitter) en la rotación.
	// I: Force our Tick to happen AFTER the physics engine has resolved collisions.
	// This is crucial to avoid rotation jitter.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// E: Configuración de la cápsula física (Raíz).
	// I: Physical capsule setup (Root).
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	RootComponent = CapsuleComp;
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);

	// E: Habilitamos las físicas y deshabilitamos la gravedad por defecto de Unreal (Z- constante).
	// I: Enable physics and disable Unreal's default gravity (constant Z-).
	CapsuleComp->SetSimulatePhysics(true);
	CapsuleComp->SetEnableGravity(false);

	// E: Bloqueamos la rotación física (X, Y, Z). Nosotros controlaremos la orientación por código en el Tick.
	// Si no se bloquea, la cápsula volcaría al colisionar.
	// I: We lock physical rotation (X, Y, Z). We will control orientation via code in Tick.
	// If not locked, the capsule would tip over on collision.
	CapsuleComp->BodyInstance.bLockXRotation = false;
	CapsuleComp->BodyInstance.bLockYRotation = false;
	CapsuleComp->BodyInstance.bLockZRotation = false;

	// [E: EL TRUCO: Escalamos el Tensor de Inercia a un valor colosal. Esto hace que chocar contra 
	// paredes o el suelo no tenga fuerza suficiente para volcarte, pero SetWorldRotation sí pueda girarte]
	// [I: THE TRICK: Scale Inertia Tensor to a colossal value. This makes hitting walls or ground 
	// lack the force to tip you over, but SetWorldRotation can still turn you]
	CapsuleComp->BodyInstance.InertiaTensorScale = FVector(10000.0f, 10000.0f, 10000.0f);

	// [E: Aplicamos una amortiguación angular altísima para estabilizar cualquier bamboleo]
	// [I: Apply extremely high angular damping to stabilize any wobbling]
	CapsuleComp->SetAngularDamping(10.0f);

	// E: Amortiguación lineal alta (Fricción) para que el jugador se detenga al soltar las teclas.
	// I: High linear damping (Friction) so the player stops when releasing keys.
	CapsuleComp->SetLinearDamping(1.0f);

	// E: Configuración del brazo elástico (SpringArm).
	// I: Setup for spring arm.
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(CapsuleComp);
	SpringArmComp->TargetArmLength = 0.0f; // E: 0.0f para cámara en primera persona. / I: 0.0f for first person camera.

	// E: Usamos la rotación del controlador (ratón) para girar el brazo.
	// I: Use controller rotation (mouse) to rotate the arm.
	SpringArmComp->bUsePawnControlRotation = true;

	// E: Activamos el "Lag" de cámara para suavizar tirones físicos y absorber vibraciones del terreno.
	// I: Enable Camera "Lag" to smooth physical jerks and absorb terrain vibrations.
	SpringArmComp->bEnableCameraLag = true;
	SpringArmComp->bEnableCameraRotationLag = true;
	SpringArmComp->CameraLagSpeed = 15.0f;
	SpringArmComp->CameraRotationLagSpeed = 20.0f;

	// E: Configuración de la cámara.
	// I: Camera setup.
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);

	// E: Desactivamos bUsePawnControlRotation en la cámara directamente para que herede la rotación suave del SpringArm.
	// I: Disable bUsePawnControlRotation on camera directly so it inherits SpringArm's smooth rotation.
	CameraComp->bUsePawnControlRotation = false;

	// E: Instanciamos el componente gravitacional de tu plugin.
	// I: Instantiate your plugin's gravitational component.
	GravityComp = CreateDefaultSubobject<UCosmicGravityComponent>(TEXT("GravityComp"));
	GravityComp->IsPlanet = false; // E: Somos un objeto afectado por gravedad, no un planeta generador. / I: We are an object affected by gravity, not a generating planet.

}

void ACosmicPlayer::BeginPlay()
{
	Super::BeginPlay();

	// =========================================================================
	// REQUERIMIENTO: RENDERIZAR EL COLLIDER DE LA CÁPSULA
	// REQUERIMENT: RENDER THE CAPSULE COLLIDER
	// =========================================================================
	if (CapsuleComp)
	{
		// E: Por defecto, los colisionadores están ocultos en el juego. Forzamos su visibilidad para depuración.
		// I: By default, colliders are hidden in game. We force visibility for debugging.
		CapsuleComp->SetHiddenInGame(false);
	}
}

void ACosmicPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// E: 1. Asegurarnos de que tenemos el componente de gravedad y datos válidos.
	// I: 1. Make sure we have the gravity component and valid data.
	if (!GravityComp) return;

	// E: 2. Obtenemos la última dirección de gravedad calculada por el subsistema (Hacia abajo).
	// I: 2. Get the last gravity direction calculated by the subsystem (Downwards).
	FVector GravityDown = GravityComp->CurrentGravityDirection;

	// E: Si no hay fuerza de gravedad, no alteramos la rotación para evitar errores matemáticos.
	// I: If there is no gravity force, we don't alter rotation to avoid mathematical errors.
	if (GravityDown.IsNearlyZero()) return;

	// E: 3. El vector "Arriba" (donde apuntará la cabeza) es exactamente opuesto a la gravedad.
	// I: 3. The "Up" vector (where the head points) is exactly opposite to gravity.
	FVector TargetUp = -GravityDown;

	// E: 4. Proyectamos el vector frontal actual (donde mira el pecho) sobre el plano inclinado del suelo actual.
	// Esto es crucial para mantener la estabilidad y que el ratón (Yaw local) siga funcionando bien.
	// I: 4. Project the current forward vector (where the chest looks) onto the current ground's tilted plane.
	// This is crucial for maintaining stability and ensuring the mouse (local Yaw) keeps working correctly.
	FVector CurrentForward = CapsuleComp->GetForwardVector();
	FVector NewForward = FVector::VectorPlaneProject(CurrentForward, TargetUp).GetSafeNormal();

	// E: 5. Creamos una matriz de rotación perfecta definiendo cuál es nuestro Frente (X) y nuestro Arriba (Z).
	// I: 5. Create a perfect rotation matrix by defining our Forward (X) and Up (Z).
	FQuat TargetQuat = FRotationMatrix::MakeFromXZ(NewForward, TargetUp).ToQuat();
	FQuat CurrentQuat = CapsuleComp->GetComponentQuat();

	// E: 6. Interpolación Esférica (Slerp) para que la cápsula se alinee suavemente a la curvatura en vez de dar tirones.
	// I: 6. Spherical Interpolation (Slerp) so the capsule aligns smoothly to the curvature instead of snapping.
	FQuat NewQuat = FMath::QInterpTo(CurrentQuat, TargetQuat, DeltaTime, 5.0f);

	// =========================================================================
	// DEBUG VISUAL: PINTAR VECTORES DE ROTACIÓN (Rojo, Verde, Azul)
	// I: VISUAL DEBUG: DRAW ROTATION VECTORS (Red, Green, Blue)
	// =========================================================================
	FVector StartLoc = CapsuleComp->GetComponentLocation(); // E: Origen / I: Origin
	float LineLength = 150.0f; // E: Longitud flechas / I: Arrow length
	float ArrowSize = 10.0f;   // E: Tamaño punta / I: Tip size
	float Thickness = 2.0f;    // E: Grosor / I: Thickness

	// E: Flecha ROJA -> Vector "TargetUp" (Contrario a gravedad, hacia donde queremos ir).
	DrawDebugDirectionalArrow(GetWorld(), StartLoc, StartLoc + (TargetUp * LineLength), ArrowSize, FColor::Red, false, -1.0f, 0, Thickness);

	// E: Flecha VERDE -> Vector "CurrentForward" (Hacia dónde miramos realmente ahora).
	DrawDebugDirectionalArrow(GetWorld(), StartLoc, StartLoc + (CurrentForward * LineLength), ArrowSize, FColor::Green, false, -1.0f, 0, Thickness);

	// E: Flecha AZUL -> Vector "NewForward" (La dirección frontal estabilizada paralela al suelo local).
	DrawDebugDirectionalArrow(GetWorld(), StartLoc, StartLoc + (NewForward * LineLength), ArrowSize, FColor::Blue, false, -1.0f, 0, Thickness);
	// =========================================================================

	// E: 7. Aplicamos la nueva rotación a la cápsula usando TeleportPhysics para no interferir con el motor de colisiones.
	// I: 7. Apply new rotation to capsule using TeleportPhysics so as not to interfere with collision engine.
	CapsuleComp->SetWorldRotation(NewQuat, false, nullptr, ETeleportType::TeleportPhysics);
}

void ACosmicPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// E: Registro del contexto de mapeo (Enhanced Input).
	// I: Mapping context registration (Enhanced Input).
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

	// E: Vinculación de las Input Actions con los métodos de C++.
	// I: Binding Input Actions with C++ methods.
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_PlayerMove) { EnhancedInputComponent->BindAction(IA_PlayerMove, ETriggerEvent::Triggered, this, &ACosmicPlayer::Move); }
		if (IA_PlayerLook) { EnhancedInputComponent->BindAction(IA_PlayerLook, ETriggerEvent::Triggered, this, &ACosmicPlayer::Look); }
		if (IA_PlayerJump) { EnhancedInputComponent->BindAction(IA_PlayerJump, ETriggerEvent::Started, this, &ACosmicPlayer::Jump); }
	}
}

void ACosmicPlayer::Move(const FInputActionValue& Value)
{
	// E: Obtenemos el vector 2D de input (W/S -> Y, A/D -> X).
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller && CapsuleComp && CameraComp)
	{
		// =========================================================================
		// [NUEVO] CÁLCULO DE DIRECCIÓN 6DOF (Gravedad Esférica)
		// =========================================================================

		// 1. Obtenemos cuál es nuestro "Arriba" local (ya estabilizado por el Tick)
		FVector UpDirection = CapsuleComp->GetUpVector();

		// 2. Tomamos los vectores puros de la cámara
		FVector CameraForward = CameraComp->GetForwardVector();
		FVector CameraRight = CameraComp->GetRightVector();

		// 3. Proyectamos la visión de la cámara sobre el plano del suelo actual
		// Esto asegura que caminar hacia adelante nunca nos empuje hacia el cielo o hacia el núcleo del planeta
		FVector ForwardDirection = FVector::VectorPlaneProject(CameraForward, UpDirection).GetSafeNormal();
		FVector RightDirection = FVector::VectorPlaneProject(CameraRight, UpDirection).GetSafeNormal();

		// =========================================================================

		// E: Calculamos la fuerza física final
		FVector ForceToApply = (ForwardDirection * MovementVector.Y + RightDirection * MovementVector.X) * MovementForce;

		// E: Si hay algún input...
		if (!ForceToApply.IsNearlyZero())
		{
			// [NUEVO] Guardamos la dirección para que el código del Tick rote la cápsula hacia allí
			TargetFacingDirection = ForceToApply.GetSafeNormal();

			FVector DebugStart = CapsuleComp->GetComponentLocation();
			FVector ForceDirection = ForceToApply.GetSafeNormal();
			float DebugArrowLength = 100.0f;

			// Flecha BLANCA de depuración
			DrawDebugDirectionalArrow(GetWorld(), DebugStart, DebugStart + (ForceDirection * DebugArrowLength), 20.0f, FColor::White, false, -1.0f, 0, 4.0f);
		}

		// E: Aplicamos la fuerza física al colisionador.
		CapsuleComp->AddForce(ForceToApply, NAME_None, false);
	}
}

void ACosmicPlayer::Look(const FInputActionValue& Value)
{
	// E: Obtenemos el movimiento del ratón escalado por la sensibilidad.
	// I: Get mouse movement scaled by sensitivity.
	FVector2D LookAxisVector = Value.Get<FVector2D>() * MouseSensitivity;

	if (Controller)
	{
		// E: LÓGICA EXISTENTE: Inyectamos el movimiento en el PlayerController para girar la cámara.
		// I: EXISTING LOGIC: Inject movement into PlayerController to turn camera.
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ACosmicPlayer::Jump(const FInputActionValue& Value)
{
	if (CapsuleComp)
	{
		// E: LÓGICA EXISTENTE: El salto es un impulso hacia el "Arriba" local del actor actual.
		// I: EXISTING LOGIC: Jump is an impulse towards the current actor's local "Up".
		FVector UpDirection = GetActorUpVector();

		// E: LÓGICA EXISTENTE: bVelChange = true para ignorar la masa e impartir velocidad instantánea.
		// I: EXISTING LOGIC: bVelChange = true to ignore mass and impart instant velocity.
		CapsuleComp->AddImpulse(UpDirection * JumpForce, NAME_None, true);
	}
}