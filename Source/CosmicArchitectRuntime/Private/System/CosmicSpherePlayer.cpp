// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "System/CosmicSpherePlayer.h"
#include "Terrain/CosmicCollisionTargetComponent.h"
#include "Terrain/CosmicCollisionComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Simulation/CosmicGravityComponent.h"
#include "Simulation/CosmicGravitySubsystem.h"
#include "Engine/World.h"

ACosmicSpherePlayer::ACosmicSpherePlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	bReplicates = true;
	SetReplicateMovement(true);

	// Disable controller rotation so the character freely turns toward movement direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Capsule configuration
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (Capsule)
	{
		Capsule->InitCapsuleSize(40.0f, 90.0f);
		Capsule->SetCollisionProfileName(TEXT("Pawn"));
		Capsule->SetSimulatePhysics(false);
		Capsule->SetEnableGravity(false);
	}

	// Default orientation and ground alignment for character skeletal mesh
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		MeshComp->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	}

	// Visual root strictly aligned with local gravitational normal (TargetUp)
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);
	VisualRoot->SetUsingAbsoluteRotation(true);

	// Decoupled orbital camera system attached to VisualRoot
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(VisualRoot);
	SpringArmComp->TargetArmLength = 400.0f;
	SpringArmComp->bUsePawnControlRotation = false;
	SpringArmComp->bInheritPitch = true;
	SpringArmComp->bInheritYaw = true;
	SpringArmComp->bInheritRoll = true;
	SpringArmComp->bEnableCameraLag = true;
	SpringArmComp->CameraLagSpeed = 10.0f;
	SpringArmComp->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	// Gravity component
	GravityComp = CreateDefaultSubobject<UCosmicGravityComponent>(TEXT("GravityComp"));

	// Collision target component
	CollisionTargetComp = CreateDefaultSubobject<UCosmicCollisionTargetComponent>(TEXT("CollisionTargetComp"));
	CollisionTargetComp->Priority = 1.0f;

	// Character Movement configuration for planetary navigation
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = BaseWalkSpeed;
		MoveComp->JumpZVelocity = BaseJumpVelocity;
		MoveComp->AirControl = 0.35f;
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		MoveComp->bConstrainToPlane = false;
		MoveComp->bRunPhysicsWithNoController = true;

		// Multiplayer client authority settings for local terrain collision
		MoveComp->bServerAcceptClientAuthoritativePosition = true;
		MoveComp->bIgnoreClientMovementErrorChecksAndCorrection = true;
	}

	// Initial camera orientation
	CameraYaw = 0.0f;
	CameraPitch = -20.0f;
	SpringArmComp->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
}

void ACosmicSpherePlayer::BeginPlay()
{
	Super::BeginPlay();

	// Ensure controller rotation doesn't lock character yaw, even if enabled in BP details
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->bUseControllerDesiredRotation = false;
		MoveComp->JumpZVelocity = BaseJumpVelocity;
		MoveComp->MaxWalkSpeed = BaseWalkSpeed;
	}
}

void ACosmicSpherePlayer::OnRep_ReplicatedBasedMovement()
{
	if (!IsReplicatingMovement())
	{
		return;
	}

	if (GetLocalRole() != ROLE_SimulatedProxy)
	{
		return;
	}

	// Skip base updates while playing root motion, it is handled inside of OnRep_RootMotion
	if (IsPlayingNetworkedRootMotionMontage())
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	MoveComp->bNetworkUpdateReceived = true;
	FGuardValue_Bitfield(bInBaseReplication, true);

	const FBasedMovementInfo& RepBasedMovement = GetReplicatedBasedMovement();
	const bool bBaseChanged = (BasedMovement.MovementBase != RepBasedMovement.MovementBase || BasedMovement.BoneName != RepBasedMovement.BoneName);
	if (bBaseChanged)
	{
		// Even though we will copy the replicated based movement info, we need to use SetBase() to set up tick dependencies and trigger notifications.
		SetBase(RepBasedMovement.MovementBase, RepBasedMovement.BoneName);
	}

	// Make sure to use the values of relative location/rotation etc from the server.
	BasedMovement = RepBasedMovement;

	if (RepBasedMovement.HasRelativeLocation())
	{
		// Update transform relative to movement base
		const FVector OldLocation = GetActorLocation();
		const FQuat OldRotation = GetActorQuat();
		MovementBaseUtility::GetMovementBaseTransform(RepBasedMovement.MovementBase, RepBasedMovement.BoneName, MoveComp->OldBaseLocation, MoveComp->OldBaseQuat);
		const FTransform BaseTransform(MoveComp->OldBaseQuat, MoveComp->OldBaseLocation);
		const FVector NewLocation = BaseTransform.TransformPositionNoScale(RepBasedMovement.Location);
		FRotator NewRotation;

		if (RepBasedMovement.HasRelativeRotation())
		{
			// Relative location, relative rotation
			NewRotation = (FRotationMatrix(RepBasedMovement.Rotation) * FQuatRotationMatrix(MoveComp->OldBaseQuat)).Rotator();
			
			if (MoveComp->ShouldRemainVertical())
			{
				if (MoveComp->HasCustomGravity())
				{
					// Rotate into gravity space, zero Pitch and Roll relative to planet gravity, and rotate back to world space
					FRotator GravityRelativeDesiredRotation = (MoveComp->GetGravityToWorldTransform() * NewRotation.Quaternion()).Rotator();
					GravityRelativeDesiredRotation.Pitch = 0.f;
					GravityRelativeDesiredRotation.Yaw = FRotator::NormalizeAxis(GravityRelativeDesiredRotation.Yaw);
					GravityRelativeDesiredRotation.Roll = 0.f;
					NewRotation = (MoveComp->GetWorldToGravityTransform() * GravityRelativeDesiredRotation.Quaternion()).Rotator();
				}
				else
				{
					NewRotation.Pitch = 0.f;
					NewRotation.Roll = 0.f;
				}
			}
		}
		else
		{
			// Relative location, absolute rotation
			NewRotation = RepBasedMovement.Rotation;
		}

		// When position or base changes, movement mode will need to be updated. This assumes rotation changes don't affect that.
		MoveComp->bJustTeleported |= (bBaseChanged || NewLocation != OldLocation);
		MoveComp->bNetworkSmoothingComplete = false;
		MoveComp->SmoothCorrection(OldLocation, OldRotation, NewLocation, NewRotation.Quaternion());
		OnUpdateSimulatedPosition(OldLocation, OldRotation);
	}
}


float ACosmicSpherePlayer::GetCurrentGravityMagnitude() const
{
	if (GravityComp && !GravityComp->CurrentGravityDirection.IsNearlyZero())
	{
		return static_cast<float>(GravityComp->CurrentGravityDirection.Length());
	}
	return 980.0f;
}

void ACosmicSpherePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_PlayerMove)
		{
			EnhancedInputComponent->BindAction(IA_PlayerMove, ETriggerEvent::Triggered, this, &ACosmicSpherePlayer::Move);
		}
		if (IA_PlayerLook)
		{
			EnhancedInputComponent->BindAction(IA_PlayerLook, ETriggerEvent::Triggered, this, &ACosmicSpherePlayer::Look);
		}
		if (IA_PlayerJump)
		{
			EnhancedInputComponent->BindAction(IA_PlayerJump, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(IA_PlayerJump, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
	}
}

void ACosmicSpherePlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Determine local gravitational down direction and magnitude from CosmicGravityComponent
	FVector GravityDown = FVector::DownVector;
	float GravityMagnitude = 980.0f;

	if (GravityComp && !GravityComp->CurrentGravityDirection.IsNearlyZero())
	{
		GravityMagnitude = static_cast<float>(GravityComp->CurrentGravityDirection.Length());
		GravityDown = (GravityComp->CurrentGravityDirection / GravityMagnitude);
	}

	// Feed gravity direction and planetary gravity scale to CharacterMovementComponent 
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetGravityDirection(GravityDown);

		const float DefaultGravityZ = (GetWorld() && FMath::Abs(GetWorld()->GetDefaultGravityZ()) > 0.0f)
			? FMath::Abs(GetWorld()->GetDefaultGravityZ())
			: 980.0f;

		MoveComp->GravityScale = GravityMagnitude / DefaultGravityZ;
	}

	// Update VisualRoot orientation to strictly align with planet normal 
	if (VisualRoot)
	{
		FVector TargetUp = -GravityDown;

		FVector StableForward = FVector::VectorPlaneProject(VisualRoot->GetForwardVector(), TargetUp).GetSafeNormal();
		if (StableForward.IsNearlyZero())
		{
			StableForward = FVector::CrossProduct(VisualRoot->GetRightVector(), TargetUp).GetSafeNormal();
			if (StableForward.IsNearlyZero())
			{
				StableForward = FVector::VectorPlaneProject(FVector::ForwardVector, TargetUp).GetSafeNormal();
				if (StableForward.IsNearlyZero())
				{
					StableForward = FVector::VectorPlaneProject(FVector::RightVector, TargetUp).GetSafeNormal();
				}
			}
		}

		if (!StableForward.IsNearlyZero())
		{
			FQuat TargetVisualQuat = FRotationMatrix::MakeFromXZ(StableForward, TargetUp).ToQuat();
			VisualRoot->SetWorldRotation(TargetVisualQuat);
		}
	}

	// Update grounded state and vertical velocity for animation blueprint
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		bIsGroundedState = MoveComp->IsMovingOnGround();
		VerticalVelocity = static_cast<float>(MoveComp->GetGravitySpaceZ(GetVelocity()));
	}
	else
	{
		bIsGroundedState = false;
		VerticalVelocity = 0.0f;
	}
}

void ACosmicSpherePlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.IsNearlyZero())
	{
		return;
	}

	if (VisualRoot && CameraComp)
	{
		FVector UpVector = VisualRoot->GetUpVector();

		// Horizontal projection of camera orientation onto local planet surface
		FVector ForwardOnGround = FVector::VectorPlaneProject(CameraComp->GetForwardVector(), UpVector).GetSafeNormal();
		FVector RightOnGround = FVector::VectorPlaneProject(CameraComp->GetRightVector(), UpVector).GetSafeNormal();

		// Builds final movement direction relative to camera orientation
		FVector MoveDirection = ((ForwardOnGround * MovementVector.Y) + (RightOnGround * MovementVector.X)).GetSafeNormal();

		if (!MoveDirection.IsNearlyZero())
		{
			AddMovementInput(MoveDirection, 1.0f);
		}
	}
}

void ACosmicSpherePlayer::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>() * MouseSensitivity;

	if (SpringArmComp)
	{
		// Cumulative update of camera orbital rotation
		CameraYaw += LookAxisVector.X;

		// Clamps vertical rotation to avoid camera flipping
		CameraPitch = FMath::Clamp(CameraPitch + LookAxisVector.Y, -85.0f, 85.0f);

		SpringArmComp->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	}
}