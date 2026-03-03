#include "CosmicSpaceShip.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

// E: Configura los componentes base y establece límites extremos de velocidad para la navegación espacial.
// I: Configures base components and sets extreme speed limits for space navigation.
ACosmicSpaceShip::ACosmicSpaceShip()
{
	PrimaryActorTick.bCanEverTick = true;

	ShipMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipMesh"));
	RootComponent = ShipMesh;

	// E: Apagamos las físicas puras porque el movimiento se delega al FloatingPawnMovement.
	// I: We disable pure physics because movement is delegated to the FloatingPawnMovement.
	ShipMesh->SetSimulatePhysics(false);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(ShipMesh);
	CameraComp->SetRelativeLocation(FVector(-500.0f, 0.0f, 100.0f));

	MovementComp = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComp"));
	MovementComp->MaxSpeed = 1000000.0f;
	MovementComp->Acceleration = 500000.0f;
	MovementComp->Deceleration = 300000.0f;
}

// E: Inicializa el Enhanced Input System en el Player Controller para registrar las teclas del plugin.
// I: Initializes the Enhanced Input System in the Player Controller to register plugin keys.
void ACosmicSpaceShip::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (IMC_SpaceShip)
			{
				Subsystem->AddMappingContext(IMC_SpaceShip, 0);
			}
		}
	}
}

// E: Interpola gradualmente el FOV actual hacia el FOV objetivo para simular el efecto visual de aceleración.
// I: Gradually interpolates current FOV towards target FOV to simulate the acceleration visual effect.
void ACosmicSpaceShip::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, 10.0f);
	CameraComp->SetFieldOfView(CurrentFOV);
}

// E: Vincula los Input Actions del plugin a sus respectivas funciones nativas en C++.
// I: Binds the plugin's Input Actions to their respective native C++ functions.
void ACosmicSpaceShip::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Impulso) EnhancedInputComponent->BindAction(IA_Impulso, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::Mover);
		if (IA_GiroRaton) EnhancedInputComponent->BindAction(IA_GiroRaton, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::Mirar);
		if (IA_Roll) EnhancedInputComponent->BindAction(IA_Roll, ETriggerEvent::Triggered, this, &ACosmicSpaceShip::RotarRoll);

		if (IA_Boost)
		{
			EnhancedInputComponent->BindAction(IA_Boost, ETriggerEvent::Started, this, &ACosmicSpaceShip::IniciarBoost);
			EnhancedInputComponent->BindAction(IA_Boost, ETriggerEvent::Completed, this, &ACosmicSpaceShip::DetenerBoost);
		}
	}
}

// E: Aplica movimiento ignorando la masa usando el vector frontal y el factor de velocidad actual.
// I: Applies mass-ignoring movement using the forward vector and the current velocity factor.
void ACosmicSpaceShip::Mover(const FInputActionValue& Value)
{
	float EjeV = Value.Get<float>();
	if (EjeV != 0.0f)
	{
		AddMovementInput(GetActorForwardVector(), EjeV * VelocityFactor);
	}
}

// E: Rota la nave localmente en los ejes Yaw (X del ratón) y Pitch (Y del ratón).
// I: Rotates the spaceship locally on the Yaw (mouse X) and Pitch (mouse Y) axes.
void ACosmicSpaceShip::Mirar(const FInputActionValue& Value)
{
	FVector2D EjeRaton = Value.Get<FVector2D>();
	AddActorLocalRotation(FRotator(EjeRaton.Y * Sensibilidad, EjeRaton.X * Sensibilidad, 0.0f));
}

// E: Aplica una rotación exclusiva sobre el eje X local (Roll) de la nave espacial.
// I: Applies an exclusive rotation on the spaceship's local X axis (Roll).
void ACosmicSpaceShip::RotarRoll(const FInputActionValue& Value)
{
	float EjeRoll = Value.Get<float>();
	if (EjeRoll != 0.0f)
	{
		AddActorLocalRotation(FRotator(0.0f, 0.0f, EjeRoll * Sensibilidad));
	}
}

// E: Triplica la velocidad base y define un nuevo FOV objetivo para el zoom de la cámara.
// I: Triples the base speed and sets a new target FOV for the camera zoom.
void ACosmicSpaceShip::IniciarBoost()
{
	VelocityFactor = 15000.0f;
	TargetFOV = 115.0f;
}

// E: Restaura la velocidad estándar y devuelve el objetivo del FOV a su valor por defecto.
// I: Restores standard speed and returns the target FOV to its default value.
void ACosmicSpaceShip::DetenerBoost()
{
	VelocityFactor = 5000.0f;
	TargetFOV = 90.0f;
}