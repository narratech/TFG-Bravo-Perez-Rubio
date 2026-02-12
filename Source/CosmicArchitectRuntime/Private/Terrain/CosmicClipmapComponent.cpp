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

    TimeToRefreshActive = TimeToRefresh;
}


// Called every frame
void UCosmicClipmapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ElapsedTime += DeltaTime;

    if (ElapsedTime > TimeToRefreshActive) {

        ElapsedTime = 0;

        float DistanceToSurface = UpdatePatchTransform();

        if (!FarLevel)
            return;

        // UE_LOG(LogTemp, Warning, TEXT("Distancia de cambio: %.4f, Distancia a la superficie:  %.4f"),
          //   IntermediateLevel.Mesh->GridSpacing * BaseResolution * HeightVisibility, DistanceToSurface);

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

            //TimeToRefreshActive = bPerformaceMode ? 0.5f : TimeToRefresh;
        }

        //int LevelsToUpdate = bPerformaceMode ? 0 : Levels.Num();

        if (bPerformaceMode)
        {
            //FarLevel->UpdateMesh();
            return;
        }

        int LevelsUpdating = 0;

        for (size_t i = 4; i < Levels.Num(); i++)
        {
            UClipmapMeshComponent* Mesh = Levels[i];

            // Versión simple y rápida para clipmaps concéntricos
            bool bIsVisible = IsClipmapRingVisible(i, DistanceToSurface);

            if (bIsVisible)
            {
                if (!Mesh->bActiveMesh)
                    Mesh->SetMeshActive(true);
                LevelsUpdating++;
            }
            else if (Mesh->bActiveMesh)
            {
                // Desactivar si no es visible para ahorrar recursos
                Mesh->SetMeshActive(false);
            }
        }

        //UE_LOG(LogTemp, Warning, TEXT("Actualizando: %d"), LevelsUpdating + 4);

        for (size_t i = 0; i < Levels.Num(); i++)
        { 
            if (Levels[i]->bActiveMesh)
                Levels[i]->UpdateMesh();
        }   

        UpdatePatchTransform();
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

        Mesh->CreationMethod = EComponentCreationMethod::UserConstructionScript;
        //Mesh->bCreatedByConstructionScript = true;


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
        Levels[L] = Mesh;

        UE_LOG(LogTemp, Warning, TEXT("  Nivel %d creado: GridSpacing=%.2f, bIsRing=%s"),
            L, Mesh->GridSpacing, Mesh->bIsRing ? TEXT("true") : TEXT("false"));
    }

    // 5. Crear nivel performance
    //CreatePerformanceLevel();
        
    bInit = true;
    UE_LOG(LogTemp, Warning, TEXT("CreateLevels completado. Niveles totales: %d"), Levels.Num());
}

void UCosmicClipmapComponent::CreatePerformanceLevel(bool bActive)
{
    if (FarLevel) return;

    FName ComponentName = *FString::Printf(TEXT("ClipmapMesh_Performance_%d"), 0);

    UClipmapMeshComponent* Mesh = NewObject<UClipmapMeshComponent>(
        GetOwner(),
        ComponentName
    );

    if (Mesh)
    {
        Mesh->CreationMethod = EComponentCreationMethod::UserConstructionScript;

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

        Mesh->BuildSphereMesh();
        Mesh->SetMeshActive(bActive);

        FarLevel = Mesh;

        UE_LOG(LogTemp, Warning, TEXT("  Nivel Extra creado"));
    }
}


void UCosmicClipmapComponent::ClearLevels()
{
    bInit = false;

    int LevelsCleared = 0;  

    // 1. Destruir componentes del array Levels
    for (UClipmapMeshComponent* Mesh : Levels)
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
            if (UClipmapMeshComponent* TargetMesh = Cast<UClipmapMeshComponent>(Child))
            {
                // 1. Lo desadjuntamos del padre
                TargetMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

                // 2. Lo marcamos para destrucción definitiva
                TargetMesh->DestroyComponent();

                LevelsCleared++;
            }
        }

    }

    UE_LOG(LogTemp, Warning, TEXT("UCosmicClipmapComponent::ClearLevels() - Limpiando %d niveles"), LevelsCleared);
}

void UCosmicClipmapComponent::ReasignLevels()
{
    int LevelsReasigned = 0;

    if (ParentRoot)
    {
        // Obtenemos una copia de los hijos para evitar problemas al modificar el array mientras iteramos
        TArray<USceneComponent*> Children = ParentRoot->GetAttachChildren();

        for (USceneComponent* Child : Children)
        {
            if (UClipmapMeshComponent* TargetMesh = Cast<UClipmapMeshComponent>(Child))
            {
                LevelsReasigned++;
            }
        }

        Levels.SetNum(LevelsReasigned - 1);

        int i = 0;

        for (USceneComponent* Child : Children)
        {
            if (UClipmapMeshComponent* TargetMesh = Cast<UClipmapMeshComponent>(Child))
            {
                if (i == 0) {
                    FarLevel = TargetMesh;
                    i++;
                    continue;
                }

                Levels[i - 1] = TargetMesh;
                i++;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("UCosmicClipmapComponent::ReasignLevels() - Reasignando %d niveles"), LevelsReasigned);
}


float UCosmicClipmapComponent::UpdatePatchTransform()
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return 0.f;

    FVector ViewerPosWorld = PC->PlayerCameraManager->GetCameraLocation();

    FVector PlanetCenter = Owner->GetActorLocation();

    // Normal esférica
    FVector N = (ViewerPosWorld - PlanetCenter).GetSafeNormal();

    // Punto sobre la superficie
    FVector SurfacePos = PlanetCenter + N * PlanetRadius * HeightScale;

    if (bPerformaceMode || Levels.Num() == 0)
        return FVector::Distance(ViewerPosWorld, SurfacePos);

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


    for (UClipmapMeshComponent* Mesh : Levels)
    {
        if (!Mesh) continue;

        // Mover la malla procedimental
        Mesh->SetWorldLocationAndRotation(SurfacePos, PatchRotation);
    }

    /*if (FarLevel) {
        FarLevel->SetWorldLocationAndRotation(SurfacePos, PatchRotation);
    }*/

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

void UCosmicClipmapComponent::UpdateOrigins()
{

}


