// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "CosmicRealisticPlayer.generated.h"

class UCosmicGravityComponent;
class USphereComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class UCameraComponent;

// E: Peón realista del plugin Cosmic Architect. Utiliza una esfera física como base para garantizar un movimiento fluido y evitar problemas de rotación en gravedades esféricas.
// I: Realistic pawn from the Cosmic Architect plugin. Uses a physical sphere as a base to ensure smooth movement and prevent rotation issues in spherical gravities.
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicRealisticPlayer : public APawn
{
	GENERATED_BODY()

public:
	// E: Constructor por defecto del plugin.
	// I: Plugin default constructor.
	ACosmicRealisticPlayer();

	// E: Ciclo principal utilizado por el plugin para alinear visualmente al jugador con el centro gravitacional.
	// I: Main loop used by the plugin to visually align the player with the gravitational center.
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	// E: Configuración del sistema de entrada (Enhanced Input).
	// I: Input system setup (Enhanced Input).
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// =========================================================================
	// COMPONENTES DE JERARQUÍA DEL PLUGIN
	// =========================================================================

	// E: Esfera raíz para colisiones y físicas. El plugin desactiva su gravedad nativa.
	// I: Root sphere for collisions and physics. The plugin disables its native gravity.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USphereComponent* SphereComp;

	// E: Plataforma base estable. Se alinea automáticamente a la gravedad, sirviendo como ancla perfecta para la cámara.
	// I: Stable base platform. Automatically aligns to gravity, serving as a perfect anchor for the camera.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USceneComponent* VisualRoot;

	// E: Raíz independiente para el modelo 3D. Rota libremente hacia la dirección de movimiento (WASD).
	// I: Independent root for the 3D model. Rotates freely towards the movement direction (WASD).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USceneComponent* MeshRoot;

	// E: Malla visual del personaje. Sustitúyela por tu propio modelo en el Blueprint.
	// I: Character's visual mesh. Replace it with your own model in the Blueprint.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USkeletalMeshComponent* PlayerMesh;

	// E: Brazo retráctil para la cámara. Configurado para suavizar las rotaciones del planeta.
	// I: Retractable arm for the camera. Configured to smooth out planet rotations.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	USpringArmComponent* SpringArmComp;

	// E: Cámara principal del jugador.
	// I: Main player camera.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	UCameraComponent* CameraComp;

	// E: Componente central del plugin. Escanea el entorno buscando campos gravitacionales válidos.
	// I: Core plugin component. Scans the environment looking for valid gravitational fields.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Components")
	UCosmicGravityComponent* GravityComp;

	// =========================================================================
	// VARIABLES PARAMETRIZABLES (MODIFICABLES EN EL BLUEPRINT)
	// =========================================================================

	// E: Fuerza base que se aplica a la esfera. Auméntala si cambias la masa del personaje.
	// I: Base force applied to the sphere. Increase it if you change the character's mass.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Movement")
	float MovementForce = 350000.0f;

	// E: Multiplicador global de sensibilidad para la cámara.
	// I: Global sensitivity multiplier for the camera.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Input")
	float MouseSensitivity = 1.0f;

	// =========================================================================
	// ENHANCED INPUT SYSTEM
	// =========================================================================

	// E: Asigna aquí tu Input Mapping Context en el Blueprint.
	// I: Assign your Input Mapping Context here in the Blueprint.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputMappingContext* DefaultMappingContext;

	// E: Acción de entrada para moverse (Vector2D).
	// I: Input action for movement (Vector2D).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerMove;

	// E: Acción de entrada para mover la cámara (Vector2D).
	// I: Input action for camera look (Vector2D).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerLook;

	// E: Acción de entrada para el salto (Botón/Digital).
	// I: Input action for jump (Button/Digital).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerJump;

private:
	// E: Métodos internos de vinculación de entrada.
	// I: Internal input binding methods.
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump(const FInputActionValue& Value);

	// E: Variables puras para controlar la rotación de la cámara (Yaw y Pitch) sin gimbal lock.
	// I: Pure variables to control camera rotation (Yaw and Pitch) without gimbal lock.
	float CameraYaw;
	float CameraPitch;

	// E: Dirección objetivo hacia la que debe mirar el modelo 3D (Calculada por el input).
	// I: Target direction the 3D model should face (Calculated via input).
	FVector TargetFacingDirection;
};