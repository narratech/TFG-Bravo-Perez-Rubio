// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "CosmicPlayer.generated.h"

// E: Peón principal para la exploración a pie en superficies planetarias.
// I: Main pawn for on-foot exploration on planetary surfaces.
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicPlayer : public APawn
{
	GENERATED_BODY()

public:
	ACosmicPlayer();

	// E: Se utiliza el Tick para alinear constantemente el jugador con la gravedad del planeta.
	// I: Tick is used to constantly align the player with the planet's gravity.
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// E: Cápsula de colisión que servirá como raíz física.
	// I: Collision capsule serving as the physical root.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UCapsuleComponent* CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Componentes")
	class UCameraComponent* CameraComp;

	// E: Contexto de mapeo y acciones de Enhanced Input.
	// I: Enhanced Input mapping context and actions.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerMove;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerLook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CosmicArchitect|Input")
	class UInputAction* IA_PlayerJump;

	// E: Parámetros de movimiento. Usamos double para prevenir pérdida de precisión en operaciones LWC si es necesario.
	// I: Movement parameters. We use double to prevent precision loss in LWC operations if needed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	double MovementForce = 800000.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Fisicas")
	double JumpForce = 500000.0;

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