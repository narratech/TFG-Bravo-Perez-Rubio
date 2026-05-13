// Copyright Epic Games, Inc. All Rights Reserved.

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

	// Ejecuta Tick después de la simulación física
	// para alinear correctamente cámara y visuales.
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	// Asigna automáticamente el control
	// al jugador local principal.
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// =========================================================================
	// CREACIÓN DE COMPONENTES
	// =========================================================================

	// Cápsula principal utilizada para:
	// - Colisiones
	// - Movimiento físico
	// - Interacción gravitacional
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	RootComponent = CapsuleComp;
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);

	// Activa simulación física personalizada.
	CapsuleComp->SetSimulatePhysics(true);

	// Desactiva gravedad estándar de Unreal
	// para utilizar gravedad esférica personalizada.
	CapsuleComp->SetEnableGravity(false);

	CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));

	// Nodo visual alineado estrictamente
	// con la normal gravitacional local.
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);

	// Sistema de cámara orbital desacoplada.
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(VisualRoot);
	SpringArmComp->TargetArmLength = 400.0f;
	SpringArmComp->bUsePawnControlRotation = false;
	SpringArmComp->bEnableCameraLag = true;

	// Cámara principal controlada por el jugador.
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	// Nodo utilizado para orientar visualmente
	// el personaje hacia la dirección de movimiento.
	MeshRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshRoot"));
	MeshRoot->SetupAttachment(VisualRoot);

	// Malla esquelética principal del jugador.
	PlayerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PlayerMesh"));
	PlayerMesh->SetupAttachment(MeshRoot);

	// Compensa visualmente la altura
	// respecto a la cápsula física.
	PlayerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));

	// Componente responsable de calcular
	// gravedad planetaria personalizada.
	GravityComp = CreateDefaultSubobject<UCosmicGravityComponent>(TEXT("GravityComp"));

	// Estado inicial de orientación de cámara.
	CameraYaw = 0.0f;
	CameraPitch = -20.0f;
}

void ACosmicSpherePlayer::BeginPlay()
{
	Super::BeginPlay();

	// Inicializa el sistema moderno
	// Enhanced Input de Unreal Engine 5.
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

	// Sincroniza la masa física con el
	// componente gravitacional personalizado.
	CapsuleComp->SetMassOverrideInKg(NAME_None, GravityComp->Mass, true);
}

void ACosmicSpherePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Vincula acciones Enhanced Input
	// con lógica C++ runtime.
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_PlayerMove) { EnhancedInputComponent->BindAction(IA_PlayerMove, ETriggerEvent::Triggered, this, &ACosmicSpherePlayer::Move); }
		if (IA_PlayerMove) { EnhancedInputComponent->BindAction(IA_PlayerMove, ETriggerEvent::Completed, this, &ACosmicSpherePlayer::Move); }
		if (IA_PlayerLook) { EnhancedInputComponent->BindAction(IA_PlayerLook, ETriggerEvent::Triggered, this, &ACosmicSpherePlayer::Look); }
		if (IA_PlayerJump) { EnhancedInputComponent->BindAction(IA_PlayerJump, ETriggerEvent::Started, this, &ACosmicSpherePlayer::Jump); }
	}
}

//void ACosmicSpherePlayer::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//	// Validación de seguridad para evitar
//	// acceso a componentes inválidos.
//	if (!GravityComp || !VisualRoot || !MeshRoot) return;
//
//	// =========================================================================
//	// 1. OBTENCIÓN DE GRAVEDAD
//	// =========================================================================
//
//	FVector GravityDown = GravityComp->CurrentGravityDirection;
//	if (GravityDown.IsNearlyZero()) return;
//
//	FVector TargetUp = -GravityDown;
//
//	// =========================================================================
//	// 2. ALINEACIÓN INSTANTÁNEA
//	// =========================================================================
//
//	// Proyecta el vector frontal sobre el plano
//	// gravitacional para evitar inclinaciones laterales.
//	FVector StableForward = FVector::VectorPlaneProject(VisualRoot->GetForwardVector(), TargetUp).GetSafeNormal();
//	if (StableForward.IsNearlyZero()) {
//		StableForward = FVector::CrossProduct(VisualRoot->GetRightVector(), TargetUp).GetSafeNormal();
//	}
//
//	// Alineación gravitacional del contenedor visual.
//	FQuat TargetVisualQuat = FRotationMatrix::MakeFromXZ(StableForward, TargetUp).ToQuat();
//	VisualRoot->SetWorldRotation(TargetVisualQuat);
//
//	// =========================================================================
//	// 3. PARENTESCO DINÁMICO
//	// =========================================================================
//
//	HandleDynamicParenting();
//
//	// =========================================================================
//	// 4. ROTACIÓN SUAVE DEL MODELO 3D
//	// =========================================================================
//
//	FVector MeshDesiredForward;
//	if (!TargetFacingDirection.IsNearlyZero())
//	{
//		// Orienta el personaje hacia
//		// la dirección de desplazamiento.
//		MeshDesiredForward = FVector::VectorPlaneProject(TargetFacingDirection, TargetUp).GetSafeNormal();
//	}
//	else
//	{
//		// Mantiene orientación estable
//		// mientras el jugador está quieto.
//		MeshDesiredForward = FVector::VectorPlaneProject(MeshRoot->GetForwardVector(), TargetUp).GetSafeNormal();
//	}
//
//	if (!MeshDesiredForward.IsNearlyZero())
//	{
//		// Interpolación suave para obtener
//		// rotación visual natural.
//		FQuat TargetMeshQuat = FRotationMatrix::MakeFromXZ(MeshDesiredForward, TargetUp).ToQuat();
//		MeshRoot->SetWorldRotation(FMath::QInterpTo(MeshRoot->GetComponentQuat(), TargetMeshQuat, DeltaTime, 15.0f));
//	}
//}

void ACosmicSpherePlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Validación de seguridad para evitar
	// acceso a componentes inválidos.
	if (!VisualRoot || !MeshRoot || !CapsuleComp) return;

	// Obtiene gravedad personalizada activa
	// o utiliza gravedad fallback por defecto.
	FVector GravityDown = GravityComp ? GravityComp->CurrentGravityDirection : FVector::DownVector * GravityAcceleration * 100;

	// Aplicación física de gravedad.
	CapsuleComp->AddForce(GravityDown, NAME_None, true);

	// Vector Up gravitacional local.
	FVector TargetUp = -GravityDown.GetSafeNormal();

	// Proyecta el forward sobre el plano local
	// para evitar inclinaciones laterales.
	FVector StableForward = FVector::VectorPlaneProject(VisualRoot->GetForwardVector(), TargetUp).GetSafeNormal();

	if (StableForward.IsNearlyZero()) {
		StableForward = FVector::CrossProduct(VisualRoot->GetRightVector(), TargetUp).GetSafeNormal();
	}

	// Alineación gravitacional del contenedor visual.
	FQuat TargetVisualQuat = FRotationMatrix::MakeFromXZ(StableForward, TargetUp).ToQuat();
	VisualRoot->SetWorldRotation(TargetVisualQuat);

	// Dirección objetivo utilizada para orientar
	// visualmente el personaje.
	FVector MeshDesiredForward = TargetFacingDirection.IsNearlyZero() ?
		FVector::VectorPlaneProject(MeshRoot->GetForwardVector(), TargetUp).GetSafeNormal() :
		FVector::VectorPlaneProject(TargetFacingDirection, TargetUp).GetSafeNormal();

	if (!MeshDesiredForward.IsNearlyZero())
	{
		// Interpolación suave de orientación visual.
		FQuat TargetMeshQuat = FRotationMatrix::MakeFromXZ(MeshDesiredForward, TargetUp).ToQuat();
		MeshRoot->SetWorldRotation(FMath::QInterpTo(MeshRoot->GetComponentQuat(), TargetMeshQuat, DeltaTime, 15.0f));
	}

	// Actualización del estado de suelo.
	bIsGroundedState = IsGrounded();

	// Velocidad vertical relativa respecto
	// al eje gravitacional local.
	VerticalVelocity = FVector::DotProduct(CapsuleComp->GetComponentVelocity(), VisualRoot->GetUpVector());
}

void ACosmicSpherePlayer::HandleDynamicParenting()
{
	AActor* NearestPlanet = nullptr;
	float MinDist = TNumericLimits<float>::Max();

	// Variable auxiliar utilizada para debugging.
	int32 CheckedPlanetsCount = 0;

	if (UCosmicGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UCosmicGravitySubsystem>())
	{
		for (UCosmicGravityComponent* PlanetComp : Subsystem->GetPlanets())
		{
			if (PlanetComp && PlanetComp->GetOwner() && PlanetComp->GetOwner() != this)
			{
				if (ACosmicPlanet* PlanetActor = Cast<ACosmicPlanet>(PlanetComp->GetOwner()))
				{
					// Contador de planetas válidos evaluados.
					CheckedPlanetsCount++;

					float Dist = FVector::Dist(GetActorLocation(), PlanetActor->GetActorLocation());
					float RealRadiusCm = PlanetActor->RadiusKm * 100000.0f;
					float DistToSurface = Dist - RealRadiusCm;

					// Selecciona el planeta más cercano
					// respecto a su superficie real.
					if (DistToSurface < MinDist)
					{
						MinDist = DistToSurface;
						NearestPlanet = PlanetActor;
					}
				}
			}
		}
	}

	// =========================================================================
	// ZONA DE DEBUG
	// =========================================================================

	// Debug visual runtime para:
	// - Número de planetas evaluados
	// - Planeta más cercano
	// - Distancia a superficie
	// - Estado de parenting dinámico

	// =========================================================================
}

void ACosmicSpherePlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	// Reinicia la orientación objetivo cuando
	// no existe movimiento activo.
	if (MovementVector.IsNearlyZero()) { TargetFacingDirection = FVector::ZeroVector; return; }

	if (VisualRoot && CameraComp && CapsuleComp)
	{
		FVector UpVector = VisualRoot->GetUpVector();

		// Proyección horizontal de orientación
		// de cámara sobre la superficie local.
		FVector ForwardOnGround = FVector::VectorPlaneProject(CameraComp->GetForwardVector(), UpVector).GetSafeNormal();
		FVector RightOnGround = FVector::VectorPlaneProject(CameraComp->GetRightVector(), UpVector).GetSafeNormal();

		// Construye dirección final de movimiento
		// relativa a la orientación de cámara.
		TargetFacingDirection = ((ForwardOnGround * MovementVector.Y) + (RightOnGround * MovementVector.X)).GetSafeNormal();
		// Aplicación física de movimiento.
		CapsuleComp->AddForce(TargetFacingDirection * MovementForce);
	}
}

void ACosmicSpherePlayer::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>() * MouseSensitivity;

	if (SpringArmComp)
	{
		// Actualización acumulativa de rotación
		// orbital de cámara.
		CameraYaw += LookAxisVector.X;

		// Limita rotación vertical para evitar
		// inversión completa de cámara.
		CameraPitch = FMath::Clamp(CameraPitch + LookAxisVector.Y, -85.0f, 85.0f);

		SpringArmComp->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	}
}

void ACosmicSpherePlayer::Jump(const FInputActionValue& Value)
{
	// El salto únicamente puede ejecutarse
	// mientras el jugador está en el suelo.
	if (!IsGrounded()) return;

	if (CapsuleComp && VisualRoot)
	{
		// Impulso aplicado sobre el eje Up
		// gravitacional local.
		CapsuleComp->AddImpulse(VisualRoot->GetUpVector() * 800.0f, NAME_None, true);
	}
}

bool ACosmicSpherePlayer::IsGrounded() const
{
	if (!CapsuleComp || !VisualRoot) return false;

	// Punto inicial del raycast de suelo.
	FVector Start = GetActorLocation();

	// Dirección gravitacional descendente local.
	FVector Down = -VisualRoot->GetUpVector();

	// Longitud del raycast utilizada para
	// detectar superficies cercanas.
	float HalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();
	FVector End = Start + Down * (HalfHeight + 15.0f);

	FHitResult Hit;
	FCollisionQueryParams Params;

	// Ignora colisiones contra el propio actor.
	Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		Params
	);

	// Debug visual opcional del raycast de suelo.
	// DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, -1.0f, 0, 2.0f);

	return bHit;
}