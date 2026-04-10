// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/CosmicRealisticPlayer.h" 
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Simulation/CosmicGravityComponent.h" 
#include "DrawDebugHelpers.h"

ACosmicRealisticPlayer::ACosmicRealisticPlayer()
{
	// E: El plugin requiere Tick para calcular la curvatura del planeta frame a frame.
	// I: The plugin requires Tick to calculate the planet's curvature frame by frame.
	PrimaryActorTick.bCanEverTick = true;

	// E: CRÍTICO: Debe ser PostPhysics. Corregimos la postura DESPUÉS de que la física de la esfera resuelva el movimiento.
	// I: CRITICAL: Must be PostPhysics. We correct the posture AFTER the sphere's physics resolve the movement.
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// =========================================================================
	// CREACIÓN DE COMPONENTES DEL PLUGIN
	// =========================================================================

	// E: 1. Raíz Física: Maneja colisiones y rodamiento.
	// I: 1. Physical Root: Handles collisions and rolling.
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->InitSphereRadius(40.0f);
	SphereComp->SetCollisionProfileName(TEXT("Pawn"));
	SphereComp->SetSimulatePhysics(true);

	// E: IMPORTANTE: Apagamos la gravedad de Unreal para que el componente CosmicGravity tome el control absoluto.
	// I: IMPORTANT: We disable Unreal's gravity so the CosmicGravity component takes absolute control.
	SphereComp->SetEnableGravity(false);
	RootComponent = SphereComp;

	// E: 2. VisualRoot: Plataforma estabilizada por el plugin para anclar la cámara.
	// I: 2. VisualRoot: Platform stabilized by the plugin to anchor the camera.
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);

	// E: 3. Brazo de la cámara con suavizado activado por defecto.
	// I: 3. Camera arm with smoothing enabled by default.
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(VisualRoot);
	SpringArmComp->TargetArmLength = 400.0f;
	SpringArmComp->bUsePawnControlRotation = false; // El plugin maneja su propia rotación local. / Plugin handles its own local rotation.
	SpringArmComp->bEnableCameraLag = true;
	SpringArmComp->bEnableCameraRotationLag = true;

	// E: 4. Cámara Principal.
	// I: 4. Main Camera.
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	// E: 5. MeshRoot: Nodo intermedio para permitir que el modelo 3D rote hacia donde camina.
	// I: 5. MeshRoot: Intermediate node to allow the 3D model to rotate towards its walking direction.
	MeshRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshRoot"));
	MeshRoot->SetupAttachment(VisualRoot);

	// E: 6. Malla de personaje (Bajada -40 en Z por defecto para tocar el suelo de la esfera).
	// I: 6. Character mesh (Lowered -40 on Z by default to touch the sphere's ground).
	PlayerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PlayerMesh"));
	PlayerMesh->SetupAttachment(MeshRoot);
	PlayerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));

	// E: 7. Detector de gravedad del plugin Cosmic Architect.
	// I: 7. Gravity detector from the Cosmic Architect plugin.
	GravityComp = CreateDefaultSubobject<UCosmicGravityComponent>(TEXT("GravityComp"));

	// E: Inicialización de la vista de cámara.
	// I: Camera view initialization.
	CameraYaw = 0.0f;
	CameraPitch = -20.0f;
}

void ACosmicRealisticPlayer::BeginPlay()
{
	Super::BeginPlay();

	// E: Inyección automática del contexto de controles si ha sido asignado en el Blueprint.
	// I: Automatic injection of the control context if it has been assigned in the Blueprint.
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

	if (SpringArmComp)
	{
		SpringArmComp->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	}
}

void ACosmicRealisticPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_PlayerMove) { EnhancedInputComponent->BindAction(IA_PlayerMove, ETriggerEvent::Triggered, this, &ACosmicRealisticPlayer::Move); }
		if (IA_PlayerMove) { EnhancedInputComponent->BindAction(IA_PlayerMove, ETriggerEvent::Completed, this, &ACosmicRealisticPlayer::Move); }
		if (IA_PlayerLook) { EnhancedInputComponent->BindAction(IA_PlayerLook, ETriggerEvent::Triggered, this, &ACosmicRealisticPlayer::Look); }
		if (IA_PlayerJump) { EnhancedInputComponent->BindAction(IA_PlayerJump, ETriggerEvent::Started, this, &ACosmicRealisticPlayer::Jump); }
	}
}

void ACosmicRealisticPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GravityComp || !VisualRoot || !MeshRoot) return;

	FVector GravityDown = GravityComp->CurrentGravityDirection;
	if (GravityDown.IsNearlyZero()) return;

	// E: El Arriba estricto. NUNCA lo suavizaremos para evitar que el personaje se incline.
	// I: The strict Up. We will NEVER smooth this to prevent the character from tilting.
	FVector TargetUp = -GravityDown;

	// =========================================================================
	// 1. ROTACIÓN ESTABLE (CÁMARA Y SISTEMA)
	// =========================================================================

	FVector StableForward = FVector::VectorPlaneProject(VisualRoot->GetForwardVector(), TargetUp).GetSafeNormal();
	if (StableForward.IsNearlyZero()) {
		StableForward = FVector::CrossProduct(VisualRoot->GetRightVector(), TargetUp).GetSafeNormal();
	}

	// E: Aplicación INSTANTÁNEA de la gravedad al VisualRoot.
	// I: INSTANT application of gravity to VisualRoot.
	FQuat TargetVisualQuat = FRotationMatrix::MakeFromXZ(StableForward, TargetUp).ToQuat();
	VisualRoot->SetWorldRotation(TargetVisualQuat);

	// =========================================================================
	// 2. ROTACIÓN VISUAL DEL PERSONAJE (INPUT)
	// =========================================================================

	FVector MeshDesiredForward;

	if (!TargetFacingDirection.IsNearlyZero())
	{
		MeshDesiredForward = FVector::VectorPlaneProject(TargetFacingDirection, TargetUp).GetSafeNormal();
	}
	else
	{
		MeshDesiredForward = FVector::VectorPlaneProject(MeshRoot->GetForwardVector(), TargetUp).GetSafeNormal();
		if (MeshDesiredForward.IsNearlyZero()) {
			MeshDesiredForward = FVector::CrossProduct(MeshRoot->GetRightVector(), TargetUp).GetSafeNormal();
		}
	}

	// E: Interpolamos SOLO el vector frontal (giro de cintura), no el vector Arriba (Gravedad).
	// I: We interpolate ONLY the forward vector (waist twist), not the Up vector (Gravity).
	FVector CurrentMeshForward = MeshRoot->GetForwardVector();
	FVector SmoothForward = FMath::VInterpTo(CurrentMeshForward, MeshDesiredForward, DeltaTime, 15.0f).GetSafeNormal();

	if (SmoothForward.IsNearlyZero()) {
		SmoothForward = MeshDesiredForward;
	}

	// E: Construimos la rotación. Z siempre es TargetUp (perfectamente vertical al planeta).
	// I: Build rotation. Z is always TargetUp (perfectly vertical to the planet).
	FQuat TargetMeshQuat = FRotationMatrix::MakeFromXZ(SmoothForward, TargetUp).ToQuat();
	MeshRoot->SetWorldRotation(TargetMeshQuat);
}

void ACosmicRealisticPlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (MovementVector.IsNearlyZero())
	{
		TargetFacingDirection = FVector::ZeroVector;
		return;
	}

	if (Controller && VisualRoot && CameraComp && SphereComp)
	{
		// E: Calculamos el movimiento relativo a la cámara del jugador, sin importar la curvatura.
		// I: We calculate movement relative to the player's camera, regardless of curvature.
		FVector CameraForward = CameraComp->GetForwardVector();
		FVector CameraRight = CameraComp->GetRightVector();
		FVector UpVector = VisualRoot->GetUpVector();

		FVector ForwardOnGround = FVector::VectorPlaneProject(CameraForward, UpVector).GetSafeNormal();
		FVector RightOnGround = FVector::VectorPlaneProject(CameraRight, UpVector).GetSafeNormal();

		FVector MoveDirection = (ForwardOnGround * MovementVector.Y) + (RightOnGround * MovementVector.X);
		TargetFacingDirection = MoveDirection.GetSafeNormal();

		// E: Aplicamos la fuerza configurable expuesta al Blueprint del usuario.
		// I: Apply the configurable force exposed to the user's Blueprint.
		FVector ForceToApply = TargetFacingDirection * MovementForce;

		SphereComp->AddForce(ForceToApply, NAME_None, false);

		// E: Herramienta de depuración visual del plugin (Flecha de dirección).
		// I: Plugin's visual debugging tool (Directional arrow).
		FVector StartLoc = SphereComp->GetComponentLocation();
		float VisualArrowLength = 200.0f;
		FVector EndLoc = StartLoc + (TargetFacingDirection * VisualArrowLength);

		DrawDebugDirectionalArrow(GetWorld(), StartLoc, EndLoc, 20.0f, FColor::White, false, -1.0f, 0, 4.0f);
	}
}

void ACosmicRealisticPlayer::Look(const FInputActionValue& Value)
{
	// E: El usuario puede modificar la sensibilidad general desde el Blueprint.
	// I: The user can modify the overall sensitivity from the Blueprint.
	FVector2D LookAxisVector = Value.Get<FVector2D>() * MouseSensitivity;

	if (SpringArmComp)
	{
		CameraYaw += LookAxisVector.X;
		CameraPitch += LookAxisVector.Y;

		// E: Evita que la cámara del plugin dé giros completos de 360 grados verticalmente.
		// I: Prevents the plugin's camera from doing full 360-degree vertical flips.
		CameraPitch = FMath::Clamp(CameraPitch, -85.0f, 85.0f);

		SpringArmComp->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	}
}

void ACosmicRealisticPlayer::Jump(const FInputActionValue& Value)
{
	if (SphereComp && VisualRoot)
	{
		// E: Aplica un impulso físico empujando la masa en contra de la gravedad del planeta actual.
		// I: Applies a physical impulse pushing the mass against the current planet's gravity.
		FVector JumpImpulse = VisualRoot->GetUpVector() * 800.0f;
		SphereComp->AddImpulse(JumpImpulse, NAME_None, true);
	}
}