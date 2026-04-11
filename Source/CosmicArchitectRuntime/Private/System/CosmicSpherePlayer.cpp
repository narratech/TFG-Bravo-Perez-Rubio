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
#include "Simulation/CosmicGravitySubsystem.h" 
#include "Planet/CosmicPlanet.h"
#include "DrawDebugHelpers.h"

ACosmicSpherePlayer::ACosmicSpherePlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	// E: Usamos PostPhysics para que la cámara y el modelo se alineen después de que Chaos calcule la colisión.
	// I: We use PostPhysics so the camera and model align after Chaos calculates the collision.
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// =========================================================================
	// CREACIÓN DE COMPONENTES
	// =========================================================================

	// E: 1. Esfera de Colisión y Físicas.
	// I: 1. Collision and Physics Sphere.
	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->InitSphereRadius(40.0f);
	SphereComp->SetCollisionProfileName(TEXT("Pawn"));
	SphereComp->SetSimulatePhysics(true);
	SphereComp->SetEnableGravity(false); // E: Desactivamos gravedad nativa para usar la del plugin. I: Disable native gravity to use the plugin's.
	RootComponent = SphereComp;

	// E: 2. Raíz visual estabilizada (Alineada estrictamente con el planeta).
	// I: 2. Stabilized visual root (Strictly aligned with the planet).
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(RootComponent);

	// E: 3. Brazo de Cámara.
	// I: 3. Camera Spring Arm.
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(VisualRoot);
	SpringArmComp->TargetArmLength = 400.0f;
	SpringArmComp->bUsePawnControlRotation = false;
	SpringArmComp->bEnableCameraLag = true;

	// E: 4. Cámara del Jugador.
	// I: 4. Player Camera.
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = false;

	// E: 5. Pivote para rotar al personaje hacia la dirección de movimiento.
	// I: 5. Pivot to rotate the character towards the movement direction.
	MeshRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MeshRoot"));
	MeshRoot->SetupAttachment(VisualRoot);

	// E: 6. Malla 3D del Jugador.
	// I: 6. Player's 3D Mesh.
	PlayerMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PlayerMesh"));
	PlayerMesh->SetupAttachment(MeshRoot);
	PlayerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f)); // E: Compensa el radio de la esfera. I: Compensates for the sphere's radius.

	// E: 7. Componente Gravitatorio del Plugin Cosmic Architect.
	// I: 7. Cosmic Architect Plugin's Gravity Component.
	GravityComp = CreateDefaultSubobject<UCosmicGravityComponent>(TEXT("GravityComp"));

	// E: Valores iniciales de cámara.
	// I: Initial camera values.
	CameraYaw = 0.0f;
	CameraPitch = -20.0f;
}

void ACosmicSpherePlayer::BeginPlay()
{
	Super::BeginPlay();

	// E: Inicialización del sistema de inputs moderno de Unreal 5.
	// I: Initialization of Unreal 5's modern input system.
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

//void ACosmicSpherePlayer::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//
//	// E: Prevención de errores si faltan componentes clave.
//	// I: Error prevention if key components are missing.
//	if (!GravityComp || !VisualRoot || !MeshRoot) return;
//
//	// =========================================================================
//	// 1. OBTENCIÓN DE GRAVEDAD (GRAVITY FETCHING)
//	// =========================================================================
//
//	FVector GravityDown = GravityComp->CurrentGravityDirection;
//	if (GravityDown.IsNearlyZero()) return;
//
//	FVector TargetUp = -GravityDown;
//
//	// =========================================================================
//	// 2. ALINEACIÓN INSTANTÁNEA (INSTANT ALIGNMENT)
//	// =========================================================================
//
//	// E: Proyectamos el vector frontal en el plano de la gravedad para evitar que el personaje se ladee ("Efecto Torre de Pisa").
//	// I: We project the forward vector on the gravity plane to prevent the character from tilting ("Leaning Tower effect").
//	FVector StableForward = FVector::VectorPlaneProject(VisualRoot->GetForwardVector(), TargetUp).GetSafeNormal();
//	if (StableForward.IsNearlyZero()) {
//		StableForward = FVector::CrossProduct(VisualRoot->GetRightVector(), TargetUp).GetSafeNormal();
//	}
//
//	// E: Aplicamos la rotación estable al VisualRoot (Cámara y contenedor general).
//	// I: Apply the stable rotation to the VisualRoot (Camera and general container).
//	FQuat TargetVisualQuat = FRotationMatrix::MakeFromXZ(StableForward, TargetUp).ToQuat();
//	VisualRoot->SetWorldRotation(TargetVisualQuat);
//
//	// =========================================================================
//	// 3. PARENTESCO DINÁMICO (DYNAMIC PARENTING)
//	// =========================================================================
//
//	HandleDynamicParenting();
//
//	// =========================================================================
//	// 4. ROTACIÓN SUAVE DEL MODELO 3D (SMOOTH MESH ROTATION)
//	// =========================================================================
//
//	FVector MeshDesiredForward;
//	if (!TargetFacingDirection.IsNearlyZero())
//	{
//		// E: Si nos estamos moviendo, queremos mirar hacia esa dirección de forma segura.
//		// I: If we are moving, we want to face that direction safely.
//		MeshDesiredForward = FVector::VectorPlaneProject(TargetFacingDirection, TargetUp).GetSafeNormal();
//	}
//	else
//	{
//		// E: Si estamos quietos, mantenemos la orientación actual proyectada correctamente.
//		// I: If standing still, we keep the current correctly projected orientation.
//		MeshDesiredForward = FVector::VectorPlaneProject(MeshRoot->GetForwardVector(), TargetUp).GetSafeNormal();
//	}
//
//	if (!MeshDesiredForward.IsNearlyZero())
//	{
//		// E: Interpolación suave (QInterpTo) para que el personaje gire la cintura con naturalidad.
//		// I: Smooth interpolation (QInterpTo) so the character twists its waist naturally.
//		FQuat TargetMeshQuat = FRotationMatrix::MakeFromXZ(MeshDesiredForward, TargetUp).ToQuat();
//		MeshRoot->SetWorldRotation(FMath::QInterpTo(MeshRoot->GetComponentQuat(), TargetMeshQuat, DeltaTime, 15.0f));
//	}
//}
void ACosmicSpherePlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!VisualRoot || !MeshRoot || !SphereComp) return;

	// =========================================================================
	// 1. CÁLCULO DE GRAVEDAD LOCAL (LOCAL GRAVITY CALCULATION)
	// =========================================================================

	FVector GravityDown = FVector::DownVector; // Gravedad por defecto
	AActor* NearestPlanet = nullptr;
	float MinDist = TNumericLimits<float>::Max();

	// E: Buscamos el planeta más cercano para saber hacia dónde caer.
	// I: Find the nearest planet to know which way to fall.
	if (UCosmicGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UCosmicGravitySubsystem>())
	{
		for (UCosmicGravityComponent* PlanetComp : Subsystem->GetPlanets())
		{
			if (ACosmicPlanet* PlanetActor = Cast<ACosmicPlanet>(PlanetComp->GetOwner()))
			{
				float DistToSurface = FVector::Dist(GetActorLocation(), PlanetActor->GetActorLocation()) - (PlanetActor->RadiusKm * 100000.0f);
				if (DistToSurface < MinDist)
				{
					MinDist = DistToSurface;
					NearestPlanet = PlanetActor;
				}
			}
		}
	}

	// E: Si hay un planeta cerca, definimos la gravedad hacia su núcleo y la aplicamos.
	// I: If there is a nearby planet, we define gravity towards its core and apply it.
	if (NearestPlanet)
	{
		GravityDown = (NearestPlanet->GetActorLocation() - GetActorLocation()).GetSafeNormal();

		// E: Aplicamos una fuerza de gravedad terrestre constante (980 cm/s^2).
		// I: We apply a constant Earth-like gravity force (980 cm/s^2).
		SphereComp->AddForce(GravityDown * 980.0f, NAME_None, true);
	}

	FVector TargetUp = -GravityDown;

	// =========================================================================
	// 2. ALINEACIÓN INSTANTÁNEA (INSTANT ALIGNMENT)
	// =========================================================================

	FVector StableForward = FVector::VectorPlaneProject(VisualRoot->GetForwardVector(), TargetUp).GetSafeNormal();
	if (StableForward.IsNearlyZero()) {
		StableForward = FVector::CrossProduct(VisualRoot->GetRightVector(), TargetUp).GetSafeNormal();
	}

	FQuat TargetVisualQuat = FRotationMatrix::MakeFromXZ(StableForward, TargetUp).ToQuat();
	VisualRoot->SetWorldRotation(TargetVisualQuat);

	// =========================================================================
	// 3. PARENTESCO DINÁMICO (DYNAMIC PARENTING)
	// =========================================================================

	// E: Opcional: Descoméntalo si tus planetas se mueven por el espacio.
	// HandleDynamicParenting();

	// =========================================================================
	// 4. ROTACIÓN SUAVE DEL MODELO 3D (SMOOTH MESH ROTATION)
	// =========================================================================

	FVector MeshDesiredForward = TargetFacingDirection.IsNearlyZero() ?
		FVector::VectorPlaneProject(MeshRoot->GetForwardVector(), TargetUp).GetSafeNormal() :
		FVector::VectorPlaneProject(TargetFacingDirection, TargetUp).GetSafeNormal();

	if (!MeshDesiredForward.IsNearlyZero())
	{
		FQuat TargetMeshQuat = FRotationMatrix::MakeFromXZ(MeshDesiredForward, TargetUp).ToQuat();
		MeshRoot->SetWorldRotation(FMath::QInterpTo(MeshRoot->GetComponentQuat(), TargetMeshQuat, DeltaTime, 15.0f));
	}
}

void ACosmicSpherePlayer::HandleDynamicParenting()
{
	AActor* NearestPlanet = nullptr;
	float MinDist = TNumericLimits<float>::Max();

	// E: Accedemos al subsistema de gravedad para encontrar planetas cercanos de forma optimizada.
	// I: We access the gravity subsystem to find nearby planets in an optimized way.
	if (UCosmicGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UCosmicGravitySubsystem>())
	{
		for (UCosmicGravityComponent* PlanetComp : Subsystem->GetPlanets())
		{
			if (PlanetComp && PlanetComp->GetOwner() && PlanetComp->GetOwner() != this)
			{
				// E: Verificamos si el dueño de la gravedad es realmente un ACosmicPlanet.
				// I: We verify if the gravity owner is actually an ACosmicPlanet.
				if (ACosmicPlanet* PlanetActor = Cast<ACosmicPlanet>(PlanetComp->GetOwner()))
				{
					float Dist = FVector::Dist(GetActorLocation(), PlanetActor->GetActorLocation());

					// E: Convertimos los Km a Centímetros (Unidades de Unreal) multiplicando por 100,000.
					// I: We convert Km to Centimeters (Unreal Units) by multiplying by 100,000.
					float RealRadiusCm = PlanetActor->RadiusKm * 100000.0f;

					// E: Calculamos la distancia exacta a la superficie matemática del planeta.
					// I: Calculate the exact distance to the mathematical surface of the planet.
					float DistToSurface = Dist - RealRadiusCm;

					if (DistToSurface < MinDist)
					{
						MinDist = DistToSurface;
						NearestPlanet = PlanetActor;
					}
				}
			}
		}
	}

	// E: Si estamos cerca de la superficie, nos "pegamos" al planeta para heredar su movimiento en el espacio.
	// I: If close to the surface, we "stick" to the planet to inherit its movement in space.
	if (NearestPlanet && MinDist <= ParentingDistanceThreshold)
	{
		if (CurrentParentPlanet != NearestPlanet)
		{
			CurrentParentPlanet = NearestPlanet;
			FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false);
			AttachToActor(NearestPlanet, AttachRules);
		}
	}
	else if (CurrentParentPlanet != nullptr)
	{
		// E: Si saltamos o volamos lejos, nos desvinculamos para volver a ser independientes.
		// I: If we jump or fly away, we detach to become independent again.
		FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false);
		DetachFromActor(DetachRules);
		CurrentParentPlanet = nullptr;
	}
}

void ACosmicSpherePlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	// E: Si no hay input, reseteamos la dirección objetivo para que el modelo deje de intentar rotar.
	// I: If no input, reset the target direction so the model stops trying to rotate.
	if (MovementVector.IsNearlyZero()) { TargetFacingDirection = FVector::ZeroVector; return; }

	if (VisualRoot && CameraComp && SphereComp)
	{
		FVector UpVector = VisualRoot->GetUpVector();

		// E: Calculamos hacia dónde mira la cámara, ignorando si miramos hacia el cielo o el suelo.
		// I: Calculate where the camera is looking, ignoring if we look at the sky or the ground.
		FVector ForwardOnGround = FVector::VectorPlaneProject(CameraComp->GetForwardVector(), UpVector).GetSafeNormal();
		FVector RightOnGround = FVector::VectorPlaneProject(CameraComp->GetRightVector(), UpVector).GetSafeNormal();

		// E: Creamos el vector direccional y aplicamos fuerza a la esfera para rodar.
		// I: Create the directional vector and apply force to the sphere to roll.
		TargetFacingDirection = (ForwardOnGround * MovementVector.Y) + (RightOnGround * MovementVector.X);
		SphereComp->AddForce(TargetFacingDirection * MovementForce);
	}
}

void ACosmicSpherePlayer::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>() * MouseSensitivity;

	if (SpringArmComp)
	{
		// E: Actualizamos las rotaciones puras del brazo de la cámara.
		// I: Update the pure rotations of the camera spring arm.
		CameraYaw += LookAxisVector.X;
		CameraPitch = FMath::Clamp(CameraPitch + LookAxisVector.Y, -85.0f, 85.0f); // Evitamos dar volteretas. I: Prevent somersaults.

		SpringArmComp->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	}
}

void ACosmicSpherePlayer::Jump(const FInputActionValue& Value)
{
	if (SphereComp && VisualRoot)
	{
		// E: Impulso seco en el eje Z local (que gracias a nuestro Tick, siempre es contrario a la gravedad).
		// I: Sharp impulse on the local Z axis (which thanks to our Tick, is always opposite to gravity).
		SphereComp->AddImpulse(VisualRoot->GetUpVector() * 800.0f, NAME_None, true);
	}
}