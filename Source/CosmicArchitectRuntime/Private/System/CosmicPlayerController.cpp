// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "System/CosmicPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"

ACosmicPlayerController::ACosmicPlayerController()
{
	bReplicates = true;
}

void ACosmicPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// If already possessing a local pawn on begin play, setup context
	if (IsLocalController() && GetPawn())
	{
		SetupInputContext();
	}
}

void ACosmicPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Called on server (or standalone) when possessing a pawn
	if (IsLocalController())
	{
		SetupInputContext();
	}
}

void ACosmicPlayerController::OnUnPossess()
{
	if (IsLocalController())
	{
		RemoveInputContext();
	}

	Super::OnUnPossess();
}

void ACosmicPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	// Canonical client-side notification of pawn possession in UE
	if (IsLocalController())
	{
		SetupInputContext();
	}
}

void ACosmicPlayerController::SetupInputContext()
{
	if (!DefaultMappingContext)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (!Subsystem->HasMappingContext(DefaultMappingContext))
			{
				Subsystem->AddMappingContext(DefaultMappingContext, MappingPriority);
			}
		}
	}
}

void ACosmicPlayerController::RemoveInputContext()
{
	if (!DefaultMappingContext)
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->RemoveMappingContext(DefaultMappingContext);
		}
	}
}
