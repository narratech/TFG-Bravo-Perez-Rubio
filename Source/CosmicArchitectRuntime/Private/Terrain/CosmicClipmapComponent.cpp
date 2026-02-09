// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain/CosmicClipmapComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Terrain/ClipmapMeshComponent.h"

// Sets default values for this component's properties
UCosmicClipmapComponent::UCosmicClipmapComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
    bTickInEditor = true;

	PrimaryComponentTick.bCanEverTick = true;
    
	// ...
}


// Called when the game starts
void UCosmicClipmapComponent::BeginPlay()
{
    Super::BeginPlay();
}


// Called every frame
void UCosmicClipmapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ElapsedTime += DeltaTime;

    if (ElapsedTime > TimeToRefresh) {

        ElapsedTime = 0;

        if (!bInit || Levels.Num() == 0)
            return;

        float DistanceToSurface = UpdatePatchTransform();

           // UE_LOG(LogTemp, Warning, TEXT("Distancia de cambio: %.4f, Distancia a la superficie:  %.4f"),
             //   IntermediateLevel.Mesh->GridSpacing * BaseResolution * HeightVisibility, DistanceToSurface);

        //Activar/desactivar modo rendimiento al alejarte lo suficiente

        bPerformaceMode = DistanceToSurface > PlanetRadius * HeightVisibility;

        if (FarLevel.Mesh->bActiveMesh && !bPerformaceMode || !FarLevel.Mesh->bActiveMesh && bPerformaceMode)
        {
            FarLevel.Mesh->SetMeshActive(bPerformaceMode);
            for (size_t i = 0; i < Levels.Num(); i++)
            {
                Levels[i].Mesh->SetMeshActive(!bPerformaceMode);
            }
        }

        int LevelsToUpdate = bPerformaceMode ? 0 : Levels.Num();

        if (bPerformaceMode)
        {
            FarLevel.Mesh->UpdateMesh();
        }

        for (size_t i = 0; i < LevelsToUpdate; i++)
        {
            //UE_LOG(LogTemp, Warning, TEXT("Actualizando: %d"), NumLevels);
            Levels[i].Mesh->UpdateMesh();
        }

        
    }    
}

void UCosmicClipmapComponent::CreateLevels()
{
    if (bInit)
    {
        ClearLevels();
    }

    // 2. Validar parámetros
    if (NumLevels <= 0 || BaseResolution <= 0 || PlanetRadius <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Parámetros inválidos para CreateLevels"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("UCosmicClipmapComponent::CreateLevels() - Creando %d niveles"), NumLevels);

    // 3. Inicializar array
    Levels.Empty();
    Levels.SetNum(NumLevels);

    // 4. Crear cada nivel
    for (int32 L = 0; L < NumLevels; ++L)
    {
        // Crear nombre único para el componente
        FName ComponentName = *FString::Printf(TEXT("ClipmapMesh_Level_%d"), L);

        // Crear componente
        UClipmapMeshComponent* Mesh = NewObject<UClipmapMeshComponent>(
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

        // Construir malla
        Mesh->BuildBaseMesh();

        // Asignar material
        if (BaseMaterial)
        {
            Mesh->SetMaterial(0, BaseMaterial);
        }
        else
        {
            // Material por defecto
            Mesh->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
        }

        // Guardar referencia
        Levels[L].Mesh = Mesh;

        UE_LOG(LogTemp, Warning, TEXT("  Nivel %d creado: GridSpacing=%.2f, bIsRing=%s"),
            L, Mesh->GridSpacing, Mesh->bIsRing ? TEXT("true") : TEXT("false"));
    }

    FName ComponentName = *FString::Printf(TEXT("ClipmapMesh_Performance_%d"), 0);

    UClipmapMeshComponent* Mesh = NewObject<UClipmapMeshComponent>(
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
        Mesh->bActiveMesh = false;

        Mesh->BuildBaseMesh();
        Mesh->SetMeshActive(false);

        FarLevel.Mesh = Mesh;

        UE_LOG(LogTemp, Warning, TEXT("  Nivel Extra creado" ));
    }
        
   

    bInit = true;
    UE_LOG(LogTemp, Warning, TEXT("CreateLevels completado. Niveles totales: %d"), Levels.Num());
}


void UCosmicClipmapComponent::ClearLevels()
{
    bInit = false;
    bPerformaceMode = false;

    UE_LOG(LogTemp, Warning, TEXT("UCosmicClipmapComponent::ClearLevels() - Limpiando %d niveles"), Levels.Num());

    // 1. Destruir componentes del array Levels
    for (FCosmicClipmapLevel& Level : Levels)
    {
        if (UClipmapMeshComponent* Mesh = Level.Mesh)
        {
            UE_LOG(LogTemp, Warning, TEXT("  Destruyendo nivel %d"), Mesh->LevelIndex);

            // Desactivar y limpiar la malla
            Mesh->SetMeshActive(false);
            Mesh->ClearAllMeshSections();

            // Destruir el componente
            Mesh->DestroyComponent();
            Mesh = nullptr;
        }
        Level.Mesh = nullptr;
    }

    // 2. Destruir nivel intermedio
    if (FarLevel.Mesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("  Destruyendo nivel intermedio"));

        FarLevel.Mesh->SetMeshActive(false);
        FarLevel.Mesh->ClearAllMeshSections();
        FarLevel.Mesh->DestroyComponent();
        FarLevel.Mesh = nullptr;
    }

    // 3. Limpiar arrays
    Levels.Empty();
    FarLevel = FCosmicClipmapLevel();

    UE_LOG(LogTemp, Warning, TEXT("ClearLevels() completado"));

}

float UCosmicClipmapComponent::UpdatePatchTransform()
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    if (Levels.Num() == 0) return 0.f;

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return 0.f;

    FVector ViewerPosWorld = PC->PlayerCameraManager->GetCameraLocation();

    FVector PlanetCenter = Owner->GetActorLocation();

    // Normal esférica
    FVector N = (ViewerPosWorld - PlanetCenter).GetSafeNormal();

    // Punto sobre la superficie
    FVector SurfacePos = PlanetCenter + N * PlanetRadius * HeightScale;

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


    for (FCosmicClipmapLevel& Level : Levels)
    {
        UClipmapMeshComponent* Mesh = Level.Mesh;
        if (!Mesh) continue;

        // Mover la malla procedimental
        Mesh->SetWorldLocationAndRotation(SurfacePos, PatchRotation);
    }

    if (FarLevel.Mesh) {
        FarLevel.Mesh->SetWorldLocationAndRotation(SurfacePos, PatchRotation);
    }

    return FVector::Distance(ViewerPosWorld, SurfacePos);
}



void UCosmicClipmapComponent::UpdateOrigins()
{

}


