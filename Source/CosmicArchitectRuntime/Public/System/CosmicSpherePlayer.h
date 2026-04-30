// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "CosmicSpherePlayer.generated.h"

class UCosmicGravityComponent;
class UCapsuleComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class UCameraComponent;

// E: Peón principal del plugin Cosmic Architect. Utiliza un colisionador esférico para evitar problemas de rotación física en planetas curvos.
// I: Main pawn of the Cosmic Architect plugin. Uses a spherical collider to prevent physical rotation issues on curved planets.
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicSpherePlayer : public APawn
{
	GENERATED_BODY()

public:
	// E: Constructor por defecto. Configura la jerarquía de componentes inicial.
	// I: Default constructor. Sets up the initial component hierarchy.
	ACosmicSpherePlayer();

	// E: Gestiona la alineación gravitacional del modelo 3D frame a frame.
	// I: Manages the gravitational alignment of the 3D model frame by frame.
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	// E: Vincula los controles del jugador (Enhanced Input System).
	// I: Binds the player controls (Enhanced Input System).
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// =========================================================================
	// COMPONENTES DE JERARQUÍA (HIERARCHY COMPONENTS)
	// =========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	UCapsuleComponent* CapsuleComp;

	// E: Nodo que no rota con la esfera física, sino que se alinea estrictamente con la gravedad (Eje Z siempre hacia "Arriba").
	// I: Node that does not rotate with the physics sphere, but aligns strictly with gravity (Z axis always "Up").
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USceneComponent* VisualRoot;

	// E: Nodo responsable de rotar al personaje suavemente hacia donde camina.
	// I: Node responsible for smoothly rotating the character towards where it walks.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USceneComponent* MeshRoot;

	// E: La malla 3D de tu personaje.
	// I: The 3D mesh of your character.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USkeletalMeshComponent* PlayerMesh;

	// E: Brazo de la cámara.
	// I: Camera spring arm.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USpringArmComponent* SpringArmComp;

	// E: Cámara del jugador.
	// I: Player camera.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	UCameraComponent* CameraComp;

	// E: Componente del plugin responsable de recibir la atracción gravitacional.
	// I: Plugin component responsible for receiving gravitational pull.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	UCosmicGravityComponent* GravityComp;

	// =========================================================================
	// PARÁMETROS CONFIGURABLES (CONFIGURABLE PARAMETERS)
	// =========================================================================

	// E: Fuerza base aplicada a la esfera para moverse. Auméntala si incrementas la masa del jugador.
	// I: Base force applied to the sphere to move. Increase it if you increase the player's mass.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Movement")
	float MovementForce = 350000.0f;

	// E: Sensibilidad de la cámara al mover el ratón o el joystick.
	// I: Camera sensitivity when moving the mouse or joystick.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Input")
	float MouseSensitivity = 1.0f;

	// E: Distancia (en unidades) a la superficie de un planeta a partir de la cual el jugador se hace hijo dinámico de este. Útil para planetas en movimiento.
	// I: Distance (in units) to a planet's surface at which the player becomes its dynamic child. Useful for moving planets.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Parenting")
	float ParentingDistanceThreshold = 150.0f;

	// E: Muestra en el editor el planeta al que el jugador está anclado actualmente. (Solo lectura).
	// I: Shows in the editor the planet the player is currently attached to. (Read-only).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Parenting")
	AActor* CurrentParentPlanet;

	// =========================================================================
	// ENHANCED INPUT SYSTEM (NUEVO SISTEMA DE CONTROLES)
	// =========================================================================

	// E: Asigna aquí tu Input Mapping Context desde el Blueprint.
	// I: Assign your Input Mapping Context here from the Blueprint.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputMappingContext* DefaultMappingContext;

	// E: Acción para Caminar (Generalmente WASD / Joystick izquierdo).
	// I: Action for Walking (Usually WASD / Left Joystick).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerMove;

	// E: Acción para Mirar (Ratón / Joystick derecho).
	// I: Action for Looking (Mouse / Right Joystick).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerLook;

	// E: Acción para Saltar (Barra espaciadora / Botón frontal inferior).
	// I: Action for Jumping (Spacebar / Bottom face button).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerJump;

private:
	// E: Funciones internas ejecutadas por los Inputs.
	// I: Internal functions executed by the Inputs.
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump(const FInputActionValue& Value);

	// E: Lógica que evalúa si el jugador debe anclarse a un planeta cercano para heredar su movimiento/rotación.
	// I: Logic that evaluates if the player should attach to a nearby planet to inherit its movement/rotation.
	void HandleDynamicParenting();

	// E: Variables de control de cámara.
	// I: Camera control variables.
	float CameraYaw;
	float CameraPitch;
	FVector TargetFacingDirection;
};