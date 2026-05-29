// Copyright 1998 - 2026 Epic Games, Inc. All Rights Reserved.

#include "System/CosmicSpaceShip.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h" 
#include "Components/PrimitiveComponent.h" 

ACosmicSpaceShip::ACosmicSpaceShip()
{
	// Permite actualización runtime continua para:
	// - Simulación física
	// - Frenado espacial
	// - Sistemas de boost
	PrimaryActorTick.bCanEverTick = true;

	// Asigna automáticamente el control
	// al jugador local principal.
	AutoPossessPlayer = EAutoReceiveInput::Player0; 

	// Configuración del rigid body principal
	// utilizado para navegación espacial.
	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	RootComponent = ShipMesh;

	// Activa simulación física completa.
	ShipMesh->SetSimulatePhysics(true);

	// Desactiva gravedad para movimiento espacial.
	ShipMesh->SetEnableGravity(false);

	// Reduce resistencia lineal para conservar inercia.
	ShipMesh->SetLinearDamping(0.0f);

	// Estabiliza parcialmente la rotación angular.
	ShipMesh->SetAngularDamping(0.5f);

	// Sistema de cámara desacoplada
	// mediante spring arm.
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	SpringArmComp->SetupAttachment(ShipMesh);
	SpringArmComp->TargetArmLength = 800.0f;
	SpringArmComp->bEnableCameraRotationLag = true;

	// Evita colisiones de cámara en espacio abierto.
	SpringArmComp->bDoCollisionTest = false;

	// Cámara principal controlada por el jugador.
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArmComp);

	// Configuración inicial del sistema de frenado.
	BrakingSpeed = 5.0f;
}

void ACosmicSpaceShip::BeginPlay()
{
	Super::BeginPlay();

	// Elimina restricciones de límites globales
	// para navegación a gran escala.
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

	// Registra el contexto principal de input
	// dentro del subsistema local del jugador.
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

	// Vincula acciones Enhanced Input
	// con lógica C++ runtime.
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
		// Durante boost únicamente se permite
		// aceleración frontal estabilizada.
		if (bBoostMode)
		{
			// Bloquea desplazamientos laterales
			// y retroceso durante boost.
			if (MovementVector.X <= 0.5f || FMath::Abs(MovementVector.Y) > 0.5f)
			{
				return;
			}
		}

		// Fuerza local aplicada proporcionalmente
		// al input y delta temporal.
		FVector LocalForce = MovementVector * ThrusterForce * GetWorld()->GetDeltaSeconds();

		// Incrementa potencia de propulsión
		// mientras el boost está activo.
		if (bBoostMode)
		{
			LocalForce *= BoostIncreasePower;
		}

		// Conversión desde espacio local
		// hacia coordenadas globales.
		FVector WorldForce = ShipMesh->GetComponentRotation().RotateVector(LocalForce);

		// Aplicación física final sobre el rigid body.
		ShipMesh->AddForce(WorldForce, NAME_None, true);
	}
}

void ACosmicSpaceShip::AplicarOrientacion(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (!LookAxisVector.IsNearlyZero() && ShipMesh)
	{
		// Torque local utilizado para:
		// - Pitch
		// - Yaw
		FVector LocalTorque = FVector(0.0f, LookAxisVector.Y * -1.0f, LookAxisVector.X) * RotationTorque * GetWorld()->GetDeltaSeconds();

		// Conversión desde espacio local
		// hacia espacio global.
		FVector WorldTorque = ShipMesh->GetComponentRotation().RotateVector(LocalTorque);

		// Aplicación física de torque angular.
		ShipMesh->AddTorqueInDegrees(WorldTorque, NAME_None, true);
	}
}

void ACosmicSpaceShip::AplicarAlabeo(const FInputActionValue& Value)
{
	float RollValue = Value.Get<float>();

	if (FMath::Abs(RollValue) > 0.0f && ShipMesh)
	{
		// El alabeo se aplica sobre el eje
		// longitudinal frontal de la nave.
		FVector LocalTorque = FVector(RollValue, 0.0f, 0.0f) * AlabeoTorque * GetWorld()->GetDeltaSeconds();

		// Conversión hacia coordenadas globales.
		FVector WorldTorque = ShipMesh->GetComponentRotation().RotateVector(LocalTorque);

		// Aplicación física del torque de roll.
		ShipMesh->AddTorqueInDegrees(WorldTorque, NAME_None, true);
	}
}

void ACosmicSpaceShip::StartBoost(const FInputActionValue& Value)
{
	bBoostMode = true;

	// Incrementa estabilidad lineal durante
	// navegación a alta velocidad.
	if (ShipMesh)
	{
		// Guarda damping original para restaurarlo
		// al finalizar el boost.
		OriginalLinearDamping = ShipMesh->GetLinearDamping();

		// Reduce deriva lateral durante boost.
		ShipMesh->SetLinearDamping(0.1f);
	}
}

void ACosmicSpaceShip::EndBoost(const FInputActionValue& Value)
{
	bBoostMode = false;

	// Restaura configuración física original
	// tras abandonar el boost.
	if (ShipMesh)
	{
		ShipMesh->SetLinearDamping(OriginalLinearDamping);
	}

	// El frenado residual será gestionado
	// posteriormente mediante Tick/runtime.
}