// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain/CosmicMeshComponent.h"
#include "CosmicNoiseSettings.h"

void UCosmicMeshComponent::BuildBaseMesh()
{
    // 1. NUEVA RESOLUCIÓN (Resolution - 1)
    const int32 VertRes = Resolution - 1;
    const int32 TotalVertices = VertRes * VertRes;
    const int32 HalfRes = Resolution / 2;

    // Los quads generados serán VertRes - 1 (es decir, Resolution - 2)
    const int32 QuadRes = VertRes - 1;

    ClearAllMeshSections();

    BaseVertices.Empty(TotalVertices);
    BaseNormals.Empty(TotalVertices);
    BaseTangents.Empty(TotalVertices);
    UVs.Empty(TotalVertices);
    Triangles.Empty();
    CurrentVertices.Empty();
    CurrentNormals.Empty();
    CurrentTangents.Empty();

    // 2. CALCULAR VÉRTICES
    int32 ActualVerticesCalculated = 0;

    for (int32 y = 0; y < VertRes; ++y)
    {
        for (int32 x = 0; x < VertRes; ++x)
        {
            // Al iterar hasta VertRes (que es menor), y mantener HalfRes de la Resolution original,
            // la malla entera queda geométricamente desplazada 1 unidad hacia Arriba/Izquierda (-X, -Y).
            float WorldX = (x - HalfRes) * GridSpacing;
            float WorldY = (y - HalfRes) * GridSpacing;

            FVector SphereCenter = FVector(0, 0, -PlanetRadius);
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

            BaseVertices.Add(BasePosition);
            ActualVerticesCalculated++;

            // Normal
            FVector Normal = (BasePosition - SphereCenter);
            if (Normal.SizeSquared() > 0.001f) Normal.Normalize();
            else Normal = FVector::UpVector;
            BaseNormals.Add(Normal);

            // Tangente
            FVector TangentDir = FVector(-Normal.Y, Normal.X, 0);
            if (TangentDir.SizeSquared() > 0.001f) TangentDir.Normalize();
            else TangentDir = FVector(1, 0, 0);
            BaseTangents.Add(FProcMeshTangent(TangentDir.X, TangentDir.Y, TangentDir.Z));

            // UVs - Mantenemos la división por Resolution para no alterar la escala de texturas respecto al LOD anterior
            UVs.Add(FVector2D((float)x / Resolution, (float)y / Resolution));
        }
    }

    // 3. CALCULAR TRIÁNGULOS CON NUEVOS LÍMITES
    int32 TriangleCount = 0;

    // Iteramos hasta QuadRes (Resolution - 2)
    for (int32 y = 0; y < QuadRes; ++y)
    {
        for (int32 x = 0; x < QuadRes; ++x)
        {
            int32 i0 = y * VertRes + x;
            int32 i1 = i0 + 1;
            int32 i2 = i0 + VertRes;
            int32 i3 = i2 + 1;

            if (bIsRing)
            {
                // Desplazamos el hueco interior 1 unidad Arriba/Izquierda para que encaje 
                // con el Shift natural de la cuadrícula
                int32 HoleOffset = -1;
                int32 HoleMin = (HalfRes / 2) + HoleOffset;
                int32 HoleMax = (Resolution - HalfRes / 2) + HoleOffset;

                bool bInsideInner = x > HoleMin && x < HoleMax && y > HoleMin && y < HoleMax;

                if (bInsideInner) continue;
            }

            if (i0 >= TotalVertices || i1 >= TotalVertices || i2 >= TotalVertices || i3 >= TotalVertices)
            {
                UE_LOG(LogTemp, Error, TEXT("Índice de triángulo inválido en [%d,%d]"), x, y);
                continue;
            }

            // Detectar bordes usando QuadRes
            bool bBorder = (x == 0) || (x == QuadRes - 1) || (y == 0) || (y == QuadRes - 1);

            if (bBorder)
            {
                // Bordes horizontales
                if ((y == 0 || y == QuadRes - 1) && (x % 2 == 0) && x < QuadRes - 1)
                {
                    int32 i4 = i1 + 1;
                    int32 i5 = i3 + 1;

                    if (i4 < TotalVertices)
                    {
                        if (y == QuadRes - 1) // Borde inferior
                        {
                            if (x != QuadRes - 2) {
                                Triangles.Add(i1); Triangles.Add(i5); Triangles.Add(i4); TriangleCount++;
                            }
                            if (x != 0) {
                                Triangles.Add(i1); Triangles.Add(i0); Triangles.Add(i2); TriangleCount++;
                            }
                            Triangles.Add(i2); Triangles.Add(i5); Triangles.Add(i1); TriangleCount++;
                        }
                        else // Borde superior
                        {
                            if (x != 0) {
                                Triangles.Add(i0); Triangles.Add(i2); Triangles.Add(i3); TriangleCount++;
                            }
                            if (x != QuadRes - 2) {
                                Triangles.Add(i3); Triangles.Add(i5); Triangles.Add(i4); TriangleCount++;
                            }
                            Triangles.Add(i0); Triangles.Add(i3); Triangles.Add(i4); TriangleCount++;
                        }
                    }
                }
                // Bordes verticales
                else if ((x == 0 || x == QuadRes - 1) && (y % 2 == 0) && y < QuadRes - 1)
                {
                    int32 i4 = i2 + VertRes;
                    int32 i5 = i3 + VertRes;

                    if (i4 < TotalVertices)
                    {
                        if (x == QuadRes - 1) // Borde derecho
                        {
                            Triangles.Add(i1); Triangles.Add(i2); Triangles.Add(i5); TriangleCount++;

                            if (y != 0) {
                                Triangles.Add(i2); Triangles.Add(i1); Triangles.Add(i0); TriangleCount++;
                            }
                            if (y != QuadRes - 2) {
                                Triangles.Add(i2); Triangles.Add(i4); Triangles.Add(i5); TriangleCount++;
                            }
                        }
                        else // Borde izquierdo
                        {
                            if (y != 0) {
                                Triangles.Add(i0); Triangles.Add(i3); Triangles.Add(i1); TriangleCount++;
                            }
                            if (y != QuadRes - 2) {
                                Triangles.Add(i3); Triangles.Add(i4); Triangles.Add(i5); TriangleCount++;
                            }
                            Triangles.Add(i0); Triangles.Add(i4); Triangles.Add(i3); TriangleCount++;
                        }
                    }
                }
            }
            else
            {
                // Interior normal
                Triangles.Add(i0); Triangles.Add(i2); Triangles.Add(i1); TriangleCount++;
                Triangles.Add(i1); Triangles.Add(i2); Triangles.Add(i3); TriangleCount++;
            }
        }
    }


    UE_LOG(LogTemp, Warning, TEXT("  Triángulos calculados: %d"), TriangleCount);
    //UE_LOG(LogTemp, Warning, TEXT("  BaseVertices.Num(): %d"), BaseVertices.Num());
    //UE_LOG(LogTemp, Warning, TEXT("  UVs.Num(): %d"), UVs.Num());
    //UE_LOG(LogTemp, Warning, TEXT("  Triangles.Num(): %d"), Triangles.Num());

    // 5. COPIAR a Current arrays
    CurrentVertices = BaseVertices;
    CurrentNormals = BaseNormals;
    CurrentTangents = BaseTangents;

    // 6. CREAR LA MALLA por primera vez
    
    //double CreateStartTime = FPlatformTime::Seconds();

    // Inicializamos el array de colores con el mismo tamaño que los vértices
    CurrentColors.Init(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f), CurrentVertices.Num());

    CreateMeshSection_LinearColor(
        0,                    // SectionIndex
        CurrentVertices,      // Vértices
        Triangles,           // Triángulos
        CurrentNormals,      // Normales
        UVs,                 // UVs
        CurrentColors,    // Colores de vértice
        CurrentTangents,     // Tangentes
        //LevelIndex == 0      // Crear colisión
        false
    );

    //double CreateEndTime = FPlatformTime::Seconds();

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

    // Inicializamos el array de colores con el mismo tamaño que los vértices
    CurrentColors.Init(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f), CurrentVertices.Num());

    CreateMeshSection_LinearColor(
        0,                    // SectionIndex
        CurrentVertices,      // Vértices
        Triangles,           // Triángulos
        CurrentNormals,      // Normales
        UVs,                 // UVs
        CurrentColors,    // Colores de vértice
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

    // 1. APLICAMOS EL NUEVO TAMAÑO (Resolution - 1)
    const int32 VertRes = Resolution - 1;
    const int32 HalfRes = Resolution / 2;

    FVector SphereCenter = FVector(0, 0, -PlanetRadius);

    for (int32 y = 0; y < VertRes; ++y)
    {
        for (int32 x = 0; x < VertRes; ++x)
        {
            // El índice encajará perfectamente porque los arrays se inicializaron
            // con (Resolution - 1) * (Resolution - 1) en BuildBaseMesh()
            const int32 Index = x + y * VertRes;

            // Al mantener HalfRes (Resolution / 2) pero iterar hasta VertRes, 
            // se preserva el "Shift" hacia Arriba/Izquierda con el nuevo GridSpacing
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

            // UV - Mantenemos la misma escala proporcional
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

void UCosmicMeshComponent::SetMeshActive(bool active)
{
    bActiveMesh = active;
    SetMeshSectionVisible(0, active);
}



void UCosmicMeshComponent::RequestMeshUpdate()
{
    if (!bMeshCreated || bIsGeneratingNoise || !NoiseSettings) return;

    bIsGeneratingNoise = true;

    // Centro del planeta 
    FVector PlanetCenter = GetOwner()->GetActorLocation();

    NoiseTask = new FAsyncTask<FCosmicArchitectNoiseGenerator>(
        BaseVertices,
        BaseNormals,
        GetComponentTransform(),
        PlanetCenter,
        NoiseSettings->Params
    );
    // Lanzar la tarea asincrona
    NoiseTask->StartBackgroundTask();
}

bool UCosmicMeshComponent::CheckAndApplyMeshUpdate()
{
    // Si no hay tarea o no ha terminado, devolvemos false
    if (!NoiseTask || !NoiseTask->IsDone()) return false;

    // Copiamos los vértices calculados del hilo secundario a nuestro array principal
    CurrentVertices = NoiseTask->GetTask().CalculatedVertices;
    CurrentColors = NoiseTask->GetTask().CalculatedColors;

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
        CurrentColors,
        CurrentTangents
    );

    SetCollisionEnabled(ECollisionEnabled::NoCollision);

    return true; // La malla se ha actualizado
}