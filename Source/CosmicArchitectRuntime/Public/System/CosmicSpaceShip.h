// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "CosmicSpaceShip.generated.h"

// E: Clase principal de la nave espacial para el plugin CosmicArchitect.
// I: Main spaceship class for the CosmicArchitect plugin.
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicSpaceShip : public APawn
{
	GENERATED_BODY()

public:
	// E: Constructor para inicializar componentes.
	// I: Constructor to initialize components.
	ACosmicSpaceShip();

protected:
	// E: Llamado al iniciar el juego.
	// I: Called when the game starts.
	virtual void BeginPlay() override;

	// E: Configuración de los vínculos de entrada (teclado/ratón).
	// I: Setup for input bindings (keyboard/mouse).
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// E: Componente de malla que servirá como raíz física.
	// I: Mesh component that will serve as the physical root.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UStaticMeshComponent* ShipMesh;

	// E: Brazo elástico para la cámara que suaviza el movimiento.
	// I: Spring arm for the camera that smoothes movement.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class USpringArmComponent* SpringArmComp;

	// E: Cámara principal del jugador.
	// I: Main player camera.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UCameraComponent* CameraComp;

	// E: Multiplicador de fuerza para el movimiento lineal.
	// I: Force multiplier for linear movement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float ThrusterForce = 500000.0f;

	// E: Multiplicador de fuerza para el giro de la nave.
	// I: Force multiplier for ship rotation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float RotationTorque = 300000.0f;

	// E: Multiplicador de fuerza para el alabeo de la nave.
	// I: Force multiplier for ship rotation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	float AlabeoTorque = 300000.0f;

	// E: Contexto que define qué teclas activan qué acciones.
	// I: Context defining which keys trigger which actions.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputMappingContext* DefaultMappingContext;

	// E: Acción de entrada para traslación (W,A,S,D,Q,E).
	// I: Input action for translation (W,A,S,D,Q,E).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Traslacion;

	// E: Acción de entrada para mirar (Ratón X/Y).
	// I: Input action for looking (Mouse X/Y).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Orientacion;

	// E: Acción de entrada para giro lateral (Roll).
	// I: Input action for lateral roll.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Alabeo;

private:
	// E: Lógica para mover la nave en el espacio.
	// I: Logic to move the ship in space.
	void AplicarTraslacion(const FInputActionValue& Value);

	// E: Lógica para rotar el morro de la nave.
	// I: Logic to rotate the ship's nose.
	void AplicarOrientacion(const FInputActionValue& Value);

	// E: Lógica para inclinar la nave lateralmente.
	// I: Logic to tilt the ship laterally.
	void AplicarAlabeo(const FInputActionValue& Value);
};