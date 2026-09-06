// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Simulation/CosmicGravityComponent.h"
#include "Simulation/CosmicGravitySubsystem.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"

UCosmicGravityComponent::UCosmicGravityComponent()
{
    // ...
}


void UCosmicGravityComponent::BeginPlay()
{
    Super::BeginPlay(); 

    if (AActor* Owner = GetOwner())
    {
        RootPrimitive = Cast<UPrimitiveComponent>(Owner->GetRootComponent());

        if (RootPrimitive)
        {
            // If active mode requires movement, force Movable mobility to
            // allow SetActorLocation and AddForce to work correctly at runtime.
            if (GravityMode != ECosmicGravityMode::None && RootPrimitive->Mobility != EComponentMobility::Movable)
            {
                RootPrimitive->SetMobility(EComponentMobility::Movable);
            }

            // Orbital bodies use Unreal physics engine to resolve collisions
            // and force response. Engine internal gravity is disabled because gravity
            // is exclusively managed by CosmicGravitySubsystem.
            if (!IsPlanet) {
                RootPrimitive->SetSimulatePhysics(true);
                RootPrimitive->SetEnableGravity(false);
                RootPrimitive->SetMassOverrideInKg(NAME_None, Mass, true);
            }

        }
    }

    UCosmicGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UCosmicGravitySubsystem>();

    if (!Subsystem) return;

    // Planet mass is derived from observable physical properties (radius and surface gravity)
    // using the inverse Newtonian gravity formula: M = (g * R²) / G
    // This allows designers to configure planets with intuitive parameters without manual mass calculations.
    if (IsPlanet) {
        Mass = FMath::Square(RadiusKm * 1000) * SurfaceGravity / Subsystem->GetGravityConstant();
    }

    Subsystem->RegisterBody(this);
}

void UCosmicGravityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UCosmicGravitySubsystem* Subsystem =
            World->GetSubsystem<UCosmicGravitySubsystem>())
        {
            Subsystem->UnregisterBody(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void UCosmicGravityComponent::Integrate(double DeltaTime)
{
    // AccumulatedForce is in Newtons (SI units). Converted to cm/s² by multiplying by 100
    // to convert from meters to centimeters (Unreal Engine internal unit system).
    // Formula: a = F / m  ->  a_ue = (F / m) * 100
    FVector Acceleration = AccumulatedForce * 100 / Mass;

    if (!IsPlanet && RootPrimitive && RootPrimitive->IsSimulatingPhysics())
    {
        // When physics engine is active, full integration is delegated to it.
        // bAccelChange = true indicates vector is an acceleration, not a force,
        // avoiding the engine dividing by mass internally again.
        RootPrimitive->AddForce(Acceleration, NAME_None, true);
    }
    else
    {
        // Semi-implicit Euler integration for objects without engine physics (planets and kinematic bodies).
        // Velocity updated first, then position, for greater numerical stability.
        AActor* Owner = GetOwner();

        if (!Owner) return;

        Velocity += Acceleration * DeltaTime;

        FVector NewLocation = Owner->GetActorLocation() + (Velocity * DeltaTime);

        Owner->SetActorLocation(NewLocation);
    }

    // Save net acceleration direction before clearing accumulator.
    // CurrentGravityDirection allows external systems (character orientation,
    // particles, cameras, etc.) to know gravitational "down" this frame without recalculating.
    if (!AccumulatedForce.IsNearlyZero())
    {
        CurrentGravityDirection = Acceleration;
    }

    AccumulatedForce = FVector::ZeroVector;
}

void UCosmicGravityComponent::SetIsPlanet(bool bNewIsPlanet)
{
    if (bNewIsPlanet == IsPlanet) return;

    IsPlanet = bNewIsPlanet;

    // Subsystem maintains separate lists for planets and orbital bodies.
    // When changing role, it is necessary to unregister and re-register so
    // object is correctly classified in the corresponding list.
    if (UCosmicGravitySubsystem* Subsystem = GetWorld()->GetSubsystem<UCosmicGravitySubsystem>())
    {
        Subsystem->UnregisterBody(this);
        Subsystem->RegisterBody(this);
    }
}

float UCosmicGravityComponent::GetObjectRadius() const
{
    // GetActorBounds returns full actor AABB (axis-aligned bounding box),
    // including child components. BoxExtent contains half-extents along each axis (X, Y, Z).
    // Maximum value across the three axes is taken to approximate bounding sphere radius,
    // tolerating non-perfectly spherical geometries without underestimating them.
    FVector Origin, BoxExtent;
    GetOwner()->GetActorBounds(true, Origin, BoxExtent);

    return BoxExtent.GetMax();
}