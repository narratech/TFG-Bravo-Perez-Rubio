// Fill out your copyright notice in the Description page of Project Settings.


#include "System/AttachParentComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"

// Constructor
UAttachParentComponent::UAttachParentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bTickInEditor = true;
}

void UAttachParentComponent::DebugTriggerState()
{
	if (!TriggerSphere)
	{
		UE_LOG(LogTemp, Error, TEXT("TriggerSphere es NULL!"));
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Posiciones
	FVector OwnerPos = Owner->GetActorLocation();
	FVector TriggerPos = TriggerSphere->GetComponentLocation();
	FVector RelativePos = TriggerPos - OwnerPos;

	UE_LOG(LogTemp, Warning, TEXT("======== DEBUG TRIGGER ========"));
	UE_LOG(LogTemp, Warning, TEXT("Owner: %s"), *Owner->GetName());
	UE_LOG(LogTemp, Warning, TEXT("Owner Position: %s"), *OwnerPos.ToString());
	UE_LOG(LogTemp, Warning, TEXT("Trigger Position: %s"), *TriggerPos.ToString());
	UE_LOG(LogTemp, Warning, TEXT("Relative Position: %s"), *RelativePos.ToString());
	UE_LOG(LogTemp, Warning, TEXT("Trigger Radius: %f km (%.0f unreal units)"),
		AttachRadiusKm, TriggerSphere->GetUnscaledSphereRadius());

	// Verificar adjunción
	UE_LOG(LogTemp, Warning, TEXT("Attached to: %s"),
		TriggerSphere->GetAttachParent() ?
		*TriggerSphere->GetAttachParent()->GetName() : TEXT("Nothing"));

	// Actores superpuestos
	TArray<AActor*> OverlappingActors;
	TriggerSphere->GetOverlappingActors(OverlappingActors);
	UE_LOG(LogTemp, Warning, TEXT("Actors overlapping: %d"), OverlappingActors.Num());

	for (AActor* Actor : OverlappingActors)
	{
		if (Actor)
		{
			float Distance = FVector::Dist(Actor->GetActorLocation(), TriggerPos);
			UE_LOG(LogTemp, Warning, TEXT("  - %s (distance: %.2f)"),
				*Actor->GetName(), Distance);
		}
	}

	// Verificar configuración de colisión
	FCollisionResponseContainer Response = TriggerSphere->GetCollisionResponseToChannels();
	UE_LOG(LogTemp, Warning, TEXT("Collision Enabled: %d"),
		(uint8)TriggerSphere->GetCollisionEnabled());
	UE_LOG(LogTemp, Warning, TEXT("Generate Overlap Events: %d"),
		TriggerSphere->GetGenerateOverlapEvents());
	UE_LOG(LogTemp, Warning, TEXT("================================="));
}

// BeginPlay
void UAttachParentComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld()) return;

	// Crear trigger dinámico
	TriggerSphere = NewObject<USphereComponent>(Owner, USphereComponent::StaticClass(),
		TEXT("AttachTriggerSphere"));

	if (!TriggerSphere) return;

	TriggerSphere->RegisterComponentWithWorld(GetWorld());

	// Adjuntar al root
	if (Owner->GetRootComponent())
	{
		TriggerSphere->AttachToComponent(Owner->GetRootComponent(),
			FAttachmentTransformRules::KeepRelativeTransform);

		// Obtener la escala del padre
		FVector ParentScale = Owner->GetRootComponent()->GetComponentScale();
		float UniformScale = ParentScale.GetMax(); // Usar la escala máxima

		// Compensar el radio dividiendo por la escala
		float DesiredRadius = AttachRadiusKm * 100000.0f;
		float CompensatedRadius = DesiredRadius / UniformScale;

		TriggerSphere->SetSphereRadius(CompensatedRadius);

		UE_LOG(LogTemp, Warning, TEXT("Parent Scale: %s"), *ParentScale.ToString());
		UE_LOG(LogTemp, Warning, TEXT("Desired Radius: %f"), DesiredRadius);
		UE_LOG(LogTemp, Warning, TEXT("Compensated Radius: %f (to account for scale)"),
			CompensatedRadius);
		UE_LOG(LogTemp, Warning, TEXT("Final World Radius: %f"),
			CompensatedRadius * UniformScale);
	}

	// Resto de la configuración...
	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	TriggerSphere->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	TriggerSphere->SetGenerateOverlapEvents(true);

	// Bind eventos
	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this,
		&UAttachParentComponent::OnBeginOverlap);
	TriggerSphere->OnComponentEndOverlap.AddDynamic(this,
		&UAttachParentComponent::OnEndOverlap);

	// Debug
	//DebugTriggerState();
}

// Tick
void UAttachParentComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bShowDebugArea)
	{
		DrawDebugArea();
	}

	// Debug: Verificar si el trigger existe y su radio
	if (TriggerSphere)
	{
		TArray<AActor*> OverlappingActors;
		TriggerSphere->GetOverlappingActors(OverlappingActors);

		if (OverlappingActors.Num() > 0)
		{
			UE_LOG(LogTemp, Verbose, TEXT("Actors overlapping: %d"),
				OverlappingActors.Num());
		}
	}
}

// Overlap begin
void UAttachParentComponent::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	// Intentar con array
	if (ActorsToAttach.Contains(OtherActor))
	{
		TryAttach(OtherActor);
	}

	// Intentar con pawn
	if (bAttachPlayerPawn)
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (OtherActor == Pawn)
		{
			TryAttach(Pawn);
		}
	}
}

// Overlap end
void UAttachParentComponent::OnEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor || !bAutoDetach) return;

	

	if (ActorsToAttach.Contains(OtherActor))
	{
		TryDetach(OtherActor);
	}

	if (bAttachPlayerPawn)
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (OtherActor == Pawn)
		{
			TryDetach(Pawn);
		}
	}
}

// Attach
void UAttachParentComponent::TryAttach(AActor* Actor)
{
	if (!Actor || !GetOwner()) return;

	Actor->AttachToActor(GetOwner(), FAttachmentTransformRules::KeepWorldTransform);

	UE_LOG(LogTemp, Warning, TEXT("Conectado"));
}

// Detach
void UAttachParentComponent::TryDetach(AActor* Actor)
{
	if (!Actor) return;

	Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	UE_LOG(LogTemp, Warning, TEXT("Desconectado"));
}

void UAttachParentComponent::DrawDebugArea()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector Center = GetOwner()->GetActorLocation();
	float Radius = AttachRadiusKm * 100000;

	//UE_LOG(LogTemp, Warning, TEXT("Posicion %4.f"), Center.X);

	TArray<FVector> CircleXY;
	TArray<FVector> CircleXZ;

	for (int32 i = 0; i <= DebugSegments; i++)
	{
		float Angle = (2 * PI * i) / DebugSegments;

		// Plano XY
		CircleXY.Add(Center + FVector(
			FMath::Cos(Angle) * Radius,
			FMath::Sin(Angle) * Radius,
			0));

		// Plano XZ (cruzado)
		CircleXZ.Add(Center + FVector(
			FMath::Cos(Angle) * Radius,
			0,
			FMath::Sin(Angle) * Radius));
	}

	// Dibujar ambos
	for (int32 i = 0; i < CircleXY.Num() - 1; i++)
	{
		DrawDebugLine(World, CircleXY[i], CircleXY[i + 1], DebugColor, false, -1, 0, DebugThickness);
		DrawDebugLine(World, CircleXZ[i], CircleXZ[i + 1], DebugColor, false, -1, 0, DebugThickness);
	}
}

