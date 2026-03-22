// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "CosmicPlayer.generated.h"

// E: Peón principal para la exploración a pie en superficies planetarias (6DOF / Gravedad Esférica).
// I: Main pawn for on-foot exploration on planetary surfaces (6DOF / Spherical Gravity).
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicPlayer : public APawn
{
	GENERATED_BODY()

public:
	// E: Constructor para inicializar componentes y valores por defecto.
	// I: Constructor to initialize components and default values.
	ACosmicPlayer();

	// E: Se utiliza el Tick para alinear constantemente el jugador con la gravedad del planeta.
	// I: Tick is used to constantly align the player with the planet's gravity.
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	// E: Configuración de los vínculos de entrada (teclado/ratón).
	// I: Setup for input bindings (keyboard/mouse).
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// E: Cápsula de colisión que servirá como raíz física del jugador.
	// I: Collision capsule serving as the physical root of the player.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UCapsuleComponent* CapsuleComp;

	// E: Brazo elástico para la cámara.
	// I: Spring arm for the camera.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class USpringArmComponent* SpringArmComp;

	// E: Cámara principal del jugador.
	// I: Main player camera.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UCameraComponent* CameraComp;

	// =========================================================================
	// VARIABLES CONFIGURABLES DESDE EL EDITOR / CONFIGURABLE VARIABLES FROM EDITOR
	// =========================================================================

	// E: Fuerza de aceleración al caminar. Usamos double para LWC.
	// I: Walking acceleration force. We use double for LWC.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	double MovementForce = 800000.0;

	// E: Potencia del impulso de salto hacia arriba (relativo al planeta).
	// I: Upward jump impulse power (relative to the planet).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	double JumpForce = 500000.0;

	// E: Multiplicador de sensibilidad para la cámara (Ratón).
	// I: Sensitivity multiplier for the camera (Mouse).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float MouseSensitivity = 1.0f;

	// =========================================================================
	// ENHANCED INPUT SYSTEM
	// =========================================================================

	// E: Contexto de mapeo de controles base.
	// I: Base control mapping context.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputMappingContext* DefaultMappingContext;

	// E: Acción de entrada para caminar (Adelante, Atrás, Izquierda, Derecha).
	// I: Input action for walking (Forward, Backward, Left, Right).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerMove;

	// E: Acción de entrada para mirar con la cámara.
	// I: Input action for looking with the camera.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerLook;

	// E: Acción de entrada para saltar.
	// I: Input action for jumping.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerJump;

private:
	// E: Métodos de vinculación de entrada.
	// I: Input binding methods.
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump(const FInputActionValue& Value);

	// E: Referencia temporal para el centro del planeta actual en coordenadas LWC.
	// I: Temporary reference for the current planet's center in LWC coordinates.
	FVector3d CurrentPlanetCenter = FVector3d::ZeroVector;
};