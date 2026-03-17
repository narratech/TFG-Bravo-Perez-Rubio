// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/CosmicSpaceShip.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/PrimitiveComponent.h" 

ACosmicSpaceShip::ACosmicSpaceShip()
{
	// E: Desactivamos el Tick para mejorar rendimiento; usamos físicas constantes.
	// I: Disable Tick to improve performance; we use constant physics.
	PrimaryActorTick.bCanEverTick = true; // Cambiado a true para el frenado suave

	// E: Posesión automática por el jugador local.
	// I: Automatic possession by the local player.
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// E: Configuración de la malla raíz y sus propiedades físicas iniciales.
	// I: Root mesh setup and its initial physical properties.
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	RootComponent = ShipMesh;
	ShipMesh->SetSimulatePhysics(true);
	ShipMesh->SetEnableGravity(false); // E: Sin gravedad espacial. I: No space gravity.
	ShipMesh->SetLinearDamping(0.0f);  // E: Amortiguación lineal. I: Linear damping.
	ShipMesh->SetAngularDamping(0.5f); // E: Amortiguación de giro. I: Angular damping.

	// E: Inicialización del brazo y la cámara siguiendo a la nave.
	// I: Boom and camera initialization following the ship.
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(ShipMesh);
	SpringArmComp->TargetArmLength = 800.0f;
	SpringArmComp->bEnableCameraRotationLag = true;
	SpringArmComp->bDoCollisionTest = false;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	// Valores por defecto para el frenado
	BrakingSpeed = 5.0f;
}

void ACosmicSpaceShip::BeginPlay()
{
	Super::BeginPlay();

	// E: Eliminamos los límites del mundo para permitir vuelos a larga distancia.
	// I: Remove world bounds to allow long-distance flights.
	if (UWorld* World = GetWorld())
	{
		if (AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			WorldSettings->bEnableWorldBoundsChecks = false;
		}
	}
}

void ACosmicSpaceShip::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// E: Registro del contexto de mapeo para que Unreal reconozca el teclado.
	// I: Registration of the mapping context so Unreal recognizes the keyboard.
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

	// E: Vinculación de las acciones de entrada con las funciones de C++.
	// I: Binding input actions with C++ functions.
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Traslacion) { EnhancedInputComponent->BindAction(IA_Traslacion, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::AplicarTraslacion); }
		if (IA_Orientacion) { EnhancedInputComponent->BindAction(IA_Orientacion, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::AplicarOrientacion); }
		if (IA_Alabeo) { EnhancedInputComponent->BindAction(IA_Alabeo, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::AplicarAlabeo); }
		if (IA_Boost)
		{
			EnhancedInputComponent->BindAction(IA_Boost, ETriggerEvent::Started, this, &ACosmicSpaceShip::StartBoost);
			EnhancedInputComponent->BindAction(IA_Boost, ETriggerEvent::Completed, this, &ACosmicSpaceShip::EndBoost);
		}
	}
}

void ACosmicSpaceShip::AplicarTraslacion(const FInputActionValue& Value)
{
	FVector MovementVector = Value.Get<FVector>();

	if (!MovementVector.IsNearlyZero() && ShipMesh)
	{
		// E: En modo boost, solo permitir movimiento hacia adelante
		// I: In boost mode, only allow forward movement
		if (bBoostMode)
		{
			// E: Si no es movimiento hacia adelante (eje X positivo), ignorar
			// I: If it's not forward movement (positive X axis), ignore
			if (MovementVector.X <= 0.5f || FMath::Abs(MovementVector.Y) > 0.5f)
			{
				return;
			}
		}

		// E: Transformamos el vector local a mundial para aplicar la fuerza correctamente.
		// I: Transform the local vector to world to apply the force correctly.
		FVector LocalForce = MovementVector * ThrusterForce * GetWorld()->GetDeltaSeconds();

		// E: APLICAR BOOST - Si estamos en modo boost, multiplicamos la fuerza
		// I: APPLY BOOST - If in boost mode, multiply the force
		if (bBoostMode)
		{
			LocalForce *= BoostIncreasePower; // Necesitas añadir esta variable
		}

		FVector WorldForce = ShipMesh->GetComponentRotation().RotateVector(LocalForce);

		ShipMesh->AddForce(WorldForce, NAME_None, true);
	}
}

void ACosmicSpaceShip::AplicarOrientacion(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = -Value.Get<FVector2D>();

	if (!LookAxisVector.IsNearlyZero() && ShipMesh)
	{
		// E: Aplicamos torque rotando el vector local de giro al espacio del mundo.
		// I: Apply torque by rotating the local spin vector to world space.
		FVector LocalTorque = FVector(0.0f, LookAxisVector.Y * -1.0f, LookAxisVector.X) * RotationTorque * GetWorld()->GetDeltaSeconds();
		FVector WorldTorque = ShipMesh->GetComponentRotation().RotateVector(LocalTorque);

		ShipMesh->AddTorqueInDegrees(WorldTorque, NAME_None, true);
	}
}

void ACosmicSpaceShip::AplicarAlabeo(const FInputActionValue& Value)
{
	float RollValue = Value.Get<float>();

	if (FMath::Abs(RollValue) > 0.0f && ShipMesh)
	{
		// E: El alabeo (Roll) se aplica sobre el eje X frontal de la nave.
		// I: Roll is applied on the front X axis of the ship.
		FVector LocalTorque = FVector(RollValue, 0.0f, 0.0f) * AlabeoTorque * GetWorld()->GetDeltaSeconds();
		FVector WorldTorque = ShipMesh->GetComponentRotation().RotateVector(LocalTorque);

		ShipMesh->AddTorqueInDegrees(WorldTorque, NAME_None, true);
	}
}

void ACosmicSpaceShip::StartBoost(const FInputActionValue& Value)
{
	bBoostMode = true;

	// E: Opcional: Aumentar el damping lineal para que se sienta más "pegado" al frente
	// I: Optional: Increase linear damping to feel more "stuck" to forward direction
	if (ShipMesh)
	{
		// E: Guardar damping original para restaurar después
		// I: Save original damping to restore later
		OriginalLinearDamping = ShipMesh->GetLinearDamping();
		ShipMesh->SetLinearDamping(0.1f); // Pequeño damping para estabilidad
	}
}

void ACosmicSpaceShip::EndBoost(const FInputActionValue& Value)
{
	bBoostMode = false;

	// E: Restaurar damping original
	// I: Restore original damping
	if (ShipMesh)
	{
		ShipMesh->SetLinearDamping(OriginalLinearDamping);
	}

	// E: El frenado se manejará en Tick
	// I: Braking will be handled in Tick
}