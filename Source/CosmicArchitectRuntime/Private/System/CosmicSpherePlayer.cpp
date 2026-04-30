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

	// E: Usamos PostPhysics para que la cámara y el modelo se alineen después de que Chaos calcule la colisión.
	// I: We use PostPhysics so the camera and model align after Chaos calculates the collision.
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// =========================================================================
	// CREACIÓN DE COMPONENTES
	// =========================================================================

	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	RootComponent = CapsuleComp;
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);

	// E: Habilitamos las físicas y deshabilitamos la gravedad por defecto de Unreal (Z- constante).
	// I: Enable physics and disable Unreal's default gravity (constant Z-).
	CapsuleComp->SetSimulatePhysics(true);
	CapsuleComp->SetEnableGravity(false);
	CapsuleComp->SetCollisionProfileName(TEXT("Pawn"));

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

	if (!VisualRoot || !MeshRoot || !CapsuleComp) return;

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
		CapsuleComp->AddForce(GravityDown * 980.0f, NAME_None, true);
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

	HandleDynamicParenting();

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
	int32 CheckedPlanetsCount = 0; // E: Variable temporal para Debug. I: Temporary variable for Debug.

	if (UCosmicGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UCosmicGravitySubsystem>())
	{
		for (UCosmicGravityComponent* PlanetComp : Subsystem->GetPlanets())
		{
			if (PlanetComp && PlanetComp->GetOwner() && PlanetComp->GetOwner() != this)
			{
				if (ACosmicPlanet* PlanetActor = Cast<ACosmicPlanet>(PlanetComp->GetOwner()))
				{
					CheckedPlanetsCount++; // Contamos cuántos planetas reales estamos evaluando

					float Dist = FVector::Dist(GetActorLocation(), PlanetActor->GetActorLocation());
					float RealRadiusCm = PlanetActor->RadiusKm * 100000.0f;
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

	// =========================================================================
	// ZONA DE DEBUG (MUESTRA LA INFORMACIÓN EN PANTALLA CADA FRAME)
	// =========================================================================
	//if (GEngine)
	//{
	//	// 1. Ver cuántos planetas hay registrados en tu mapa
	//	GEngine->AddOnScreenDebugMessage(10, 0.0f, FColor::Cyan, FString::Printf(TEXT("Planetas evaluados: %d"), CheckedPlanetsCount));

	//	if (NearestPlanet)
	//	{
	//		// 2. Ver a qué planeta estamos apuntando
	//		GEngine->AddOnScreenDebugMessage(11, 0.0f, FColor::Yellow, FString::Printf(TEXT("Planeta más cercano: %s"), *NearestPlanet->GetName()));

	//		// 3. Ver la distancia matemática vs el Threshold (Si es menor, se pone verde. Si es mayor, naranja)
	//		FColor DistColor = (MinDist <= ParentingDistanceThreshold) ? FColor::Green : FColor::Orange;
	//		GEngine->AddOnScreenDebugMessage(12, 0.0f, DistColor, FString::Printf(TEXT("Distancia a Superficie: %.2f cm | Límite: %.2f cm"), MinDist, ParentingDistanceThreshold));
	//	}
	//	else
	//	{
	//		GEngine->AddOnScreenDebugMessage(11, 0.0f, FColor::Red, TEXT("No se encontró ningún planeta cercano."));
	//	}
	//}
	//// =========================================================================

	//if (NearestPlanet && MinDist <= ParentingDistanceThreshold)
	//{
	//	if (CurrentParentPlanet != NearestPlanet)
	//	{
	//		CurrentParentPlanet = NearestPlanet;
	//		FAttachmentTransformRules AttachRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false);
	//		//SphereComp->SetSimulatePhysics(false);
	//		AttachToActor(NearestPlanet, AttachRules);

	//		// DEBUG: Aviso de que nos hemos pegado (Dura 3 segundos en pantalla)
	//		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Emerald, FString::Printf(TEXT(">>> ATTACHED AL PLANETA %s <<<"), *NearestPlanet->GetName()));
	//	}
	//}
	//else if (CurrentParentPlanet != nullptr)
	//{
	//	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, false);
	//	DetachFromActor(DetachRules);
	//	CurrentParentPlanet = nullptr;

	//	// DEBUG: Aviso de que nos hemos soltado (Dura 3 segundos en pantalla)
	//	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT(">>> DETACHED (LIBRE EN EL ESPACIO) <<<"));
	//}
}

void ACosmicSpherePlayer::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	// E: Si no hay input, reseteamos la dirección objetivo para que el modelo deje de intentar rotar.
	// I: If no input, reset the target direction so the model stops trying to rotate.
	if (MovementVector.IsNearlyZero()) { TargetFacingDirection = FVector::ZeroVector; return; }

	if (VisualRoot && CameraComp && CapsuleComp)
	{
		FVector UpVector = VisualRoot->GetUpVector();

		// E: Calculamos hacia dónde mira la cámara, ignorando si miramos hacia el cielo o el suelo.
		// I: Calculate where the camera is looking, ignoring if we look at the sky or the ground.
		FVector ForwardOnGround = FVector::VectorPlaneProject(CameraComp->GetForwardVector(), UpVector).GetSafeNormal();
		FVector RightOnGround = FVector::VectorPlaneProject(CameraComp->GetRightVector(), UpVector).GetSafeNormal();

		// E: Creamos el vector direccional y aplicamos fuerza a la esfera para rodar.
		// I: Create the directional vector and apply force to the sphere to roll.
		TargetFacingDirection = (ForwardOnGround * MovementVector.Y) + (RightOnGround * MovementVector.X);
		CapsuleComp->AddForce(TargetFacingDirection * MovementForce);
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
	// E: Solo saltamos si estamos en el suelo.
	// I: Only jump if we are grounded.
	if (!IsGrounded()) return;

	if (CapsuleComp && VisualRoot)
	{
		CapsuleComp->AddImpulse(VisualRoot->GetUpVector() * 800.0f, NAME_None, true);
	}
}

bool ACosmicSpherePlayer::IsGrounded() const
{
	if (!CapsuleComp || !VisualRoot) return false;

	// E: El rayo sale desde el centro de la cápsula hacia el suelo local (opuesto a UpVector).
	// I: The ray starts from the capsule center towards the local ground (opposite to UpVector).
	FVector Start = GetActorLocation();
	FVector Down = -VisualRoot->GetUpVector();

	// E: La longitud del rayo = mitad de la cápsula + un pequeño margen de 15 cm.
	// I: Ray length = half capsule height + a small 15 cm margin.
	float HalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();
	FVector End = Start + Down * (HalfHeight + 15.0f);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this); // E: Ignoramos al propio jugador. I: Ignore the player itself.

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility, // E: Canal estándar; cámbialo a WorldStatic si prefieres. I: Standard channel; swap to WorldStatic if preferred.
		Params
	);

	// E: (Opcional) Dibuja el rayo en pantalla para depuración. Comenta en producción.
	// I: (Optional) Draw the ray on screen for debugging. Comment out in production.
	// DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, -1.0f, 0, 2.0f);

	return bHit;
}
