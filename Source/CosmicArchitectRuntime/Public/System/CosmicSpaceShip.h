// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "CosmicSpaceShip.generated.h"

// E: Exponemos explícitamente la clase al motor para permitir la creación de Blueprints hijos.
// I: Explicitly expose the class to the engine to allow the creation of child Blueprints.
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicSpaceShip : public APawn
{
	GENERATED_BODY()

public:
	ACosmicSpaceShip();

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// E: Componente visual de la nave. Será la raíz y el que reciba las físicas newtonianas.
	// I: Visual component of the ship. It will be the root and receive the Newtonian physics.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UStaticMeshComponent* ShipMesh;

	// E: Brazo articulado para la cámara que permite añadir retraso visual al rotar y mover la nave.
	// I: Articulated camera boom that allows adding visual lag when rotating and moving the spaceship.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class USpringArmComponent* SpringArmComp;

	// E: Cámara principal que seguirá a la nave.
	// I: Main camera that will follow the ship.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UCameraComponent* CameraComp;

	// E: Fuerza base de los propulsores para desplazarse (Traslación).
	// I: Base strength of the thrusters for moving (Translation).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float ThrusterForce = 500000.0f;

	// E: Fuerza base de los motores de giro para orientar la nave (Rotación).
	// I: Base strength of the turning motors to orient the ship (Rotation).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float RotationTorque = 300000.0f;

	// E: Contexto de mapeo de controles por defecto.
	// I: Default control mapping context.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputMappingContext* DefaultMappingContext;

	// E: Input para moverse (Adelante/Atrás, Izquierda/Derecha, Arriba/Abajo) -> Requiere Value Type: Axis3D
	// I: Input for moving (Forward/Backward, Left/Right, Up/Down) -> Requires Value Type: Axis3D
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Traslacion;

	// E: Input para apuntar el morro de la nave (Cabeceo/Pitch, Guiñada/Yaw) -> Requiere Value Type: Axis2D
	// I: Input to point the nose of the ship (Pitch, Yaw) -> Requires Value Type: Axis2D
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Orientacion;

	// E: Input para girar sobre sí misma (Roll) -> Requiere Value Type: Axis1D
	// I: Input to spin on itself (Roll) -> Requires Value Type: Axis1D
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Alabeo;

private:
	// E: Funciones internas para procesar las entradas del jugador.
	// I: Internal functions to process player inputs.
	void AplicarTraslacion(const FInputActionValue& Value);
	void AplicarOrientacion(const FInputActionValue& Value);
	void AplicarAlabeo(const FInputActionValue& Value);
};