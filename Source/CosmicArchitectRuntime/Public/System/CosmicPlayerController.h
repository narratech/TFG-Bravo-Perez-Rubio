// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CosmicPlayerController.generated.h"

class UInputMappingContext;

/**
 * Custom PlayerController for CosmicArchitect.
 * Manages Enhanced Input mapping contexts safely across network possession events
 * on both client and server.
 */
UCLASS(Blueprintable, BlueprintType)
class COSMICARCHITECTRUNTIME_API ACosmicPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACosmicPlayerController();

	/** Default Input Mapping Context to apply when possessing a pawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Mapping context priority */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CosmicArchitect|Input")
	int32 MappingPriority = 0;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void AcknowledgePossession(APawn* P) override;

	/** Adds DefaultMappingContext to the EnhancedInput subsystem for this local player */
	void SetupInputContext();

	/** Removes DefaultMappingContext from the EnhancedInput subsystem */
	void RemoveInputContext();
};
