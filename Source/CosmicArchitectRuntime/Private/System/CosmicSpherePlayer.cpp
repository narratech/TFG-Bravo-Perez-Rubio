// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "System/CosmicSpherePlayer.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Simulation/CosmicGravityComponent.h"
#include "Simulation/CosmicGravitySubsystem.h" 
#include "Planet/CosmicPlanet.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

ACosmicSpherePlayer::ACosmicSpherePlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	// Runs Tick after physics simulation
	// to properly align camera and visuals.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	// Automatically assigns control 
	// to the primary local player.
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// COMPONENT CREATION

	// Main capsule used for:
	// - Collisions
	// - Physics movement
	// - Gravitational interaction
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	RootComponent = CapsuleComp;
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);

	// Enables custom physics simulation.
	CapsuleComp->SetSimulatePhysics(true);

	// Disables standard Unreal gravity
	// to use custom spherical gravity.
	CapsuleComp->SetEnableGravity(false);

	CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));

	// Visual node strictly aligned
	// with local gravitational normal.
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);

	// Decoupled orbital camera system.
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(VisualRoot);
	SpringArmComp->TargetArmLength = 400.0f;
	SpringArmComp->bUsePawnControlRotation = false;
	SpringArmComp->bEnableCameraLag = true;

	// Main player-controlled camera.
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	// Node used to visually orient
	// the character toward movement direction.
	MeshRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshRoot"));
	MeshRoot->SetupAttachment(VisualRoot);

	// Main skeletal mesh of the player.
	PlayerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PlayerMesh"));
	PlayerMesh->SetupAttachment(MeshRoot);

	// Visually offsets height
	// relative to physics capsule.
	PlayerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));

	// Component responsible for calculating
	// custom planetary gravity.
	GravityComp = CreateDefaultSubobject<UCosmicGravityComponent>(TEXT("GravityComp"));

	// Initial camera orientation state.
	CameraYaw = 0.0f;
	CameraPitch = -20.0f;
}

void ACosmicSpherePlayer::BeginPlay()
{
	Super::BeginPlay();

	// Initializes Unreal Engine 5's
	// modern Enhanced Input system.
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

	// Synchronizes physics mass with
	// custom gravitational component.
	CapsuleComp->SetMassOverrideInKg(NAME_None, GravityComp->Mass, true);
}

void ACosmicSpherePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Binds Enhanced Input actions
	// to runtime C++ logic.
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

	// Safety validation to avoid
	// accessing invalid components.
	if (!VisualRoot || !MeshRoot || !CapsuleComp) return;

	// Gets active custom gravity
	// or uses default fallback gravity.
	FVector GravityDown = GravityComp ? GravityComp->CurrentGravityDirection : FVector::DownVector * GravityAcceleration * 100;

	// Gravitational physics application.
	CapsuleComp->AddForce(GravityDown, NAME_None, true);

	// Local gravitational Up vector.
	FVector TargetUp = -GravityDown.GetSafeNormal();

	// Projects forward onto local plane
	// to avoid lateral tilting.
	FVector StableForward = FVector::VectorPlaneProject(VisualRoot->GetForwardVector(), TargetUp).GetSafeNormal();

	if (StableForward.IsNearlyZero()) {
		StableForward = FVector::CrossProduct(VisualRoot->GetRightVector(), TargetUp).GetSafeNormal();
	}

	// Gravitational alignment of visual container.
	FQuat TargetVisualQuat = FRotationMatrix::MakeFromXZ(StableForward, TargetUp).ToQuat();
	VisualRoot->SetWorldRotation(TargetVisualQuat);

	// Target direction used to visually
	// orient the character.
	FVector MeshDesiredForward = TargetFacingDirection.IsNearlyZero() ?
		FVector::VectorPlaneProject(MeshRoot->GetForwardVector(), TargetUp).GetSafeNormal() :
		FVector::VectorPlaneProject(TargetFacingDirection, TargetUp).GetSafeNormal();

	if (!MeshDesiredForward.IsNearlyZero())
	{
		// Smooth interpolation of visual orientation.
		FQuat TargetMeshQuat = FRotationMatrix::MakeFromXZ(MeshDesiredForward, TargetUp).ToQuat();
		MeshRoot->SetWorldRotation(FMath::QInterpTo(MeshRoot->GetComponentQuat(), TargetMeshQuat, DeltaTime, 15.0f));
	}

	// Ground state update.
	bIsGroundedState = IsGrounded();

	// Relative vertical velocity with respect
	// to local gravitational axis.
	VerticalVelocity = FVector::DotProduct(CapsuleComp->GetComponentVelocity(), VisualRoot->GetUpVector());
}

void ACosmicSpherePlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	// Resets target orientation when
	// there is no active movement.
	if (MovementVector.IsNearlyZero()) { TargetFacingDirection = FVector::ZeroVector; return; }

	if (VisualRoot && CameraComp && CapsuleComp)
	{
		FVector UpVector = VisualRoot->GetUpVector();

		// Horizontal projection of camera
		// orientation onto local surface.
		FVector ForwardOnGround = FVector::VectorPlaneProject(CameraComp->GetForwardVector(), UpVector).GetSafeNormal();
		FVector RightOnGround = FVector::VectorPlaneProject(CameraComp->GetRightVector(), UpVector).GetSafeNormal();

		// Builds final movement direction
		// relative to camera orientation.
		TargetFacingDirection = ((ForwardOnGround * MovementVector.Y) + (RightOnGround * MovementVector.X)).GetSafeNormal();
		// Physics application of movement.
		CapsuleComp->AddForce(TargetFacingDirection * MovementForce);
	}
}

void ACosmicSpherePlayer::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>() * MouseSensitivity;

	if (SpringArmComp)
	{
		// Cumulative update of camera
		// orbital rotation.
		CameraYaw += LookAxisVector.X;

		// Clamps vertical rotation to avoid
		// complete camera flipping.
		CameraPitch = FMath::Clamp(CameraPitch + LookAxisVector.Y, -85.0f, 85.0f);

		SpringArmComp->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	}
}

void ACosmicSpherePlayer::Jump(const FInputActionValue& Value)
{
	// Jump can only be executed
	// while player is grounded.
	if (!IsGrounded()) return;

	if (CapsuleComp && VisualRoot)
	{
		// Impulse applied along local
		// gravitational Up axis.
		CapsuleComp->AddImpulse(VisualRoot->GetUpVector() * 800.0f, NAME_None, true);
	}
}

bool ACosmicSpherePlayer::IsGrounded() const
{
	if (!CapsuleComp || !VisualRoot) return false;

	// Starting point of ground raycast.
	FVector Start = GetActorLocation();

	// Local downward gravitational direction.
	FVector Down = -VisualRoot->GetUpVector();

	// Raycast length used to
	// detect nearby surfaces.
	float HalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();
	FVector End = Start + Down * (HalfHeight + 15.0f);

	FHitResult Hit;
	FCollisionQueryParams Params;

	// Ignores collisions against the actor itself.
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	// Optional visual debug of ground raycast.
	// DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, -1.0f, 0, 2.0f);

	return bHit;
}