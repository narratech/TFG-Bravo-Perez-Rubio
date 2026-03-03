#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h" 
#include "CosmicSpaceShip.generated.h"

// E: Clase base C++ que maneja la lógica de movimiento espacial, delegando assets e inputs a un Blueprint hijo.
// I: Base C++ class handling space movement logic, delegating assets and inputs to a child Blueprint.
UCLASS()
class COSMICARCHITECTRUNTIME_API ACosmicSpaceShip : public APawn
{
	GENERATED_BODY()

public:
	ACosmicSpaceShip();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

	// E: Malla visual de la nave sin físicas de colisión activas.
	// I: Visual mesh of the spaceship with collision physics disabled.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UStaticMeshComponent* ShipMesh;

	// E: Cámara del jugador asociada a la estructura de la nave.
	// I: Player camera attached to the spaceship structure.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UCameraComponent* CameraComp;

	// E: Sustituye las físicas puras para alcanzar velocidades extremas sin los límites del motor Chaos.
	// I: Replaces pure physics to reach extreme speeds bypassing Chaos engine limits.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UFloatingPawnMovement* MovementComp;

	// E: Archivos del Enhanced Input expuestos para asignarse visualmente sin depender del DefaultInput.ini.
	// I: Enhanced Input files exposed for visual assignment without relying on DefaultInput.ini.
	UPROPERTY(EditDefaultsOnly, Category = "CosmicArchitect|Input")
	class UInputMappingContext* IMC_SpaceShip;

	UPROPERTY(EditDefaultsOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Impulso;

	UPROPERTY(EditDefaultsOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_GiroRaton;

	UPROPERTY(EditDefaultsOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Roll;

	UPROPERTY(EditDefaultsOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_Boost;

	// E: Multiplicadores de velocidad y sensibilidad de giro modificables desde el editor.
	// I: Speed and rotation sensitivity multipliers adjustable from the editor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Navegacion")
	float VelocityFactor = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Navegacion")
	float Sensibilidad = 2.0f;

private:
	// E: Funciones de respuesta vinculadas a los eventos del Enhanced Input System.
	// I: Response functions bound to the Enhanced Input System events.
	void Mover(const FInputActionValue& Value);
	void Mirar(const FInputActionValue& Value);
	void RotarRoll(const FInputActionValue& Value);
	void IniciarBoost();
	void DetenerBoost();

	// E: Variables para interpolar suavemente el FOV en la función Tick, sustituyendo al Timeline de Blueprint.
	// I: Variables to smoothly interpolate FOV in the Tick function, replacing the Blueprint Timeline.
	float TargetFOV = 90.0f;
	float CurrentFOV = 90.0f;
};