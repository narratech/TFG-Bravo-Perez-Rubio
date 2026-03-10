// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain/CosmicClipmapComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "CosmicNoiseSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Terrain/CosmicMeshComponent.h"
#include "Terrain/CosmicCollisionComponent.h"
#include "CosmicCameraBridge.h"


// Sets default values for this component's properties
UCosmicClipmapComponent::UCosmicClipmapComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
    bTickInEditor = true;

	PrimaryComponentTick.bCanEverTick = true;

    CollisionComponent = CreateDefaultSubobject<UCosmicCollisionComponent>(TEXT("CollisionComponent"));
    CollisionComponent->SetupAttachment(ParentRoot);
    
	// ...
}


void UCosmicClipmapComponent::UpdateCollisionNearPlayer(const FVector& SurfacePos, const FVector& SurfaceNormal, const float DistanceToSurface)
{
    if (!CollisionComponent) return;

    // Solo generar colisión si el jugador está cerca de la superficie
    if (DistanceToSurface < CollisionComponent->MaxCollisionDistance)
    {
        CollisionComponent->GenerateCollisionMesh(
            SurfacePos,
            SurfaceNormal,
            CollisionComponent->CollisionAreaSize,
            CollisionComponent->CollisionResolution
        );
    }
    else
    {
        // Limpiar colisión si está lejos
        CollisionComponent->ClearCollisionMesh();
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

    if (ElapsedTime > TimeToRefreshActive) {

        ElapsedTime = 0;

        FVector SurfacePos = FVector();
        FVector N = FVector();

        float DistanceToSurface = GetDistanceToSurface(SurfacePos, N);

        UpdateCollisionNearPlayer(SurfacePos, N, DistanceToSurface);

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

        if (bPerformaceMode) return;

        for (size_t i = 0; i < Levels.Num(); i++)
        {
            if (Levels[i]->bActiveMesh)
            {
                // Aplica los nuevos vértices a la gráfica si la tarea ya terminó
                Levels[i]->CheckAndApplyMeshUpdate();
            }
        }
        
        if (Levels.Num() > 1)
        {
            UCosmicMeshComponent* MeshLast = Levels.Last();
            UCosmicMeshComponent* MeshFirst = Levels[0];

            bool bIsVisible = IsClipmapRingVisible(Levels.Num() - 1, DistanceToSurface);

            if (!bIsVisible && MeshFirst->GridSpacing > MinTriangleSize) {
                ReduceClimapLevel();
            }
            else if(IsClipmapRingVisible(MeshLast->GridSpacing * 2, MeshLast->Resolution, DistanceToSurface) 
                && MeshLast->GridSpacing < BaseGridSpacing * FMath::Pow(2.0f, NumLevels - 1)){
                IncreaseClipmapLevel();
            }         
        }

        if (FreezeGeneration) {
            return;
        }

        UpdatePatchTransform(SurfacePos, N);

        for (size_t i = 0; i < Levels.Num(); i++)
        {
            if (Levels[i]->bActiveMesh)
            {
                // Inicia el cálculo de ruido en hilos de fondo si no está haciéndolo ya
                Levels[i]->RequestMeshUpdate();
            }
        }
    }    
}

void UCosmicClipmapComponent::CreateLevels()
{
    if (bInit)
    {
        ClearLevels();
    }

    DynamicPlanetMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);

    if (DynamicPlanetMat)
    {
        DynamicPlanetMat->SetScalarParameterValue(FName("PlanetRadius"), PlanetRadius*1000);
        DynamicPlanetMat->SetScalarParameterValue(FName("MaxHeight"), NoiseSettings->MaxMountainHeight);
    }

    if (CollisionComponent && ParentRoot && !CollisionComponent->IsAttachedTo(ParentRoot))
    {
        CollisionComponent->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);
    }

    // 2. Validar parámetros
    if (NumLevels <= 0 || BaseResolution <= 0 || PlanetRadius <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Parámetros inválidos para CreateLevels"));
        return;
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

    FVector SurfacePos = FVector();
    FVector N = FVector();

    float DistanceToSurface = GetDistanceToSurface(SurfacePos, N);
    UpdatePatchTransform(SurfacePos, N);

        
    bInit = true;
    //UE_LOG(LogTemp, Warning, TEXT("CreateLevels completado. Niveles totales: %d"), Levels.Num());
}

void UCosmicClipmapComponent::CreatePerformanceLevel(bool bActive)
{
    if (FarLevel) return;

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

       

        if (BaseMaterial)
        {
            Mesh->SetMaterial(0, BaseMaterial);
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

float UCosmicClipmapComponent::GetDistanceToSurface(FVector& SurfacePos, FVector& N)
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    FVector ViewerPosWorld = GetPlayerLocation();

    FVector PlanetCenter = Owner->GetActorLocation();

    FVector CenterToViewer = ViewerPosWorld - PlanetCenter;
    float DistanceToCenter = CenterToViewer.Length();

    // Normal esférica
    N = (ViewerPosWorld - PlanetCenter).GetSafeNormal();

    // Punto sobre la superficie
    SurfacePos = PlanetCenter + N * PlanetRadius;

    if (DistanceToCenter <= PlanetRadius)
    {
        return 0.f;
    }

    return FVector::Distance(ViewerPosWorld, SurfacePos);
}



bool UCosmicClipmapComponent::IsClipmapRingVisible(const int32 LevelIndex, const float DistanceToSurface)
{  
    // Calcular el radio del clipmap en la superficie
    float ClipmapSurfaceRadius = Levels[LevelIndex]->GridSpacing * Levels[LevelIndex]->Resolution * 0.5f;

    // Radio maximo visible desde esta altura (proyeccion en la superficie)
    float VisibleRadius = PlanetRadius * FMath::Sin(FMath::Acos(PlanetRadius / (PlanetRadius + DistanceToSurface)));

    // El clipmap es visible si su radio es menor que el radio visible
    return ClipmapSurfaceRadius <= VisibleRadius * 2.f; 
}

bool UCosmicClipmapComponent::IsClipmapRingVisible(const float GridSpacing, const int32 Resolution, const float DistanceToSurface)
{
    float ClipmapSurfaceRadius = GridSpacing * Resolution * 0.5f;

    // Radio maximo visible desde esta altura (proyeccion en la superficie)
    float VisibleRadius = PlanetRadius * FMath::Sin(FMath::Acos(PlanetRadius / (PlanetRadius + DistanceToSurface)));

    // El clipmap es visible si su radio es menor que el radio visible
    return ClipmapSurfaceRadius <= VisibleRadius * 2.f;
}

void UCosmicClipmapComponent::ReduceClimapLevel()
{
    if (NumLevels > 1)
    {
        float spacing = Levels[0]->GridSpacing;

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
        Levels[0]->RegenerateLevel(spacing / 2.f);
        Levels[1]->RegenerateLevel(spacing);
    }
}

void UCosmicClipmapComponent::IncreaseClipmapLevel()
{
    if (NumLevels > 1)
    {
        float spacing = Levels[0]->GridSpacing;

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

        Levels[0]->RegenerateLevel(spacing * 2.f);
        Levels[NumLevels - 1]->RegenerateLevel(Levels[NumLevels - 2]->GridSpacing * 2.f);
    }
}



