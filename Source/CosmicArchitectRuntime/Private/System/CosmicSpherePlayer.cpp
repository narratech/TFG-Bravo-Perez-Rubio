// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/CosmicSpherePlayer.h"
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

ACosmicSpherePlayer::ACosmicSpherePlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	// E: Como ya no forzamos rotaciones físicas, el Tick puede ir en el grupo por defecto.
	// I: Since we no longer force physics rotations, Tick can stay in the default group.
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// =========================================================================
	// CREACIÓN DE COMPONENTES
	// =========================================================================

	// E: 1. La esfera es la raíz. Se encarga de las colisiones y las físicas.
	// I: 1. Sphere is the root. It handles collisions and physics.
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->InitSphereRadius(40.0f);
	SphereComp->SetCollisionProfileName(TEXT("Pawn"));
	SphereComp->SetSimulatePhysics(true);
	RootComponent = SphereComp;

	// E: 2. El VisualRoot es hijo de la esfera. Este es el que giraremos por código.
	// I: 2. VisualRoot is child of the sphere. This is the one we will rotate via code.
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);

	// E: 3. Malla del personaje acoplada al VisualRoot.
	// I: 3. Character mesh attached to VisualRoot.
	PlayerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PlayerMesh"));
	PlayerMesh->SetupAttachment(VisualRoot);
	// Bajamos la malla para que sus pies toquen el fondo de la esfera de 40 unidades de radio.
	PlayerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));

	// E: 4. El brazo de cámara acoplado al VisualRoot para heredar la orientación del planeta.
	// I: 4. Camera boom attached to VisualRoot to inherit planet orientation.
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(VisualRoot);
	SpringArmComp->TargetArmLength = 400.0f;
	SpringArmComp->bUsePawnControlRotation = false; // Usamos nuestra lógica local.
	SpringArmComp->bEnableCameraLag = true;
	SpringArmComp->bEnableCameraRotationLag = true;

	// E: 5. Cámara principal.
	// I: 5. Main camera.
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	// E: 6. Componente de Gravedad.
	// I: 6. Gravity Component.
	GravityComp = CreateDefaultSubobject<UCosmicGravityComponent>(TEXT("GravityComp"));

	// Inicialización de variables
	CameraYaw = 0.0f;
	CameraPitch = -20.0f;
}

void ACosmicSpherePlayer::BeginPlay()
{
	Super::BeginPlay();

	// E: Añadir el contexto de mapeo inicial para Enhanced Input.
	// I: Add the initial mapping context for Enhanced Input.
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

	// E: Aplicamos la rotación inicial de la cámara.
	// I: Apply initial camera rotation.
	if (SpringArmComp)
	{
		SpringArmComp->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	}
}

void ACosmicSpherePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_PlayerMove) { EnhancedInputComponent->BindAction(IA_PlayerMove, ETriggerEvent::Triggered, this, &ACosmicSpherePlayer::Move); }
		if (IA_PlayerMove) { EnhancedInputComponent->BindAction(IA_PlayerMove, ETriggerEvent::Completed, this, &ACosmicSpherePlayer::Move); }
		if (IA_PlayerLook) { EnhancedInputComponent->BindAction(IA_PlayerLook, ETriggerEvent::Triggered, this, &ACosmicSpherePlayer::Look); }
		if (IA_PlayerJump) { EnhancedInputComponent->BindAction(IA_PlayerJump, ETriggerEvent::Started, this, &ACosmicSpherePlayer::Jump); }
	}
}

void ACosmicSpherePlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GravityComp || !VisualRoot) return;

	FVector GravityDown = GravityComp->CurrentGravityDirection;
	if (GravityDown.IsNearlyZero()) return;

	FVector TargetUp = -GravityDown;

	// E: Calculamos el frente deseado basado en el input o en la cámara.
	// I: Calculate desired forward based on input or camera.
	FVector DesiredForward;
	if (!TargetFacingDirection.IsNearlyZero())
	{
		DesiredForward = FVector::VectorPlaneProject(TargetFacingDirection, TargetUp).GetSafeNormal();
	}
	else
	{
		DesiredForward = FVector::VectorPlaneProject(VisualRoot->GetForwardVector(), TargetUp).GetSafeNormal();
	}

	if (DesiredForward.IsNearlyZero())
	{
		DesiredForward = VisualRoot->GetForwardVector();
	}

	// E: EL TRUCO DE LA ESFERA: Rotamos el VisualRoot, NO la esfera física.
	// I: THE SPHERE TRICK: We rotate the VisualRoot, NOT the physical sphere.
	FQuat TargetQuat = FRotationMatrix::MakeFromXZ(DesiredForward, TargetUp).ToQuat();

	// E: Podemos usar interpolación para hacerlo aún más suave, o setearlo directo.
	// I: We can use interpolation to make it even smoother, or set it directly.
	FQuat NewVisualQuat = FMath::QInterpTo(VisualRoot->GetComponentQuat(), TargetQuat, DeltaTime, 15.0f);
	VisualRoot->SetWorldRotation(NewVisualQuat);
}

void ACosmicSpherePlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	// E: Reseteamos la dirección objetivo si el input es nulo.
	// I: Reset target direction if input is null.
	if (MovementVector.IsNearlyZero())
	{
		TargetFacingDirection = FVector::ZeroVector;
		return;
	}

	if (Controller && VisualRoot && CameraComp && SphereComp)
	{
		// E: El movimiento siempre es relativo a la orientación actual de la cámara.
		// I: Movement is always relative to the current camera orientation.
		FVector CameraForward = CameraComp->GetForwardVector();
		FVector CameraRight = CameraComp->GetRightVector();
		FVector UpVector = VisualRoot->GetUpVector();

		// E: Proyectamos los vectores de la cámara sobre el plano del suelo local.
		// I: Project camera vectors onto the local ground plane.
		FVector ForwardOnGround = FVector::VectorPlaneProject(CameraForward, UpVector).GetSafeNormal();
		FVector RightOnGround = FVector::VectorPlaneProject(CameraRight, UpVector).GetSafeNormal();

		// E: Calculamos la dirección final en el espacio del mundo.
		// I: Calculate final direction in world space.
		FVector MoveDirection = (ForwardOnGround * MovementVector.Y) + (RightOnGround * MovementVector.X);
		TargetFacingDirection = MoveDirection.GetSafeNormal();

		// E: Calculamos la fuerza final a aplicar usando nuestra variable parametrizada.
		// I: Calculate the final force to apply using our parameterized variable.
		FVector ForceToApply = TargetFacingDirection * MovementForce;

		// E: Aplicamos fuerza a la ESFERA RAÍZ.
		// I: Apply force to the ROOT SPHERE.
		SphereComp->AddForce(ForceToApply, NAME_None, false);

		// =========================================================================
		// DEBUG VISUAL DE LA FUERZA
		// =========================================================================

		// E: Obtenemos el punto de origen (El centro de nuestra esfera física).
		// I: Get the starting point (The center of our physical sphere).
		FVector StartLoc = SphereComp->GetComponentLocation();

		// E: Para que la flecha no sea gigantesca (350.000 unidades de larga), 
		// usamos la dirección pura y la multiplicamos por una longitud visual fija (ej. 200).
		// I: So the arrow isn't gigantic (350,000 units long), 
		// we use the pure direction and multiply it by a fixed visual length (e.g. 200).
		float VisualArrowLength = 200.0f;
		FVector EndLoc = StartLoc + (TargetFacingDirection * VisualArrowLength);

		// E: Dibujamos la flecha direccional.
		// Parámetros: Mundo, Inicio, Fin, Tamaño Flecha, Color, Persistente, Tiempo de vida, Profundidad, Grosor.
		// I: Draw the directional arrow.
		DrawDebugDirectionalArrow(GetWorld(), StartLoc, EndLoc, 20.0f, FColor::White, false, -1.0f, 0, 4.0f);
	}
}

void ACosmicSpherePlayer::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>() * MouseSensitivity;

	if (SpringArmComp)
	{
		CameraYaw += LookAxisVector.X;
		CameraPitch += LookAxisVector.Y; // Cambia a -= si quieres invertir.
		CameraPitch = FMath::Clamp(CameraPitch, -85.0f, 85.0f);

		SpringArmComp->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	}
}

void ACosmicSpherePlayer::Jump(const FInputActionValue& Value)
{
	if (SphereComp && VisualRoot)
	{
		// E: El impulso de salto se basa en la orientación visual ("Arriba" local).
		// I: Jump impulse is based on visual orientation (local "Up").
		FVector JumpImpulse = VisualRoot->GetUpVector() * 800.0f;
		SphereComp->AddImpulse(JumpImpulse, NAME_None, true); // true = ignorar masa
	}
}