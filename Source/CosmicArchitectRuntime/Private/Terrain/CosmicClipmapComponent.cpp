// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain/CosmicClipmapComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "CosmicNoiseSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Terrain/CosmicMeshComponent.h"
#include "Terrain/CosmicCollisionComponent.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture.h"
#include "CosmicCameraBridge.h"


// Sets default values for this component's properties
UCosmicClipmapComponent::UCosmicClipmapComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
    bTickInEditor = true;

	PrimaryComponentTick.bCanEverTick = true;

    
    
	// ...
}


void UCosmicClipmapComponent::UpdateCollisionNearPlayer(const FVector& SurfacePos, const FVector& SurfaceNormal, const float DistanceToSurface)
{
    if (!CollisionComponent || !NoiseSettings) return;

    // Solo generar colisión si el jugador está cerca de la superficie
    if (DistanceToSurface < CollisionComponent->MaxCollisionDistance)
    {
        CollisionComponent->UpdateCollisionMesh(NoiseSettings);
    }
    else
    {
        // Limpiar colisión si está lejos
        //CollisionComponent->ClearCollisionMesh();
    }
}

// Called when the game starts
void UCosmicClipmapComponent::BeginPlay()
{
    Super::BeginPlay();

    TimeToRefreshActive = TimeToRefresh;
}


// Called every frame
void UCosmicClipmapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ElapsedTime += DeltaTime;

    if (DynamicPlanetMat) {
        DynamicPlanetMat->SetVectorParameterValue("CentroPlaneta", GetOwner()->GetActorLocation());
    }

    if (ElapsedTime > TimeToRefreshActive) {

        ElapsedTime = 0;

        FVector SurfacePos = FVector();
        FVector N = FVector();
        FVector ViewerPos = FVector();

        //float DistanceToSurface = GetDistanceToSurface(ViewerPos, SurfacePos, N);
        float DistanceToSurface = GetDistanceToPlainSurface(ViewerPos, SurfacePos, N);

        //UpdateCollisionNearPlayer(SurfacePos, N, DistanceToSurface);

        if (!FarLevel)
            return;

        if (!bPerformanceBuild) {
            FarLevel->RequestMeshUpdate();
            bPerformanceBuild = FarLevel->CheckAndApplyMeshUpdate();
        }

        //Activar/desactivar modo rendimiento al alejarte lo suficiente
        bPerformaceMode = DistanceToSurface > PlanetRadius * HeightVisibility;

        if (!bInit && !bPerformaceMode) {
            CreateLevels();
        }

        if (FarLevel->bActiveMesh && !bPerformaceMode || !FarLevel->bActiveMesh && bPerformaceMode)
        {
            FarLevel->SetMeshActive(bPerformaceMode);
            for (size_t i = 0; i < Levels.Num(); i++)
            {
                Levels[i]->SetMeshActive(!bPerformaceMode);
            }
        }

        if (bPerformaceMode || FreezeGeneration) return;

        bool MeshesUpdated = true;

        for (size_t i = 0; i < Levels.Num(); i++)
        {
            if (!Levels[i]->CheckAndApplyMeshUpdate()) {
                MeshesUpdated = false;
            }
        }

        if (!MeshesUpdated) return;
     
        //if (Levels.Num() > 1)
        //{
        //    UCosmicMeshComponent* MeshLast = Levels.Last();
        //    UCosmicMeshComponent* MeshFirst = Levels[0];

        //    bool bIsVisible = IsClipmapRingVisible(Levels.Num() - 1, DistanceToSurface);

        //    //UE_LOG(LogTemp, Warning, TEXT("Ultimo visible %d"), bIsVisible);

        //    if (!bIsVisible && MeshFirst->GridSpacing > MinTriangleSize) {
        //        ReduceClimapLevel();
        //    }
        //    else if(IsClipmapRingVisible(MeshLast->GridSpacing * 2, MeshLast->Resolution, DistanceToSurface) 
        //        && MeshLast->GridSpacing < BaseGridSpacing * FMath::Pow(2.0f, NumLevels - 1)){
        //        IncreaseClipmapLevel();
        //    }         
        //}

        FIntPoint Shift = ComputeGridShift(ViewerPos, BaseGridSpacing * 2);

        if (Shift != FIntPoint::ZeroValue)
        {
            // actualizar niveles necesarios
            UpdateLevels(Shift);
        }

    }    
}

void UCosmicClipmapComponent::CreateLevels()
{
    if (bInit)
    {
        ClearLevels();
    }

    if (CollisionComponent && ParentRoot)
    {
        CollisionComponent->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);
        CollisionComponent->GenerateCollisionMesh(PlanetRadius);
    }

    
    //UE_LOG(LogTemp, Warning, TEXT("No entra"));

    // 2. Validar parámetros
    if (NumLevels <= 0 || BaseResolution <= 0 || PlanetRadius <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Parámetros inválidos para CreateLevels"));
        return;
    }

    int32 Remainder = BaseResolution % 4;

    if (Remainder != 0) {
        BaseResolution += 4 - Remainder;
    }

    //UE_LOG(LogTemp, Warning, TEXT("UCosmicClipmapComponent::CreateLevels() - Creando %d niveles"), NumLevels);

    // 3. Inicializar array
    Levels.Empty();
    Levels.SetNum(NumLevels);

    BaseGridSpacing = (PlanetRadius * 2.0f) / (BaseResolution * FMath::Pow(2.0f, NumLevels - 1));
    //UE_LOG(LogTemp, Error, TEXT("BaseGridSpacing %.4f"), BaseGridSpacing);

    // 4. Crear cada nivel
    for (int32 L = 0; L < NumLevels; ++L)
    {
        // Crear nombre único para el componente
        FName ComponentName = *FString::Printf(TEXT("ClipmapMesh_Level_%d"), L);

        // Crear componente
        UCosmicMeshComponent* Mesh = NewObject<UCosmicMeshComponent>(
            GetOwner(),
            ComponentName
        );

        if (!Mesh)
        {
            UE_LOG(LogTemp, Error, TEXT("No se pudo crear ClipmapMeshComponent para nivel %d"), L);
            continue;
        }

        // Registrar componente
        Mesh->RegisterComponent();

        // Adjuntar al root
        if (ParentRoot)
        {
            Mesh->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);
        }

        // Configurar propiedades
        Mesh->LevelIndex = L;
        Mesh->Resolution = BaseResolution;
        Mesh->GridSpacing = BaseGridSpacing * FMath::Pow(2.0f, L); // (1 << L) para ints
        Mesh->bIsRing = (L > 0);
        Mesh->PlanetRadius = PlanetRadius;
        Mesh->bActiveMesh = true;
        Mesh->NoiseSettings = NoiseSettings;

        // Construir malla
        Mesh->BuildBaseMesh();

        // Asignar material
        if (DynamicPlanetMat)
        {
            Mesh->SetMaterial(0, DynamicPlanetMat);
            
            
        }

        //else
        //{
        //    // Material por defecto
        //    Mesh->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
        //}

        // Guardar referencia
        Levels[L] = Mesh;

        //UE_LOG(LogTemp, Warning, TEXT("  Nivel %d creado: GridSpacing=%.2f, bIsRing=%s"),
        //    L, Mesh->GridSpacing, Mesh->bIsRing ? TEXT("true") : TEXT("false"));
    }

    /*FVector SurfacePos = FVector();
    FVector N = FVector();

    float DistanceToSurface = GetDistanceToSurface(SurfacePos, N);
    UpdatePatchTransform(SurfacePos, N);*/

        
    bInit = true;
    //UE_LOG(LogTemp, Warning, TEXT("CreateLevels completado. Niveles totales: %d"), Levels.Num());
}

void UCosmicClipmapComponent::CreatePerformanceLevel(bool bActive)
{
    if (FarLevel) return;

    if (BaseMaterial) {

        DynamicPlanetMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);

        if (DynamicPlanetMat)
        {
            DynamicPlanetMat->SetScalarParameterValue(FName("PlanetRadius"), PlanetRadius);

            if (NoiseSettings) {
                DynamicPlanetMat->SetScalarParameterValue(FName("MaxHeight"), NoiseSettings->Params.MaxMountainHeight);
            }

            DynamicPlanetMat->SetVectorParameterValue(FName("BaseColor"), PlanetMainColor1);
            DynamicPlanetMat->SetVectorParameterValue(FName("MidColor"), PlanetMainColor2);
            DynamicPlanetMat->SetVectorParameterValue(FName("ColdColor"), PlanetAltitudeColor);
            DynamicPlanetMat->SetScalarParameterValue(FName("NoiseScale"), MaterialNoiseScale);
            DynamicPlanetMat->SetTextureParameterValue(FName("PlanetTexture"), DefaultTexture);
             
        }
    }

    FName ComponentName = *FString::Printf(TEXT("ClipmapMesh_Performance_%d"), 0);

    UCosmicMeshComponent* Mesh = NewObject<UCosmicMeshComponent>(
        GetOwner(),
        ComponentName
    );

    if (Mesh)
    {
        Mesh->RegisterComponent();

        if (ParentRoot)
        {
            Mesh->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);
        }

        Mesh->LevelIndex = NumLevels - 1;
        Mesh->Resolution = BaseResolution;
        Mesh->GridSpacing = BaseGridSpacing * FMath::Pow(2.0f, NumLevels - 1);
        Mesh->bIsRing = false;
        Mesh->PlanetRadius = PlanetRadius;
        Mesh->NoiseSettings = NoiseSettings;

        Mesh->BuildSphereMesh();
        Mesh->SetMeshActive(bActive);

        FarLevel = Mesh;

       

        if (DynamicPlanetMat)
        {
            Mesh->SetMaterial(0, DynamicPlanetMat);
        }
        //else
        //{
        //    // Material por defecto
        //    Mesh->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
        //}

        //UE_LOG(LogTemp, Warning, TEXT("  Nivel Extra creado"));
    }

    bInit = false;
}


void UCosmicClipmapComponent::ClearLevels()
{
    bInit = false;

    int LevelsCleared = 0;  

    // 1. Destruir componentes del array Levels
    for (UCosmicMeshComponent* Mesh : Levels)
    {
        UE_LOG(LogTemp, Warning, TEXT("  Destruyendo nivel %d"), Mesh->LevelIndex);

        // Desactivar y limpiar la malla
        Mesh->SetMeshActive(false);
        Mesh->ClearAllMeshSections();

        // Destruir el componente
        Mesh->DestroyComponent();
        Mesh = nullptr;
        //LevelsCleared++;
    }

    // 2. Destruir nivel exterior
    if (FarLevel)
    {
        UE_LOG(LogTemp, Warning, TEXT("  Destruyendo nivel exterior"));

        FarLevel->SetMeshActive(false);
        FarLevel->ClearAllMeshSections();
        FarLevel->DestroyComponent();
        FarLevel = nullptr;
    }

    // 3. Limpiar arrays
    Levels.Empty();

    if (ParentRoot)
    {
        
        // Obtenemos una copia de los hijos para evitar problemas al modificar el array mientras iteramos
        TArray<USceneComponent*> Children = ParentRoot->GetAttachChildren();

        for (USceneComponent* Child : Children)
        {
            if (UCosmicMeshComponent* TargetMesh = Cast<UCosmicMeshComponent>(Child))
            {
                // 1. Lo desadjuntamos del padre
                TargetMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

                // 2. Lo marcamos para destrucción definitiva
                TargetMesh->DestroyComponent();

                LevelsCleared++;
            }
        }

    }

    if (DynamicPlanetMat)
    {
        DynamicPlanetMat = nullptr;
    }

    bPerformanceBuild = false;

    //UE_LOG(LogTemp, Warning, TEXT("UCosmicClipmapComponent::ClearLevels() - Limpiando %d niveles"), LevelsCleared);
}

void UCosmicClipmapComponent::ReasignLevels()
{
    int LevelsReasigned = 0;

    //double UpdateMeshStartTime = FPlatformTime::Seconds();

    if (ParentRoot)
    {
        // Obtenemos una copia de los hijos para evitar problemas al modificar el array mientras iteramos
        TArray<USceneComponent*> Children = ParentRoot->GetAttachChildren();

        for (USceneComponent* Child : Children)
        {
            if (UCosmicMeshComponent* TargetMesh = Cast<UCosmicMeshComponent>(Child))
            {
                if (!FarLevel)
                {
                    FarLevel = TargetMesh;  // Primer mesh = FarLevel
                }
                else
                {
                    Levels.Add(TargetMesh); // Resto = Levels
                }
            }
        }
    }

    /*double UpdateMeshEndTime = FPlatformTime::Seconds();
    double UpdateMeshTime = UpdateMeshEndTime - UpdateMeshStartTime;

    UE_LOG(LogTemp, Warning, TEXT("Mallas reasignada en %.4f ms"), UpdateMeshTime * 1000.0);*/

    //UE_LOG(LogTemp, Warning, TEXT("UCosmicClipmapComponent::ReasignLevels() - Reasignando %d niveles"), LevelsReasigned);
}

void UCosmicClipmapComponent::SetMaterialData(FColor color1, FColor color2, FColor colorHeight, float scale)
{
    PlanetMainColor1 = color1;
    PlanetMainColor2 = color2;
    PlanetAltitudeColor = colorHeight;
    MaterialNoiseScale = scale;
}


void UCosmicClipmapComponent::UpdatePatchTransform(const FVector& SurfacePos, const FVector& N)
{
    const FVector Up = N;

    // Elegimos un vector no colineal (branch barato)
    const FVector Tangent = (FMath::Abs(Up.Z) < 0.99f)
        ? FVector(0, 0, 1)
        : FVector(1, 0, 0);

    // Right sale normalizado si Up y Tangent son unitarios
    FVector Right = FVector::CrossProduct(Tangent, Up);
    Right.Normalize(); // solo UNA normalización

    const FVector Forward = FVector::CrossProduct(Up, Right); // ya unitario

    const FRotator PatchRotation =
        FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();

    /*FVector Up = N;
    FVector Arbitrary = (FMath::Abs(Up.Z) < 0.99f) ? FVector::UpVector : FVector::ForwardVector;
    FVector Right = FVector::CrossProduct(Arbitrary, Up).GetSafeNormal();
    FVector Forward = FVector::CrossProduct(Up, Right).GetSafeNormal();
    FRotator PatchRotation = FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();*/


    for (UCosmicMeshComponent* Mesh : Levels)
    {
        if (!Mesh) continue;

        // Mover la malla procedimental
        Mesh->SetWorldLocationAndRotation(SurfacePos, PatchRotation);
    }

    if (CollisionComponent) {
        CollisionComponent->SetWorldLocationAndRotation(SurfacePos, PatchRotation);
    }
   
}

FIntPoint UCosmicClipmapComponent::ComputeGridShift(
    const FVector& PlayerPos,
    float GridSpacing)
{
    FVector FrameDelta = PlayerPos - LastPlayerPos;

    LastPlayerPos = PlayerPos;

    AccumulatedDelta += FrameDelta;

    int32 ShiftX = FMath::FloorToInt(AccumulatedDelta.X / GridSpacing);
    int32 ShiftY = FMath::FloorToInt(AccumulatedDelta.Y / GridSpacing);

    // Quitamos lo que ya hemos consumido
    AccumulatedDelta.X -= ShiftX * GridSpacing;
    AccumulatedDelta.Y -= ShiftY * GridSpacing;

    return FIntPoint(ShiftX, ShiftY);
}

void UCosmicClipmapComponent::UpdateLevels(const FIntPoint& Shift)
{
    int32 JumpsX = Shift.X;
    int32 JumpsY = Shift.Y;

    for (int32 i = 0; i < Levels.Num(); ++i)
    {
        UCosmicMeshComponent* Level = Levels[i];

        if (i == 0)
        {
            // El nivel base siempre se mueve
            Level->ShiftLevel(Shift);
            Level->RequestMeshUpdate();
            continue;
        }

        EClipmapQuadrant currentQuadrant = Level->CurrentQuadrant;

        // Descomponemos el cuadrante: ahora usamos bIsBottom porque +Y es Bottom
        bool bIsRight = (currentQuadrant == EClipmapQuadrant::TopRight || currentQuadrant == EClipmapQuadrant::BottomRight);
        bool bIsBottom = (currentQuadrant == EClipmapQuadrant::BottomLeft || currentQuadrant == EClipmapQuadrant::BottomRight);

        // LOGICA DEL EJE X (+X es Derecha)
        if (JumpsX != 0)
        {
            if (FMath::Abs(JumpsX) % 2 == 1)
            {
                if (bIsRight) {
                    if (JumpsX > 0) JumpsX = (JumpsX / 2) + 1; // Empuja el borde derecho
                    else JumpsX = (JumpsX / 2);                // Absorbe hacia el centro
                    bIsRight = false; // El hueco cambia a la izquierda
                }
                else {
                    if (JumpsX < 0) JumpsX = (JumpsX / 2) - 1; // Empuja el borde izquierdo
                    else JumpsX = (JumpsX / 2);                // Absorbe hacia el centro
                    bIsRight = true;  // El hueco cambia a la derecha
                }
            }
            else
            {
                JumpsX = JumpsX / 2;
            }
        }

        // LOGICA DEL EJE Y (+Y es Abajo / Bottom)
        if (JumpsY != 0)
        {
            if (FMath::Abs(JumpsY) % 2 == 1)
            {
                if (bIsBottom) {
                    if (JumpsY > 0) JumpsY = (JumpsY / 2) + 1; // Empuja el borde inferior (+Y)
                    else JumpsY = (JumpsY / 2);                // Absorbe el salto -Y hacia el centro
                    bIsBottom = false; // El hueco cambia a Arriba (Top)
                }
                else {
                    if (JumpsY < 0) JumpsY = (JumpsY / 2) - 1; // Empuja el borde superior (-Y)
                    else JumpsY = (JumpsY / 2);                // Absorbe el salto +Y hacia el centro
                    bIsBottom = true;  // El hueco cambia a Abajo (Bottom)
                }
            }
            else
            {
                JumpsY = JumpsY / 2;
            }
        }

        // Reconstruimos el cuadrante a partir de los booleanos
        EClipmapQuadrant NewQuadrant;
        if (!bIsBottom && !bIsRight) NewQuadrant = EClipmapQuadrant::TopLeft;
        else if (!bIsBottom && bIsRight) NewQuadrant = EClipmapQuadrant::TopRight;
        else if (bIsBottom && !bIsRight) NewQuadrant = EClipmapQuadrant::BottomLeft;
        else NewQuadrant = EClipmapQuadrant::BottomRight;

        // Solo actualizamos el cuadrante si realmente ha cambiado
        if (NewQuadrant != currentQuadrant) {
            Level->SetHoleQuadrant(NewQuadrant);
        }

        // Aplicamos el movimiento fisico a este nivel
        FIntPoint Movement(JumpsX, JumpsY);
        if (Movement != FIntPoint::ZeroValue) {
            Level->ShiftLevel(Movement);
        }

        Level->RequestMeshUpdate();

        // Salida temprana, si ya no hay movimiento que propagar, cortamos el bucle para ahorrar CPU
        if (JumpsX == 0 && JumpsY == 0) return;
    }
}

FVector UCosmicClipmapComponent::GetPlayerLocation()
{
    FVector PlayerLocation = FVector::ZeroVector;

    if (GetWorld()->IsGameWorld())
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC && PC->PlayerCameraManager)
        {
            PlayerLocation = PC->PlayerCameraManager->GetCameraLocation();
        }
    }

#if WITH_EDITOR
    // En editor, si no tenemos cámara de juego, usar la cámara del editor
    if (PlayerLocation.IsZero())
    {
        PlayerLocation = FCosmicCameraBridge::CameraLocation;
    }
#endif

    return PlayerLocation;
}

float UCosmicClipmapComponent::GetDistanceToSurface(FVector& OutViewerPos, FVector& OutSurfacePos, FVector& OutN)
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    OutViewerPos = GetPlayerLocation();

    FVector PlanetCenter = Owner->GetActorLocation();

    FVector CenterToViewer = OutViewerPos - PlanetCenter;
    float DistanceToCenter = CenterToViewer.Length();

    // Normal esferica
    OutN = (OutViewerPos - PlanetCenter).GetSafeNormal();

    // Punto sobre la superficie
    OutSurfacePos = PlanetCenter + OutN * PlanetRadius;

    if (DistanceToCenter <= PlanetRadius)
    {
        return 0.f;
    }

    return FVector::Distance(OutViewerPos, OutSurfacePos);
}

float UCosmicClipmapComponent::GetDistanceToPlainSurface(FVector& OutViewerPos, FVector& OutSurfacePos, FVector& OutN)
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    // Posición del viewer
    OutViewerPos = GetPlayerLocation();

    // Punto base del plano (puede ser el actor)
    FVector PlanePoint = Owner->GetActorLocation();

    // Normal del plano
    OutN = FVector::UpVector;

    // Vector del plano al viewer
    FVector PlaneToViewer = OutViewerPos - PlanePoint;

    // Distancia firmada al plano
    float Distance = FVector::DotProduct(PlaneToViewer, OutN);

    // Proyeccion del viewer sobre el plano 
    OutSurfacePos = OutViewerPos - Distance * OutN;

    return Distance;
}



bool UCosmicClipmapComponent::IsClipmapRingVisible(const int32 LevelIndex, const float DistanceToSurface)
{  
    
    // Calcular el radio del clipmap en la superficie
    float ClipmapSurfaceRadius = Levels[LevelIndex]->GridSpacing * (Levels[LevelIndex]->Resolution - 2) * 0.5f;

    // Radio maximo visible desde esta altura (proyeccion en la superficie)
    float VisibleRadius = PlanetRadius * FMath::Sin(FMath::Acos(PlanetRadius / (PlanetRadius + DistanceToSurface)));

    /*UE_LOG(LogTemp, Warning, TEXT("ClipmapSurface: %.4f"), ClipmapSurfaceRadius);
    UE_LOG(LogTemp, Warning, TEXT("VisibleRadius: %.4f"), VisibleRadius * 2);*/

    // El clipmap es visible si su radio es menor que el radio visible
    return ClipmapSurfaceRadius <= VisibleRadius * 2.f; 
}

bool UCosmicClipmapComponent::IsClipmapRingVisible(const float GridSpacing, const int32 Resolution, const float DistanceToSurface)
{
    float ClipmapSurfaceRadius = GridSpacing * (Resolution - 2) * 0.5f;

    // Radio maximo visible desde esta altura (proyeccion en la superficie)
    float VisibleRadius = PlanetRadius * FMath::Sin(FMath::Acos(PlanetRadius / (PlanetRadius + DistanceToSurface)));

   /* UE_LOG(LogTemp, Warning, TEXT("ClipmapSurface: %.4f"), ClipmapSurfaceRadius);
    UE_LOG(LogTemp, Warning, TEXT("VisibleRadius: %.4f"), VisibleRadius * 2);*/
    // El clipmap es visible si su radio es menor que el radio visible
    return ClipmapSurfaceRadius <= VisibleRadius * 2.f;
}

void UCosmicClipmapComponent::ReduceClimapLevel()
{
    if (NumLevels > 1)
    {
        int64 Spacing = Levels[0]->GridSpacing;

        // Guardar ultimo
        UCosmicMeshComponent* Last = Levels[NumLevels - 1];

        // Shift manual correcto 
        for (int32 i = NumLevels - 1; i > 1; --i)
        {
            Levels[i] = Levels[i - 1];
        }

        Levels[1] = Last;

        // Reasignar indices coherentes
        for (int32 i = 0; i < NumLevels; ++i)
        {
            Levels[i]->LevelIndex = i;
        }

        // Solo regenerar los necesarios
        Levels[0]->ReScaleLevel(Spacing / 2);
        Levels[1]->ReScaleLevel(Spacing);
    }
}

void UCosmicClipmapComponent::IncreaseClipmapLevel()
{
    if (NumLevels > 1)
    {
        int64 Spacing = Levels[0]->GridSpacing;

        // Guardar el segundo nivel
        UCosmicMeshComponent* Second = Levels[1];

        // Shift hacia la izquierda desde índice 1
        for (int32 i = 1; i < NumLevels - 1; ++i)
        {
            Levels[i] = Levels[i + 1];
        }

        // Colocar el antiguo segundo al final
        Levels[NumLevels - 1] = Second;

        // Reasignar índices coherentes
        for (int32 i = 0; i < NumLevels; ++i)
        {
            Levels[i]->LevelIndex = i;
        }

        Levels[0]->ReScaleLevel(Spacing * 2);
        Levels[NumLevels - 1]->ReScaleLevel(Levels[NumLevels - 2]->GridSpacing * 2);
    }
}



