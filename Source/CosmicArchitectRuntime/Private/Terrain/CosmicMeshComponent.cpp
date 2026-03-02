// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain/CosmicMeshComponent.h"
#include "CosmicNoiseSettings.h"

void UCosmicMeshComponent::BuildBaseMesh()
{
    //UE_LOG(LogTemp, Warning, TEXT("UClipmapMeshComponent::BuildBaseMesh() iniciado"));

    const int32 VertRes = Resolution + 1;
    const int32 TotalVertices = VertRes * VertRes;
    const int32 HalfRes = Resolution / 2;

    //UE_LOG(LogTemp, Warning, TEXT("  Resolución: %d"), Resolution);
    //UE_LOG(LogTemp, Warning, TEXT("  VertRes: %d"), VertRes);
    //UE_LOG(LogTemp, Warning, TEXT("  Total vértices esperados: %d"), TotalVertices);

    // 1. LIMPIAR TODO primero
    ClearAllMeshSections();

    BaseVertices.Empty();
    BaseNormals.Empty();
    BaseTangents.Empty();
    UVs.Empty();
    Triangles.Empty();
    //HeightOffsets.Empty();
    CurrentVertices.Empty();
    CurrentNormals.Empty();
    CurrentTangents.Empty();

    // 2. INICIALIZAR con tamaño correcto
    //HeightOffsets.Init(0.0f, TotalVertices);

    // 3. CALCULAR VÉRTICES
    int32 ActualVerticesCalculated = 0;

    for (int32 y = 0; y < VertRes; ++y)
    {
        for (int32 x = 0; x < VertRes; ++x)
        {
            float WorldX = (x - HalfRes) * GridSpacing;
            float WorldY = (y - HalfRes) * GridSpacing;

            // Calcular posición en esfera
            FVector SphereCenter = FVector(0, 0, -PlanetRadius);
            float Distance2D = FMath::Sqrt(WorldX * WorldX + WorldY * WorldY);
            FVector BasePosition;

            if (Distance2D <= PlanetRadius && Distance2D > 0.001f) // Evitar división por 0
            {
                float ZOffset = FMath::Sqrt(PlanetRadius * PlanetRadius - Distance2D * Distance2D);
                BasePosition = FVector(WorldX, WorldY, -PlanetRadius + ZOffset);
            }
            else if (Distance2D <= 0.001f)
            {
                // Centro - evitar NaN
                BasePosition = FVector(0, 0, 0);
            }
            else
            {
                float Scale = PlanetRadius / Distance2D;
                BasePosition = FVector(WorldX * Scale, WorldY * Scale, -PlanetRadius);
            }

            BaseVertices.Add(BasePosition);
            ActualVerticesCalculated++;

            // Normal
            FVector Normal = (BasePosition - SphereCenter);
            if (Normal.SizeSquared() > 0.001f)
            {
                Normal.Normalize();
            }
            else
            {
                Normal = FVector::UpVector;
            }
            BaseNormals.Add(Normal);

            // Tangente
            FVector TangentDir = FVector(-Normal.Y, Normal.X, 0);
            if (TangentDir.SizeSquared() > 0.001f)
            {
                TangentDir.Normalize();
            }
            else
            {
                TangentDir = FVector(1, 0, 0);
            }
            BaseTangents.Add(FProcMeshTangent(TangentDir.X, TangentDir.Y, TangentDir.Z));

            // UVs
            UVs.Add(FVector2D(
                (float)x / Resolution,
                (float)y / Resolution
            ));
        }
    }

    //UE_LOG(LogTemp, Warning, TEXT("  Vértices calculados: %d"), ActualVerticesCalculated);

    // 4. CALCULAR TRIÁNGULOS (CORREGIDO)
    Triangles.Empty();
    int32 TriangleCount = 0;

    for (int32 y = 0; y < Resolution; ++y)
    {
        for (int32 x = 0; x < Resolution; ++x)
        {
            // Lógica del anillo
            if (bIsRing)
            {
                bool bInsideInner =
                    x > HalfRes / 2 &&
                    x < Resolution - HalfRes / 2 &&
                    y > HalfRes / 2 &&
                    y < Resolution - HalfRes / 2;

                if (bInsideInner)
                {
                    continue;
                }
            }

            // Índices de vértices
            int32 i0 = y * VertRes + x;
            int32 i1 = i0 + 1;
            int32 i2 = i0 + VertRes;
            int32 i3 = i2 + 1;

            // VERIFICAR que los índices sean válidos
            if (i0 < TotalVertices && i1 < TotalVertices &&
                i2 < TotalVertices && i3 < TotalVertices)
            {
                // Primer triángulo (i0, i2, i1)
                Triangles.Add(i0);
                Triangles.Add(i2);
                Triangles.Add(i1);
                TriangleCount++;

                // Segundo triángulo (i1, i2, i3)
                Triangles.Add(i1);
                Triangles.Add(i2);
                Triangles.Add(i3);
                TriangleCount++;
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Índice de triángulo inválido en [%d,%d]"), x, y);
            }
        }
    }

    //UE_LOG(LogTemp, Warning, TEXT("  Triángulos calculados: %d"), TriangleCount / 3);
    //UE_LOG(LogTemp, Warning, TEXT("  BaseVertices.Num(): %d"), BaseVertices.Num());
    //UE_LOG(LogTemp, Warning, TEXT("  UVs.Num(): %d"), UVs.Num());
    //UE_LOG(LogTemp, Warning, TEXT("  Triangles.Num(): %d"), Triangles.Num());

    // 5. COPIAR a Current arrays
    CurrentVertices = BaseVertices;
    CurrentNormals = BaseNormals;
    CurrentTangents = BaseTangents;

    // 6. CREAR LA MALLA por primera vez
    
    double CreateStartTime = FPlatformTime::Seconds();

    CreateMeshSection_LinearColor(
        0,                    // SectionIndex
        CurrentVertices,      // Vértices
        Triangles,           // Triángulos
        CurrentNormals,      // Normales
        UVs,                 // UVs
        TArray<FLinearColor>(),    // Colores de vértice
        CurrentTangents,     // Tangentes
        //LevelIndex == 0      // Crear colisión
        false
    );

    double CreateEndTime = FPlatformTime::Seconds();

    //UE_LOG(LogTemp, Warning, TEXT("CreateMeshSection tomo: %.4f ms"), (CreateEndTime - CreateStartTime) * 1000.0);

    // 7. VERIFICAR que se creó correctamente
    if (GetNumSections() > 0)
    {
        bMeshCreated = true;
        //UE_LOG(LogTemp, Warning, TEXT("Malla creada exitosamente. Secciones: %d"), GetNumSections());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FALLÓ la creación de la malla!"));
    }


    SetCollisionEnabled(/*LevelIndex == 0 ? ECollisionEnabled::QueryAndPhysics : */ECollisionEnabled::NoCollision);
          
    bMeshCreated = true;
}




void UCosmicMeshComponent::BuildSphereMesh()
{
    ClearAllMeshSections();

    BaseVertices.Empty();
    BaseNormals.Empty();
    BaseTangents.Empty();
    UVs.Empty();
    Triangles.Empty();
    //HeightOffsets.Empty();
    CurrentVertices.Empty();
    CurrentNormals.Empty();
    CurrentTangents.Empty();

    // Aseguramos múltiplo de 2
    Resolution = FMath::Max(4, Resolution & ~1);

    const int32 LatSegments = Resolution;
    const int32 LonSegments = Resolution * 2;

    const int32 VertResX = LonSegments + 1;
    const int32 VertResY = LatSegments + 1;

    const int32 TotalVertices = VertResX * VertResY;

    BaseVertices.Reserve(TotalVertices);
    BaseNormals.Reserve(TotalVertices);
    BaseTangents.Reserve(TotalVertices);
    UVs.Reserve(TotalVertices);

    // 1. VÉRTICES
    for (int32 y = 0; y < VertResY; ++y)
    {
        float V = (float)y / LatSegments;
        float Theta = V * PI; // 0..PI

        float SinTheta = FMath::Sin(Theta);
        float CosTheta = FMath::Cos(Theta);

        for (int32 x = 0; x < VertResX; ++x)
        {
            float U = (float)x / LonSegments;
            float Phi = U * PI * 2.f; // 0..2PI

            float SinPhi = FMath::Sin(Phi);
            float CosPhi = FMath::Cos(Phi);

            FVector Normal(
                SinTheta * CosPhi,
                SinTheta * SinPhi,
                CosTheta
            );

            FVector Position = Normal * PlanetRadius;

            BaseVertices.Add(Position);
            BaseNormals.Add(Normal);

            FVector Tangent = FVector(-SinPhi, CosPhi, 0.f);
            Tangent.Normalize();
            BaseTangents.Add(FProcMeshTangent(Tangent, false));

            UVs.Add(FVector2D(U, V));
        }
    }

    // 2. TRIÁNGULOS
    for (int32 y = 0; y < LatSegments; ++y)
    {
        for (int32 x = 0; x < LonSegments; ++x)
        {
            int32 i0 = y * VertResX + x;
            int32 i1 = i0 + 1;
            int32 i2 = i0 + VertResX;
            int32 i3 = i2 + 1;

            // Orden antihorario desde fuera
            Triangles.Add(i0);
            Triangles.Add(i1);  
            Triangles.Add(i2);  

            Triangles.Add(i1);
            Triangles.Add(i3);  
            Triangles.Add(i2);  
        }
    }

    // 5. COPIAR a Current arrays
    CurrentVertices = BaseVertices;
    CurrentNormals = BaseNormals;
    CurrentTangents = BaseTangents;

    // 6. CREAR LA MALLA por primera vez

    double CreateStartTime = FPlatformTime::Seconds();

    CreateMeshSection_LinearColor(
        0,                    // SectionIndex
        CurrentVertices,      // Vértices
        Triangles,           // Triángulos
        CurrentNormals,      // Normales
        UVs,                 // UVs
        TArray<FLinearColor>(),    // Colores de vértice
        CurrentTangents,     // Tangentes
        false      // Crear colisión
    );

    double CreateEndTime = FPlatformTime::Seconds();

    //UE_LOG(LogTemp, Warning, TEXT("CreateSphereMesh tomo: %.4f ms"), (CreateEndTime - CreateStartTime) * 1000.0);

    // 7. VERIFICAR que se creó correctamente
    if (GetNumSections() > 0)
    {
        bMeshCreated = true;
        //UE_LOG(LogTemp, Warning, TEXT("Malla creada exitosamente. Secciones: %d"), GetNumSections());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FALLÓ la creación de la malla!"));
    }


    SetCollisionEnabled(ECollisionEnabled::NoCollision);

    bMeshCreated = true;
}

void UCosmicMeshComponent::RegenerateLevel(float newGridSpacing)
{
    if (!bMeshCreated)
    {
        UE_LOG(LogTemp, Error, TEXT("RegenerateLevel() llamado pero bMeshCreated = false"));
        return;
    }

    GridSpacing = newGridSpacing;

    const int32 VertRes = Resolution + 1;
    const int32 HalfRes = Resolution / 2;

    FVector SphereCenter = FVector(0, 0, -PlanetRadius);

    for (int32 y = 0; y < VertRes; ++y)
    {
        for (int32 x = 0; x < VertRes; ++x)
        {
            const int32 Index = x + y * VertRes;

            float WorldX = (x - HalfRes) * GridSpacing;
            float WorldY = (y - HalfRes) * GridSpacing;

            float Distance2D = FMath::Sqrt(WorldX * WorldX + WorldY * WorldY);
            FVector BasePosition;

            if (Distance2D <= PlanetRadius && Distance2D > 0.001f)
            {
                float ZOffset = FMath::Sqrt(PlanetRadius * PlanetRadius - Distance2D * Distance2D);
                BasePosition = FVector(WorldX, WorldY, -PlanetRadius + ZOffset);
            }
            else if (Distance2D <= 0.001f)
            {
                BasePosition = FVector(0, 0, 0);
            }
            else
            {
                float Scale = PlanetRadius / Distance2D;
                BasePosition = FVector(WorldX * Scale, WorldY * Scale, -PlanetRadius);
            }

            BaseVertices[Index] = BasePosition;

            // Normal
            FVector Normal = BasePosition - SphereCenter;
            if (Normal.SizeSquared() > 0.001f)
            {
                Normal.Normalize();
            }
            else
            {
                Normal = FVector::UpVector;
            }

            BaseNormals[Index] = Normal;

            // Tangente
            FVector TangentDir = FVector(-Normal.Y, Normal.X, 0);
            if (TangentDir.SizeSquared() > 0.001f)
            {
                TangentDir.Normalize();
            }
            else
            {
                TangentDir = FVector(1, 0, 0);
            }

            BaseTangents[Index] = FProcMeshTangent(TangentDir, false);

            // UV
            UVs[Index] = FVector2D(
                (float)x / Resolution,
                (float)y / Resolution
            );
        }
    }

    //CurrentVertices = BaseVertices;
    CurrentNormals = BaseNormals;
    CurrentTangents = BaseTangents;

    //UpdateMeshSection_LinearColor(
    //    0,
    //    CurrentVertices,
    //    CurrentNormals,
    //    UVs,
    //    TArray<FLinearColor>(),
    //    CurrentTangents
    //);

    //SetCollisionEnabled(/*LevelIndex == 0 ? ECollisionEnabled::QueryAndPhysics :*/ ECollisionEnabled::NoCollision);
}

void UCosmicMeshComponent::UpdateMesh()
{
    //double UpdateMeshStartTime = FPlatformTime::Seconds();

    if (!bMeshCreated)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateMesh() llamado pero bMeshCreated = false"));
        UE_LOG(LogTemp, Error, TEXT("Llamando a BuildBaseMesh() primero..."));
        //BuildBaseMesh();
        return;
    }

    // VERIFICAR tamaños antes de actualizar
    //UE_LOG(LogTemp, Warning, TEXT("UpdateMesh() verificando:"));
    //UE_LOG(LogTemp, Warning, TEXT("  CurrentVertices: %d"), CurrentVertices.Num());
    //UE_LOG(LogTemp, Warning, TEXT("  CurrentNormals: %d"), CurrentNormals.Num());
    //UE_LOG(LogTemp, Warning, TEXT("  UVs: %d"), UVs.Num());
    //UE_LOG(LogTemp, Warning, TEXT("  CurrentTangents: %d"), CurrentTangents.Num());

    const FVector2D origin = FVector2D(LevelIndex, Resolution);

    UpdateHeights(origin);

    if (CurrentVertices.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("CurrentVertices está vacío! Copiando de BaseVertices"));
        CurrentVertices = BaseVertices;
        CurrentNormals = BaseNormals;
        CurrentTangents = BaseTangents;
    }

    // ACTUALIZAR la malla existente 
    // Tiempo actualizar el nivel 0 que tiene colisión: 32*32 1.2ms, 64*64 5-6ms, 128*128 21ms
    UpdateMeshSection_LinearColor(
        0,
        CurrentVertices,
        CurrentNormals,
        UVs,
        TArray<FLinearColor>(),
        CurrentTangents
    );

    SetCollisionEnabled(/*LevelIndex == 0 ? ECollisionEnabled::QueryAndPhysics :*/ ECollisionEnabled::NoCollision);

    /*double UpdateMeshEndTime = FPlatformTime::Seconds();
    double UpdateMeshTime = UpdateMeshEndTime - UpdateMeshStartTime;

    UE_LOG(LogTemp, Warning, TEXT("Malla actualizada en %.4f ms"), UpdateMeshTime * 1000.0);*/
}

void UCosmicMeshComponent::SetMeshActive(bool active)
{
    bActiveMesh = active;
    SetMeshSectionVisible(0, active);
}


void UCosmicMeshComponent::UpdateHeights(const FVector2D& Origin)
{

    //De prueba
    for (size_t i = 0; i < BaseVertices.Num(); i++)
    {
        CurrentVertices[i] = BaseVertices[i] + BaseNormals[i] * Origin.X * Origin.Y * 0;
    }
}

void UCosmicMeshComponent::RequestMeshUpdate()
{
    if (!bMeshCreated || bIsGeneratingNoise) return;

    bIsGeneratingNoise = true;

    // Centro del planeta (asumiendo que el dueño es el actor central)
    FVector PlanetCenter = GetOwner()->GetActorLocation();

    int32 Seed;
    TArray<FCosmicNoiseTypes> ResolvedLayers;
    bool bResolvedWarp;
    float ResolvedWarpStrength, ResolvedWarpFrequency;

    if (NoiseSettings)
    {
        Seed = NoiseSettings->Seed;
        ResolvedLayers = NoiseSettings->NoiseLayers;
        bResolvedWarp = NoiseSettings->bUseDomainWarp;
        ResolvedWarpStrength = NoiseSettings->DomainWarpStrength;
        ResolvedWarpFrequency = NoiseSettings->DomainWarpFrequency;
    }
    else
    {
        // Valores por defecto
        Seed = 1337;
        ResolvedLayers.Empty();
        bResolvedWarp = false;
        ResolvedWarpStrength = 0.0f;
        ResolvedWarpFrequency = 0.0f;
    }

    

    // Lanzar la tarea asíncrona
    NoiseTask = new FAsyncTask<FCosmicArchitectNoiseGenerator>(
        BaseVertices,
        BaseNormals,
        GetComponentTransform(),
        PlanetCenter,
        Seed,
        ResolvedLayers,
        bResolvedWarp,
        ResolvedWarpStrength,
        ResolvedWarpFrequency
    );

    NoiseTask->StartBackgroundTask();
}

bool UCosmicMeshComponent::CheckAndApplyMeshUpdate()
{
    // Si no hay tarea o no ha terminado, devolvemos false
    if (!NoiseTask || !NoiseTask->IsDone()) return false;

    // Copiamos los vértices calculados del hilo secundario a nuestro array principal
    CurrentVertices = NoiseTask->GetTask().CalculatedVertices;

    // Limpiamos la memoria de la tarea
    delete NoiseTask;
    NoiseTask = nullptr;
    bIsGeneratingNoise = false;

    // Actualizamos la sección de la malla (Esto ocurre instantáneamente en el Game Thread)
    UpdateMeshSection_LinearColor(
        0,
        CurrentVertices,
        CurrentNormals,
        UVs,
        TArray<FLinearColor>(),
        CurrentTangents
    );

    SetCollisionEnabled(ECollisionEnabled::NoCollision);

    return true; // La malla se ha actualizado
}