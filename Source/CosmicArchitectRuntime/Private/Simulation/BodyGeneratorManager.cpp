#include "Simulation/BodyGeneratorManager.h"
#include "Kismet/KismetMathLibrary.h"

ABodyGeneratorManager::ABodyGeneratorManager()
{
    PrimaryActorTick.bCanEverTick = false;

    // Configurar el volumen
    VolumenGeneracion = CreateDefaultSubobject<UBoxComponent>(TEXT("VolumenGeneracion"));
    RootComponent = VolumenGeneracion;
    VolumenGeneracion->SetBoxExtent(FVector(500.f, 500.f, 100.f)); // Tamaño por defecto
    VolumenGeneracion->SetLineThickness(5.0f);

    // Valores por defecto
    CantidadCuerpos = 10;
    Semilla = 12345;
}

void ABodyGeneratorManager::GenerarCuerpos()
{
    // 1. Limpiar generación anterior para no acumular basura
    LimpiarCuerpos();

    if (!ClaseAGenerar)
    {
        UE_LOG(LogTemp, Warning, TEXT("¡No has seleccionado ninguna clase para generar!"));
        return;
    }

    // 2. Inicializar el flujo aleatorio con la Semilla
    FRandomStream Stream(Semilla);

    // 3. Bucle de generación
    for (int32 i = 0; i < CantidadCuerpos; i++)
    {
        // Calcular posición aleatoria RELATIVA dentro de la caja usando el Stream
        // GetRandomPointInBox garantiza que usa nuestra semilla, no el tiempo del sistema
        FVector Origen = VolumenGeneracion->GetComponentLocation();
        FVector Extents = VolumenGeneracion->GetScaledBoxExtent();

        // Matemáticas manuales para usar el Stream determinista
        FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBox(Origen, Extents);

        // Si queremos ser PURISTAS con la semilla, lo haríamos así manualmente:
        float RandX = Stream.FRandRange(-Extents.X, Extents.X);
        float RandY = Stream.FRandRange(-Extents.Y, Extents.Y);
        float RandZ = Stream.FRandRange(-Extents.Z, Extents.Z);

        // Transformar la posición relativa a la rotación del actor manager
        FVector LocalPoint = FVector(RandX, RandY, RandZ);
        FVector WorldPoint = GetActorTransform().TransformPositionNoScale(LocalPoint); // O usar solo location si no rotas la caja interna

        // Rotación aleatoria (también basada en semilla)
        FRotator RandomRot = FRotator(Stream.FRandRange(0, 360), Stream.FRandRange(0, 360), 0);

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        // 4. Spawneamos el actor
        AActor* NuevoCuerpo = GetWorld()->SpawnActor<AActor>(ClaseAGenerar, WorldPoint, RandomRot, SpawnParams);

        if (NuevoCuerpo)
        {
            // (Opcional) Si quieres que sean hijos de este manager en el Outliner
            // NuevoCuerpo->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

            CuerposGenerados.Add(NuevoCuerpo);
        }
    }
}

void ABodyGeneratorManager::LimpiarCuerpos()
{
    for (AActor* Actor : CuerposGenerados)
    {
        if (Actor && IsValid(Actor))
        {
            Actor->Destroy();
        }
    }
    CuerposGenerados.Empty();
}