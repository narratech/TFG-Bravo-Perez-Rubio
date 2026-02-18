#include "System/CosmicSystemGenerator.h"
#include "Kismet/KismetMathLibrary.h"
#include "UObject/ConstructorHelpers.h"

ACosmicSystemGenerator::ACosmicSystemGenerator()
{
    // E: Desactivamos el Tick porque no necesitamos actualizaciones por frame.
    // I: Disable Tick as we don't need per-frame updates.
    PrimaryActorTick.bCanEverTick = false;

    GenerationVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("GenerationVolume"));
    RootComponent = GenerationVolume;
    GenerationVolume->SetLineThickness(20.0f);

    // E: Inicialización de variables por defecto.
    // I: Default variable initialization.
    VolumeSizeKm = FVector(20.0f, 20.0f, 5.0f); // 20 Km
    NumberOfBodies = 10;
    Seed = 12345;

    // E: Rango de diámetro por defecto: entre 100 metros (0.1 km) y 500 metros (0.5 km).
    // I: Default diameter range: between 100 meters (0.1 km) and 500 meters (0.5 km).
    BodyDiameterRangeKm = FVector2D(0.1f, 0.5f);

    MinDistanceBetweenBodies = 1.0f; // 1 Km
    MaxDistanceToNearest = 0.0f;     // 0 = Sin agrupación forzada / No forced clustering       
    MaxGenerationAttempts = 100;

    // E: Buscamos la esfera básica del motor para tener algo asignado por defecto.
    // I: Find the engine's basic sphere to have something assigned by default.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultSphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (DefaultSphereAsset.Succeeded())
    {
        SphereMesh = DefaultSphereAsset.Object;
    }
}

void ACosmicSystemGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform); // O usa AActor::OnConstruction(Transform) si tienes problemas con macros

    // E: Actualizamos el tamaño visual de la caja cuando cambiamos el valor en el editor.
    // I: Update the visual box size when changing the value in the editor.
    if (GenerationVolume)
    {
        // E: Convertimos Km a Unidades de Unreal (cm). 1 Km = 100,000 cm.
        // I: Convert Km to Unreal Units (cm). 1 Km = 100,000 cm.
        GenerationVolume->SetBoxExtent(VolumeSizeKm * 100000.0f);
    }
}

void ACosmicSystemGenerator::GenerateBodies()
{
    // E: Limpiamos generación anterior antes de empezar.
    // I: Clear previous generation before starting.
    ClearBodies();

    if (!SphereMesh)
    {
        UE_LOG(LogTemp, Error, TEXT("ACosmicSystemGenerator: No SphereMesh assigned!"));
        return;
    }

    FRandomStream Stream(Seed);
    FVector BoxExtents = GenerationVolume->GetScaledBoxExtent();

    int32 BodiesSpawned = 0;

    // E: Bucle principal de generación.
    // I: Main generation loop.
    for (int32 i = 0; i < NumberOfBodies; i++)
    {
        FVector CandidateLocation;
        bool bValidPosition = false;
        int32 Attempts = 0;

        // E: Algoritmo de 'Rejection Sampling': Intentamos encontrar una posición válida varias veces.
        // I: 'Rejection Sampling' algorithm: Try to find a valid position multiple times.
        while (!bValidPosition && Attempts < MaxGenerationAttempts)
        {
            Attempts++;

            // E: 1. Generar punto aleatorio dentro de la caja.
            // I: 1. Generate random point inside the box.
            float RandX = Stream.FRandRange(-BoxExtents.X, BoxExtents.X);
            float RandY = Stream.FRandRange(-BoxExtents.Y, BoxExtents.Y);
            float RandZ = Stream.FRandRange(-BoxExtents.Z, BoxExtents.Z);

            FVector LocalPoint = FVector(RandX, RandY, RandZ);
            CandidateLocation = GetActorTransform().TransformPosition(LocalPoint);

            // E: 2. Verificar reglas de distancia.
            // I: 2. Check distance rules.
            bool bTooClose = false;
            bool bFarFromEveryone = true;

            // E: El primer cuerpo siempre es válido.
            // I: The first body is always valid.
            if (GeneratedBodies.Num() == 0)
            {
                bValidPosition = true;
                break;
            }

            // E: Iteramos sobre los cuerpos existentes para comprobar distancias.
            // I: Iterate over existing bodies to check distances.
            for (AActor* ExistingBody : GeneratedBodies)
            {
                if (!ExistingBody) continue;

                // E: Usamos DistSquared para optimizar rendimiento (evita raíz cuadrada), pero para claridad usamos Dist normal aquí.
                // I: Using DistSquared is better for performance, but using normal Dist here for clarity.
                float Dist = FVector::Dist(CandidateLocation, ExistingBody->GetActorLocation());

                // E: Convertir distancia de Unreal Units (cm) a Kilómetros para comparar con nuestros parámetros.
                // I: Convert Unreal Units (cm) distance to Kilometers to compare with our parameters.
                float DistKm = Dist / 100000.0f;

                // E: Regla de Distancia Mínima (Colisión).
                // I: Minimum Distance Rule (Collision).
                if (DistKm < MinDistanceBetweenBodies)
                {
                    bTooClose = true;
                    break;
                }

                // E: Regla de Distancia Máxima (Agrupamiento).
                // I: Maximum Distance Rule (Clustering).
                if (MaxDistanceToNearest > 0.0f && DistKm <= MaxDistanceToNearest)
                {
                    bFarFromEveryone = false;
                }
            }

            // E: Validación final del intento.
            // I: Final attempt validation.
            if (bTooClose)
            {
                bValidPosition = false;
            }
            else
            {
                // E: Si MaxDistance es 0, ignoramos esa regla. Si no, debe tener un vecino cerca.
                // I: If MaxDistance is 0, ignore that rule. Otherwise, it must have a neighbor nearby.
                if (MaxDistanceToNearest <= 0.0f)
                {
                    bValidPosition = true;
                }
                else
                {
                    bValidPosition = !bFarFromEveryone;
                }
            }
        }

        // E: Si encontramos una posición válida, spawneamos el actor.
        // I: If a valid position was found, spawn the actor.
        if (bValidPosition)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            AStaticMeshActor* NewBody = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), CandidateLocation, FRotator::ZeroRotator, SpawnParams);

            if (NewBody)
            {
                UStaticMeshComponent* MeshComp = NewBody->GetStaticMeshComponent();
                if (MeshComp)
                {
                    MeshComp->SetStaticMesh(SphereMesh);

                    // E: Obtener diámetro aleatorio en Kilómetros.
                    // I: Get random diameter in Kilometers.
                    float RandomDiameterKm = Stream.FRandRange(BodyDiameterRangeKm.X, BodyDiameterRangeKm.Y);

                    // E: Conversión: La esfera base mide 1m (100cm). 1 Km = 100,000 cm.
                    //    Factor de Escala = (Km * 100,000) / 100 = Km * 1000.
                    // I: Conversion: Base sphere is 1m (100cm). 1 Km = 100,000 cm.
                    //    Scale Factor = (Km * 100,000) / 100 = Km * 1000.
                    float NewScale = RandomDiameterKm * 1000.0f;

                    NewBody->SetActorScale3D(FVector(NewScale));

                    MeshComp->SetMobility(EComponentMobility::Movable);
                }

                // E: Hacemos attach para limpiar la jerarquía.
                // I: Attach to clean up hierarchy.
                NewBody->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

                GeneratedBodies.Add(NewBody);
                BodiesSpawned++;
            }
        }
        else
        {
            // E: Advertencia si no se pudo colocar un cuerpo tras los intentos.
            // I: Warning if a body could not be placed after attempts.
            UE_LOG(LogTemp, Warning, TEXT("Failed to find valid position for body %d after %d attempts."), i, MaxGenerationAttempts);
        }
    }

    UE_LOG(LogTemp, Display, TEXT("Generation Complete. Spawned: %d / %d"), BodiesSpawned, NumberOfBodies);
}

void ACosmicSystemGenerator::GenerateWithRandomSeed()
{
    // E: Generar semilla basada en tiempo del sistema y dirección de memoria.
    // I: Generate seed based on system time and memory address.
    int32 RandomSeed = 0;
    RandomSeed += static_cast<int32>(FDateTime::Now().GetTicks());
    RandomSeed += static_cast<int32>(FPlatformTime::Cycles());
    RandomSeed += reinterpret_cast<int64>(this);

    Seed = HashCombine(GetTypeHash(RandomSeed), GetTypeHash(FMath::Rand()));

    GenerateBodies();
}

void ACosmicSystemGenerator::ClearBodies()
{
    // E: Recorremos el array y destruimos los actores válidos.
    // I: Iterate through the array and destroy valid actors.
    for (AStaticMeshActor* Actor : GeneratedBodies)
    {
        if (Actor && Actor->IsValidLowLevel())
        {
            Actor->Destroy();
        }
    }
    GeneratedBodies.Empty();
}