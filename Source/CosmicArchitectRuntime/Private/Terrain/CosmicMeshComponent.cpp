// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain/CosmicMeshComponent.h"
#include "CosmicNoiseSettings.h"

void UCosmicMeshComponent::BuildBasePlainMesh()
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

    TArray<int32> Triangles;

    for (int32 y = 0; y < VertRes; ++y)
    {
        for (int32 x = 0; x < VertRes; ++x)
        {
            double LocalX = (x - HalfRes) * GridSpacing;
            double LocalY = (y - HalfRes) * GridSpacing;

            FVector Pos = FVector(LocalX, LocalY, 0);

            BaseVertices.Add(Pos);

            // Normal plana (luego se recalcula en esfera)
            BaseNormals.Add(FVector::UpVector);

            // Tangente plana
            BaseTangents.Add(FProcMeshTangent(1, 0, 0));

            UVs.Add(FVector2D(
                (float)x / (VertRes - 1),
                (float)y / (VertRes - 1)
            ));
        }
    }
    
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
                bool bIsHorizontal = (y == 0 || y == QuadRes - 1) && (x % 2 == 0) && x < QuadRes - 1;
                bool bIsVertical = (x == 0 || x == QuadRes - 1) && (y % 2 == 0) && y < QuadRes - 1;

                // bordes horizontales
                if (bIsHorizontal)
                {
                    int32 i4 = i1 + 1;
                    int32 i5 = i3 + 1;

                    if (i4 < TotalVertices)
                    {
                        if (y == QuadRes - 1) // borde inferior 
                        {
                            if (x != QuadRes - 2) {
                                Triangles.Add(i1); Triangles.Add(i5); Triangles.Add(i4);
                            }
                            if (x != 0) {
                                Triangles.Add(i1); Triangles.Add(i0); Triangles.Add(i2);
                            }
                            Triangles.Add(i2); Triangles.Add(i5); Triangles.Add(i1);
                        }
                        else // borde superior 
                        {
                            if (x != 0) {
                                Triangles.Add(i0); Triangles.Add(i2); Triangles.Add(i3);
                            }
                            if (x != QuadRes - 2) {
                                Triangles.Add(i3); Triangles.Add(i5); Triangles.Add(i4);
                            }
                            Triangles.Add(i0); Triangles.Add(i3); Triangles.Add(i4);
                        }
                    }
                }

                // bordes verticales (Fíjate que ya NO es un "else if")
                if (bIsVertical)
                {
                    int32 i4 = i2 + VertRes;
                    int32 i5 = i3 + VertRes;

                    if (i4 < TotalVertices)
                    {
                        if (x == QuadRes - 1) // borde derecho
                        {
                            Triangles.Add(i1); Triangles.Add(i2); Triangles.Add(i5);

                            if (y != 0) {
                                Triangles.Add(i2); Triangles.Add(i1); Triangles.Add(i0);
                            }
                            if (y != QuadRes - 2) {
                                Triangles.Add(i2); Triangles.Add(i4); Triangles.Add(i5);
                            }
                        }
                        else // borde izquierdo 
                        {
                            if (y != 0) {
                                Triangles.Add(i0); Triangles.Add(i3); Triangles.Add(i1);
                            }
                            if (y != QuadRes - 2) {
                                Triangles.Add(i3); Triangles.Add(i4); Triangles.Add(i5);
                            }
                            Triangles.Add(i0); Triangles.Add(i4); Triangles.Add(i3);
                        }
                    }
                }
            }
            else
            {
                // Interior normal
                Triangles.Add(i0); Triangles.Add(i2); Triangles.Add(i1);
                Triangles.Add(i1); Triangles.Add(i2); Triangles.Add(i3);
            }
        }
    }

    CreateMeshSection_LinearColor(
        0,                    // SectionIndex
        BaseVertices,      // Vértices
        Triangles,           // Triángulos
        BaseNormals,      // Normales
        UVs,                 // UVs
        TArray<FLinearColor>(),
        BaseTangents,     // Tangentes
        false
    );

    // VERIFICAR que se creó correctamente
    if (GetNumSections() > 0)
    {
        bMeshCreated = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FALLÓ la creación de la malla!"));
    }


    SetCollisionEnabled(ECollisionEnabled::NoCollision);
          
    bMeshCreated = true;
}

void UCosmicMeshComponent::BuildBaseProjectedMesh()
{
    const int32 VertRes = Resolution + 1;
    const int32 TotalVertices = VertRes * VertRes;
    const int32 HalfRes = Resolution / 2;

    ClearAllMeshSections();

    BaseVertices.Empty(TotalVertices);
    BaseNormals.Empty(TotalVertices);
    BaseTangents.Empty(TotalVertices);
    UVs.Empty(TotalVertices);

    TArray<int32> Triangles;

    for (int32 y = 0; y < VertRes; ++y)
    {
        for (int32 x = 0; x < VertRes; ++x)
        {
            double WorldX = (x - HalfRes) * GridSpacing;
            double WorldY = (y - HalfRes) * GridSpacing;

            // Calcular posición en esfera
            FVector SphereCenter = FVector(0, 0, -PlanetRadius);
            double Distance2D = FMath::Sqrt(WorldX * WorldX + WorldY * WorldY);
            FVector BasePosition;

            if (Distance2D <= PlanetRadius && Distance2D > 0.001f) // Evitar división por 0
            {
                double ZOffset = FMath::Sqrt(PlanetRadius * PlanetRadius - Distance2D * Distance2D);
                BasePosition = FVector(WorldX, WorldY, -PlanetRadius + ZOffset);
            }
            else if (Distance2D <= 0.001f)
            {
                // Centro - evitar NaN
                BasePosition = FVector(0, 0, 0);
            }
            else
            {
                double Scale = PlanetRadius / Distance2D;
                BasePosition = FVector(WorldX * Scale, WorldY * Scale, -PlanetRadius);
            }

            BaseVertices.Add(BasePosition);

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

    // CALCULAR TRIÁNGULOS 
    Triangles.Empty();
    int32 TriangleCount = 0;

    for (int32 y = 0; y < Resolution; ++y)
    {
        for (int32 x = 0; x < Resolution; ++x)
        {
            // Índices de vértices
            int32 i0 = y * VertRes + x;
            int32 i1 = i0 + 1;
            int32 i2 = i0 + VertRes;
            int32 i3 = i2 + 1;

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

            if (i0 >= TotalVertices || i1 >= TotalVertices ||
                i2 >= TotalVertices || i3 >= TotalVertices)
            {
                UE_LOG(LogTemp, Error, TEXT("Índice de triángulo inválido en [%d,%d]"), x, y);
                continue;
            }

            bool bBorder =
                (x == 0) ||
                (x == Resolution - 1) ||
                (y == 0) ||
                (y == Resolution - 1);

            // BORDE DEL NIVEL 
            if (bBorder)
            {
                // bordes horizontales
                if ((y == 0 || y == Resolution - 1) && (x % 2 == 0) && x < Resolution - 1)
                {
                    int32 i4 = i1 + 1;
                    int32 i5 = i3 + 1;

                    if (i4 < TotalVertices)
                    {
                        if (y == Resolution - 1) // borde inferior 
                        {

                            if (x != Resolution - 2) {
                                Triangles.Add(i1);
                                Triangles.Add(i5);
                                Triangles.Add(i4);
                                TriangleCount++;
                            }

                            if (x != 0) {
                                Triangles.Add(i1);
                                Triangles.Add(i0);
                                Triangles.Add(i2);
                                TriangleCount++;
                            }

                            Triangles.Add(i2);
                            Triangles.Add(i5);
                            Triangles.Add(i1);
                            TriangleCount++;
                        }
                        else // borde superior 
                        {
                            if (x != 0) {
                                Triangles.Add(i0);
                                Triangles.Add(i2);
                                Triangles.Add(i3);
                                TriangleCount++;
                            }

                            if (x != Resolution - 2) {
                                Triangles.Add(i3);
                                Triangles.Add(i5);
                                Triangles.Add(i4);
                                TriangleCount++;
                            }

                            Triangles.Add(i0);
                            Triangles.Add(i3);
                            Triangles.Add(i4);
                            TriangleCount++;
                        }
                    }
                }
                // bordes verticales
                else if ((x == 0 || x == Resolution - 1) && (y % 2 == 0) && y < Resolution - 1)
                {
                    int32 i4 = i2 + VertRes;
                    int32 i5 = i3 + VertRes;

                    if (i4 < TotalVertices)
                    {
                        if (x == Resolution - 1) // borde derecho
                        {
                            Triangles.Add(i1);
                            Triangles.Add(i2);
                            Triangles.Add(i5);
                            TriangleCount++;

                            if (y != 0) {
                                Triangles.Add(i2);
                                Triangles.Add(i1);
                                Triangles.Add(i0);
                                TriangleCount++;
                            }

                            if (y != Resolution - 2) {
                                Triangles.Add(i2);
                                Triangles.Add(i4);
                                Triangles.Add(i5);
                                TriangleCount++;
                            }
                        }
                        else // borde izquierdo 
                        {
                            if (y != 0) {
                                Triangles.Add(i0);
                                Triangles.Add(i3);
                                Triangles.Add(i1);
                                TriangleCount++;
                            }

                            if (y != Resolution - 2) {
                                Triangles.Add(i3);
                                Triangles.Add(i4);
                                Triangles.Add(i5);
                                TriangleCount++;
                            }

                            Triangles.Add(i0);
                            Triangles.Add(i4);
                            Triangles.Add(i3);
                            TriangleCount++;
                        }
                    }
                }
            }
            else
            {
                // INTERIOR NORMAL 

                Triangles.Add(i0);
                Triangles.Add(i2);
                Triangles.Add(i1);
                TriangleCount++;

                Triangles.Add(i1);
                Triangles.Add(i2);
                Triangles.Add(i3);
                TriangleCount++;
            }
        }
    }

    CreateMeshSection_LinearColor(
        0,                    // SectionIndex
        BaseVertices,      // Vértices
        Triangles,           // Triángulos
        BaseNormals,      // Normales
        UVs,                 // UVs
        TArray<FLinearColor>(),    // Colores de vértice
        BaseTangents,     // Tangentes
        false
    );

    // VERIFICAR que se creo correctamente
    if (GetNumSections() > 0)
    {
        bMeshCreated = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FALLÓ la creación de la malla!"));
    }


    SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

void UCosmicMeshComponent::BuildBaseMesh()
{
    if (bIsPlanet) {
        BuildBaseProjectedMesh();
    }
    else
    {
        BuildBasePlainMesh();
    }
}



void UCosmicMeshComponent::BuildSphereMesh()
{
    ClearAllMeshSections();

    BaseVertices.Empty();
    BaseNormals.Empty();
    BaseTangents.Empty();
    UVs.Empty();

    TArray<int32> Triangles;

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

    CreateMeshSection_LinearColor(
        0,                    // SectionIndex
        BaseVertices,      // Vértices
        Triangles,           // Triángulos
        BaseNormals,      // Normales
        UVs,                 // UVs
        TArray<FLinearColor>(),    // Colores de vértice
        BaseTangents,     // Tangentes
        false      // Crear colisión
    );

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

void UCosmicMeshComponent::ShiftLevel(FIntPoint Shift)
{
    if (Shift == FIntPoint::ZeroValue) return;

    FVector Offset = FVector(
        Shift.X * GridSpacing * 2,
        Shift.Y * GridSpacing * 2,
        0.0f
    );

    for (size_t i = 0; i < BaseVertices.Num(); i++)
    {
        BaseVertices[i] += Offset;       
    }
}

int32 UCosmicMeshComponent::GetQuadrantIndex(EClipmapQuadrant Q) const {
    switch (Q) {
    case EClipmapQuadrant::TopLeft:     return 0;
    case EClipmapQuadrant::TopRight:    return 3;
    case EClipmapQuadrant::BottomRight: return 2;
    case EClipmapQuadrant::BottomLeft:  return 1;
    default: return 0;
    }
}

void UCosmicMeshComponent::SetHoleQuadrant(EClipmapQuadrant NewQuadrant)
{
    if (!bMeshCreated || !bIsRing || CurrentQuadrant == NewQuadrant) return;

    int32 OldIdx = GetQuadrantIndex(CurrentQuadrant);
    int32 NewIdx = GetQuadrantIndex(NewQuadrant);
    int32 Steps = (NewIdx - OldIdx + 4) % 4;

    // 1. Averiguamos qué rotación necesitamos (relativa a la posición actual)
    // Para simplificar, asumimos que siempre partimos del estado base o calculamos el giro necesario.
    // Vamos a mapear los cuadrantes a grados de rotación (0, 90, 180, 270).
    int32 RotationAngle = 0;

    // Suponiendo que TopLeft es 0º, TopRight 90º, BottomRight 180º, BottomLeft 270º
    // (Ajusta los grados según cómo estén ordenados tus cuadrantes)
    if (Steps == 1) RotationAngle = 90;
    else if (Steps == 2) RotationAngle = 180;
    else if (Steps == 3) RotationAngle = 270;

    CurrentQuadrant = NewQuadrant;

    const int32 N = Resolution - 1;

    // 2. Copias temporales para no machacar datos al reordenar
    TArray<FVector> OldV = BaseVertices;
    TArray<FVector> OldN = BaseNormals;
    TArray<FProcMeshTangent> OldT = BaseTangents;

    // 3. Rotamos la "Matriz"
    for (int32 y = 0; y < N; ++y)
    {
        for (int32 x = 0; x < N; ++x)
        {
            int32 SourceIndex = y * N + x;
            int32 TargetX = x;
            int32 TargetY = y;

            // Fórmulas estándar de rotación de matrices cuadradas
            if (RotationAngle == 90)
            {
                TargetX = N - 1 - y;
                TargetY = x;
            }
            else if (RotationAngle == 180)
            {
                TargetX = N - 1 - x;
                TargetY = N - 1 - y;
            }
            else if (RotationAngle == 270)
            {
                TargetX = y;
                TargetY = N - 1 - x;
            }

            int32 TargetIndex = TargetY * N + TargetX;

            // 4. Reasignamos las posiciones en el array
            // OJO AL DATO ABAJO CON LOS VECTORES
            BaseVertices[TargetIndex] = OldV[SourceIndex];
            BaseNormals[TargetIndex] = OldN[SourceIndex];
            BaseTangents[TargetIndex] = OldT[SourceIndex];
        }
    }
}

void UCosmicMeshComponent::ReScaleLevel(int64 NewGridSpacing)
{
    if (!bMeshCreated)
    {
        UE_LOG(LogTemp, Error, TEXT("ReScaleLevel() llamado pero bMeshCreated = false"));
        return;
    }

    GridSpacing = NewGridSpacing;

    const int32 HalfRes = Resolution / 2;

    if (bIsPlanet) {

        const int32 VertRes = Resolution + 1;

        for (int32 y = 0; y < VertRes; ++y)
        {
            for (int32 x = 0; x < VertRes; ++x)
            {
                double WorldX = (x - HalfRes) * GridSpacing;
                double WorldY = (y - HalfRes) * GridSpacing;

                // Calcular posición en esfera
                FVector SphereCenter = FVector(0, 0, -PlanetRadius);
                double Distance2D = FMath::Sqrt(WorldX * WorldX + WorldY * WorldY);
                FVector Position;

                if (Distance2D <= PlanetRadius && Distance2D > 0.001f) // Evitar división por 0
                {
                    double ZOffset = FMath::Sqrt(PlanetRadius * PlanetRadius - Distance2D * Distance2D);
                    Position = FVector(WorldX, WorldY, -PlanetRadius + ZOffset);
                }
                else if (Distance2D <= 0.001f)
                {
                    // Centro - evitar NaN
                    Position = FVector(0, 0, 0);
                }
                else
                {
                    double Scale = PlanetRadius / Distance2D;
                    Position = FVector(WorldX * Scale, WorldY * Scale, -PlanetRadius);
                }

                BaseVertices[x + y * VertRes] = Position;
            }
        }
    }
    else {

        const int32 VertRes = Resolution - 1;

        for (int32 y = 0; y < VertRes; ++y)
        {
            for (int32 x = 0; x < VertRes; ++x)
            {
                double LocalX = (x - HalfRes) * GridSpacing;
                double LocalY = (y - HalfRes) * GridSpacing;

                FVector Pos = FVector(LocalX, LocalY, 0);

                BaseVertices[x + y * VertRes] = Pos;
            }
        }
    }

    CurrentQuadrant = EClipmapQuadrant::BottomRight;
}

void UCosmicMeshComponent::SetPositionAndRotation(const FVector& SurfacePos, const FRotator& PatchRotation)
{
    // Centro del planeta 
    FVector PlanetCenter = GetOwner()->GetActorLocation();

    PatchTransform = FTransform(
        PatchRotation,
        SurfacePos, 
        FVector(1, 1, 1)
    );
}

FVector UCosmicMeshComponent::ProjectToPlanet(const FVector& WorldPos, const FVector& PlanetCenter) const
{
    FVector Dir = (WorldPos - PlanetCenter).GetSafeNormal();
    return PlanetCenter + Dir * PlanetRadius;
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

    FCosmicNoiseGenerationParameters Params;
    if (NoiseSettings)
    {
        Params = NoiseSettings->Params;
    }

    NoiseTask = new FAsyncTask<FCosmicArchitectNoiseGenerator>(
        BaseVertices,
        PatchTransform,
        PlanetCenter,
        PlanetRadius,
        GridSpacing,
        bIsPlanet,
        Params
    );
    // Lanzar la tarea asincrona
    NoiseTask->StartBackgroundTask();
}



bool UCosmicMeshComponent::CheckAndApplyMeshUpdate()
{
    //Si no hay tarea devolvemos true para saber que esta libre
    if (!NoiseTask) return true;
    // Si no ha terminado, devolvemos false
    if (!NoiseTask->IsDone()) return false;
  
    TArray<FVector> CurrentVertices = NoiseTask->GetTask().CalculatedVertices;
    TArray<FLinearColor> CurrentColors = NoiseTask->GetTask().CalculatedColors;

    if (bIsPlanet) {
        BaseNormals = NoiseTask->GetTask().CalculatedNormals;
    }
    
    // Limpiamos la memoria de la tarea
    delete NoiseTask;
    NoiseTask = nullptr;
    bIsGeneratingNoise = false;

    // Actualizamos la sección de la malla (Esto ocurre instantáneamente en el Game Thread)
    UpdateMeshSection_LinearColor(
        0,
        CurrentVertices,
        BaseNormals,
        UVs,
        CurrentColors,
        BaseTangents
    );

    SetCollisionEnabled(ECollisionEnabled::NoCollision);

    return true; // La malla se ha actualizado
}

bool UCosmicMeshComponent::IsTaskActive()
{
    return NoiseTask && !NoiseTask->IsDone();
}
