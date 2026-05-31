

#include "Terrain/CosmicClipmapComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Terrain/CosmicMeshComponent.h"
#include "Terrain/CosmicCollisionComponent.h"
#include "CosmicNoiseClass.h"
#include "CosmicDefaultNoiseStrategy.h"
#include "CosmicFoliageSpawner.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture.h"
#include "DrawDebugHelpers.h"
#include "ModulesBridge/CosmicCameraBridge.h"
#include "GameFramework/Pawn.h"


UCosmicClipmapComponent::UCosmicClipmapComponent()
{ 

    bTickInEditor = true;

	PrimaryComponentTick.bCanEverTick = true;
}

bool UCosmicClipmapComponent::UpdateCollisionNearPlayer(const FVector& SurfacePos, const FVector& SurfaceNormal, const double DistanceToSurface)
{
    if (!CollisionComponent) return false;

    // Solo generar colisión si el jugador está cerca de la superficie
    if (DistanceToSurface < CollisionComponent->MaxCollisionDistance)
    {
        if (!CollisionComponent->IsBuilt()) 
        {
            CollisionComponent->GenerateCollisionMesh(PlanetRadius);
        }
        CollisionComponent->SetWorldLocationAndRotation(
            SurfacePos,
            GetPatchRotation(SurfaceNormal),
            false, 
            nullptr,
            ETeleportType::TeleportPhysics // teleport limpio
        );
        CollisionComponent->UpdateCollisionMesh(NoiseGenerationStrategy, CurrentActorPosition);
        return true;
    }
    else if(CollisionComponent->IsBuilt())
    {
        // Limpiar colisión si está lejos
        CollisionComponent->ClearCollision();
        return true;
    }

    return false;
}


void UCosmicClipmapComponent::BeginPlay()
{

    Super::BeginPlay();

    TimeToRefreshActive = TimeToRefresh;

    // Inicializar valores para el shift
    LastSurfaceAngles = FVector2D::ZeroVector;
    AccumulatedLinearDelta = FVector2D::ZeroVector;

    FVector SurfacePos;
    FVector N;
    FVector ViewerPos;
    float DistanceToSurface;

    bPerformaceMode = true;

    ElapsedTime = FMath::FRandRange(0.f, TimeToRefresh);

    DistanceToSurface = GetDistanceToSurface(ViewerPos, SurfacePos, N);

    UpdateMeshPhase(ViewerPos, SurfacePos, N, DistanceToSurface);

    UpdateCollisionNearPlayer(SurfacePos, N, DistanceToSurface);

    LastSurfaceAngles = GetSurfaceAngles(SurfacePos);
    LastPlayerPos = ViewerPos;
    LastMeshPlayerPos = ViewerPos;
}

void UCosmicClipmapComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {

    ClearLevels();
    Super::EndPlay(EndPlayReason);
}

void UCosmicClipmapComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ElapsedTime += DeltaTime;

    if (DynamicPlanetMat) {
        DynamicPlanetMat->SetVectorParameterValue("PlanetCenter", GetOwner()->GetActorLocation());
    }

    if (ElapsedTime <= TimeToRefresh)
        return;

    if (!UseClipmap && bPerformanceBuild)
        return;

    ElapsedTime = ElapsedTime - TimeToRefresh;

    FVector SurfacePos;
    FVector N;
    FVector ViewerPos;
    float DistanceToSurface;  
 
    if (!bPerformaceMode) {

        bool UpdateFoliageExtra = false;
        DistanceToSurface = GetDistanceToSurface(ViewerPos, SurfacePos, N);

        double PhaseStart = FPlatformTime::Seconds();

        // EJECUCION POR FASE
        switch (CurrentPhase)
        {
        case EUpdatePhase::Foliage:
            UpdateFoliagePhase(DeltaTime, SurfacePos + N * DistanceToSurface, DistanceToSurface);
            break;

        case EUpdatePhase::Collision:
            UpdateFoliageExtra = !UpdateCollisionPhase(ViewerPos, SurfacePos, N, DistanceToSurface);
            break;

        case EUpdatePhase::Mesh:
            UpdateMeshPhase(ViewerPos, SurfacePos, N, DistanceToSurface);
            break;
        }

        //Si no hay que actualizar colisión pedimos actualizar foliage
        if (UpdateFoliageExtra)
        {
            //CurrentPhase = EUpdatePhase::Foliage;
            UpdateFoliagePhase(DeltaTime, SurfacePos + N * DistanceToSurface, DistanceToSurface);
        }
        
        CurrentPhase = (EUpdatePhase)(((uint8)CurrentPhase + 1) % 3);
    }
    else
    {
        DistanceToSurface = GetFastDistanceToSurface(ViewerPos, SurfacePos, N);
        UpdateMeshPhase(ViewerPos, SurfacePos, N, DistanceToSurface);
    }
}

void UCosmicClipmapComponent::UpdateFoliagePhase(float DeltaTime, const FVector& ViewerPos, float DistanceToSurface)
{
    if (FoliageSpawnerComponent)
    {
        FoliageSpawnerComponent->UpdateFoliageSpawner(
            TimeToRefresh, ViewerPos, CurrentActorPosition,
            PlanetRadius, DistanceToSurface, NoiseGenerationStrategy
        );
    }
}

bool UCosmicClipmapComponent::UpdateCollisionPhase(const FVector& ViewerPos, const FVector& SurfacePos,
    const FVector& N, float DistanceToSurface)
{
    if (CollisionComponent &&
        !LastMeshPlayerPos.Equals(ViewerPos, CollisionComponent->CollisionTriangleSize))
    {
        LastMeshPlayerPos = ViewerPos;
        return UpdateCollisionNearPlayer(SurfacePos, N, DistanceToSurface);
    }
    return false;
}

void UCosmicClipmapComponent::UpdateMeshPhase(const FVector& ViewerPos, const FVector& SurfacePos,
    const FVector& N, float DistanceToSurface)
{
    if (!FarLevel) return;

    // Actualización permanente del FarLevel (rendimiento) 
    if (!bPerformanceBuild)
    {
        FarLevel->RequestMeshUpdate(NoiseGenerationStrategy);
        bPerformanceBuild = FarLevel->CheckAndApplyMeshUpdate();
    }

    if (!UseClipmap) return;

    // Detección de cambio de modo 
    bool bPrevPerformanceMode = bPerformaceMode;
    bPerformaceMode = DistanceToSurface > PlanetRadius * HeightVisibility;

    // Crear niveles normales si es necesario y no estamos en rendimiento 
    if (!bInit && !bPerformaceMode)
    {
        CreateLevels();
    }

    // Manejar transiciones de modo
    if (bPrevPerformanceMode != bPerformaceMode)
    {
        if (bPerformaceMode) // Ir a modo rendimiento (esperar tareas pendientes)
        {
            // Activar FarLevel inmediatamente (ya está construido)
            FarLevel->SetMeshActive(true);

            for (size_t i = 0; i < Levels.Num(); i++)
            {
                Levels[i]->SetMeshActive(false);
            }
            // Iniciar espera para ocultar niveles cuando sus tareas terminen
            bWaitingForPerformanceTransition = true;
        }
        else // Ir a modo normal (esperar a que los niveles estén listos)
        {
            bWaitingForNormalTransition = true;

            const FRotator Rotation = GetPatchRotation(N);

            for (size_t i = 0; i < Levels.Num(); i++)
            {
                Levels[i]->SetPositionAndRotation(SurfacePos - CurrentActorPosition, Rotation);
                Levels[i]->RequestMeshUpdate(NoiseGenerationStrategy);
            }

            return;
        }
    }

    // Si estamos en espera de transición a rendimiento
    if (bWaitingForPerformanceTransition)
    {
        bool allTasksDone = true;
        for (size_t i = 0; i < Levels.Num(); i++)
        {
            if (Levels[i]->IsTaskActive())
            {
                allTasksDone = false;
                break;
            }
        }

        if (!allTasksDone)
        {
            return;  // Seguir esperando, FarLevel ya está visible mientras tanto
        }

        // Tareas terminadas: descartar resultados y ocultar niveles
        for (size_t i = 0; i < Levels.Num(); i++)
        {
            Levels[i]->CancelAsyncWork();
        }

        bWaitingForPerformanceTransition = false;
        return;  // No continuar con la lógica normal
    }

    // Si estamos en espera de transición a normal
    if (bWaitingForNormalTransition)
    {
        bool allTasksDone = true;
        for (size_t i = 0; i < Levels.Num(); i++)
        {
            if (Levels[i]->IsTaskActive())
            {
                allTasksDone = false;
                break;
            }
        }

        if (!allTasksDone)
        {
            return;  // Seguir esperando, FarLevel sigue visible
        }

        // Todas las tareas terminadas: aplicar resultados y mostrar niveles
        for (size_t i = 0; i < Levels.Num(); i++)
        {
            Levels[i]->CheckAndApplyMeshUpdate();
            Levels[i]->SetMeshActive(true);
        }
        FarLevel->SetMeshActive(false);

        bWaitingForNormalTransition = false;
    }

    // Si estamos en modo rendimiento o congelado, no continuar 
    if (bPerformaceMode || FreezeGeneration) return;

    // Verificar si las actualizaciones han terminado
    bool MeshesUpdated = true;
    for (size_t i = 0; i < Levels.Num(); i++)
    {
        if (Levels[i]->IsTaskActive())
        {
            MeshesUpdated = false;
            break;
        }
    }

    if (!MeshesUpdated) return;

    // Tareas terminadas: aplicar actualizaciones
    for (size_t i = 0; i < Levels.Num(); i++)
    {
        Levels[i]->CheckAndApplyMeshUpdate();
    }

    // Lógica normal de clipmap (modo normal ya establecido) 

    // Verificar si hay que reducir o incrementar niveles del clipmap
    bool UpdateClipmapLevels = false;
    if (Levels.Num() > 1)
    {
        UCosmicMeshComponent* MeshLast = Levels.Last();
        UCosmicMeshComponent* MeshFirst = Levels[0];

        const bool bLastVisible = IsClipmapRingVisible(Levels.Num() - 1, DistanceToSurface);

        if (!bLastVisible && MeshFirst->GridSpacing > MinTriangleSize)
        {
            const int32 Steps = CalculateDecreaseSteps(DistanceToSurface);
            DecreaseClipmapLevelFull(Steps);
            UpdateClipmapLevels = true;
        }
        else if (IsClipmapRingVisible(MeshLast->GridSpacing * 2, MeshLast->Resolution, DistanceToSurface)
            && MeshLast->GridSpacing < BaseSpacing * FMath::Pow(2.0f, NumLevels - 1))
        {
            const int32 Steps = CalculateIncreaseSteps(DistanceToSurface);
            IncreaseClipmapLevelFull(Steps);
            UpdateClipmapLevels = true;
        }
    }

    // Calcular desplazamiento necesario
    FIntPoint Shift = ComputeGridShiftSpherical(ViewerPos, SurfacePos, BaseGridSpacing * BaseResolution / 4);
    TotalShift += Shift;

    // Si hubo desplazamiento o cambios en los niveles, solicitar nuevas actualizaciones
    if (Shift != FIntPoint::ZeroValue || UpdateClipmapLevels)
    {
        const FRotator Rotation = GetPatchRotation(N);

        for (size_t i = 0; i < Levels.Num(); i++)
        {
            Levels[i]->SetPositionAndRotation(SurfacePos - CurrentActorPosition, Rotation);
            Levels[i]->RequestMeshUpdate(NoiseGenerationStrategy);
        }
    } 
}


#if WITH_EDITOR
void UCosmicClipmapComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // REBUILD COMPLETO 
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicClipmapComponent, BaseResolution) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicClipmapComponent, NumLevels))
    {
        ClearLevels();

        CreatePerformanceLevel(true);

        if (!bPerformaceMode)
        {
            CreateLevels();
        }

        return;
    }

    //  MATERIAL BASE / TEXTURA
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicClipmapComponent, BaseMaterial) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicClipmapComponent, DefaultTexture))
    {
        BuildDynamicMaterial();
        return;
    }

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicClipmapComponent, UseClipmap))
    {
        if (!UseClipmap)
        {
            ClearLevels();

            CreatePerformanceLevel(true);
        }
        return;
    }

}
#endif

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

    // Validar parámetros
    if (NumLevels <= 0 || BaseResolution <= 0 || PlanetRadius <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Parámetros inválidos para CreateLevels"));
        return;
    }

    int32 Remainder = BaseResolution % 4;

    if (Remainder != 0) {
        BaseResolution += 4 - Remainder;
    }

    Levels.Empty();
    Levels.SetNum(NumLevels);

    TotalShift = FIntPoint::ZeroValue;

    AActor* Owner = GetOwner();
    if (!Owner) LastPlayerPos = FVector::Zero();
    else LastPlayerPos = Owner->GetActorLocation();
        
    BaseGridSpacing = BaseSpacing = (PlanetRadius * 2.0f) / (BaseResolution * FMath::Pow(2.0f, NumLevels - 1));
     
    FVector SurfacePos;
    FVector N;
    FVector ViewerPos;

    if (IsPlanet) {
        GetDistanceToSurface(ViewerPos, SurfacePos, N);
    }

    FRotator PatchRotation = GetPatchRotation(N);

    // Crear cada nivel
    for (int32 L = 0; L < NumLevels; ++L)
    {
        // Crear nombre único para el componente
        FName ComponentName = *FString::Printf(TEXT("TerrainClipmapMesh_Level_%d"), L);

        // Crear componente
        UCosmicMeshComponent* Mesh = NewObject<UCosmicMeshComponent>(
            GetOwner(),
            ComponentName,
            RF_Transient | RF_DuplicateTransient  // Marcar como transitorio
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
        Mesh->bIsPlanet = true;

        Mesh->SetPositionAndRotation(SurfacePos - CurrentActorPosition, PatchRotation);

        // Construir malla
        Mesh->BuildBaseProjectedMesh();
        Mesh->SetMeshActive(false);
        Mesh->RequestMeshUpdate(NoiseGenerationStrategy);
        
        // Asignar material
        if (DynamicPlanetMat)
        {
            Mesh->SetMaterial(0, DynamicPlanetMat);
        }

        // Guardar referencia
        Levels[L] = Mesh;
    }
   
    bInit = true;
}

void UCosmicClipmapComponent::CreatePerformanceLevel(bool bActive)
{
    if (FarLevel) return;

    FName ComponentName = *FString::Printf(TEXT("TerrainClipmapMesh_Performance_%d"), 0);

    UCosmicMeshComponent* Mesh = NewObject<UCosmicMeshComponent>(
        GetOwner(),
        ComponentName,
        RF_Transient | RF_DuplicateTransient  // Marcar como transitorio
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
        Mesh->bIsPlanet = false;

        Mesh->BuildSphereMesh();
        Mesh->SetMeshActive(bActive);

        FarLevel = Mesh;

        BuildDynamicMaterial();
    }

    UpdateNoiseEvaluator();

    bPerformaceMode = true;

    bInit = false;
}


void UCosmicClipmapComponent::ClearLevels()
{
    bInit = false;

    int LevelsCleared = 0;  

    TotalShift = FIntPoint::ZeroValue;

    // Destruir componentes del array Levels
    for (UCosmicMeshComponent* Mesh : Levels)
    {
        // Desactivar y limpiar la malla
        Mesh->ClearAllMeshSections();
        Mesh->CancelAsyncWork();
        Mesh->DestroyComponent();
        Mesh = nullptr;
    }

    // Destruir nivel exterior
    if (FarLevel)
    {
        FarLevel->ClearAllMeshSections();
        FarLevel->CancelAsyncWork();
        FarLevel->DestroyComponent();
        FarLevel = nullptr;
    }

    Levels.Empty();

    if (CollisionComponent && CollisionComponent->IsBuilt())
    {
        CollisionComponent->ClearCollision();
    }


    DynamicPlanetMat = nullptr;
    

    bPerformanceBuild = false;
}

void UCosmicClipmapComponent::SetMaterialData(FColor Color1, FColor Color2, FColor ColorCold, FColor ColorHot,
    FColor ColorSlope, float ScaleL, float ScaleM, float ScaleS)
{
    PlanetMainColor1 = Color1;
    PlanetMainColor2 = Color2;
    PlanetColdColor = ColorCold;
    PlanetHotColor = ColorHot;
    PlanetSlopeColor = ColorSlope;
    NoiseScaleLarge = ScaleL;
    NoiseScaleMedium = ScaleM;
    NoiseScaleSmall = ScaleS;

    if (DynamicPlanetMat)
    {
        DynamicPlanetMat->SetScalarParameterValue(FName("PlanetRadius"), PlanetRadius);
        DynamicPlanetMat->SetVectorParameterValue(FName("BaseColor"), Color1);
        DynamicPlanetMat->SetVectorParameterValue(FName("MidColor"), Color2);
        DynamicPlanetMat->SetVectorParameterValue(FName("ColdColor"), ColorCold);
        DynamicPlanetMat->SetVectorParameterValue(FName("HotColor"), ColorHot);
        DynamicPlanetMat->SetVectorParameterValue(FName("SlopeColor"), ColorSlope);
        DynamicPlanetMat->SetScalarParameterValue(FName("NoiseScaleLarge"), ScaleL);
        DynamicPlanetMat->SetScalarParameterValue(FName("NoiseScaleMedium"), ScaleM);
        DynamicPlanetMat->SetScalarParameterValue(FName("NoiseScaleSmall"), ScaleS);
        if(DefaultTexture)
            DynamicPlanetMat->SetTextureParameterValue(FName("Floortexture"), DefaultTexture);
    }
}

void UCosmicClipmapComponent::BuildDynamicMaterial() 
{
    if (BaseMaterial) {

        DynamicPlanetMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);

        SetMaterialData(PlanetMainColor1, PlanetMainColor2, PlanetColdColor, PlanetHotColor,
            PlanetSlopeColor, NoiseScaleLarge, NoiseScaleMedium, NoiseScaleSmall);
    } 
    else {
        DynamicPlanetMat = nullptr;
    }

    for (size_t i = 0; i < Levels.Num(); i++)
    {
        Levels[i]->SetMaterial(0, DynamicPlanetMat);
    }

    if (FarLevel)
        FarLevel->SetMaterial(0, DynamicPlanetMat);
}

void UCosmicClipmapComponent::UpdateNoiseEvaluator()
{
    if (NoiseClass)
    {
        NoiseGenerationStrategy = NoiseClass->CreateStrategy();
    }
    else
    {
        // fallback default
        TSharedPtr<FCosmicDefaultNoiseStrategy> Strategy = MakeShared<FCosmicDefaultNoiseStrategy>();

        Strategy->Initialize(
            1337,
            FCosmicNoiseLayer(), 
            FCosmicNoiseBiomeParameters()
        );

        NoiseGenerationStrategy = Strategy;
    }
}

void UCosmicClipmapComponent::RequestCompleteMeshUpdate()
{
    bPerformanceBuild = false;

    UpdateNoiseEvaluator();
    
    if(!bPerformaceMode)
    {
        for (size_t i = 0; i < Levels.Num(); i++)
        {        
            Levels[i]->RequestMeshUpdate(NoiseGenerationStrategy);
        }
    }

    if (FoliageSpawnerComponent)
    {
        FoliageSpawnerComponent->ClearFoliage();
    } 
}

FRotator UCosmicClipmapComponent::GetPatchRotation(const FVector& N) const
{
    const FVector Up = N;

    // Elegimos un vector no colineal 
    const FVector Tangent = (FMath::Abs(Up.Z) < 0.99f)
        ? FVector(0, 0, 1)
        : FVector(1, 0, 0);

    // Right sale normalizado si Up y Tangent son unitarios
    FVector Right = FVector::CrossProduct(Tangent, Up);
    Right.Normalize(); 

    const FVector Forward = FVector::CrossProduct(Up, Right); // ya unitario

    return FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();
}

FIntPoint UCosmicClipmapComponent::ComputeGridShiftPlanar(
    const FVector& PlayerPos,
    float GridSpacing)
{
    FVector PlanetCenter = GetOwner()->GetActorLocation();

    FVector FrameDelta = PlayerPos - LastPlayerPos;

    LastPlayerPos = PlayerPos;

    AccumulatedDelta += FrameDelta;

    int32 ShiftX = FMath::FloorToInt(AccumulatedDelta.X / GridSpacing);
    int32 ShiftY = FMath::FloorToInt(AccumulatedDelta.Y / GridSpacing);

    AccumulatedDelta.X -= ShiftX * GridSpacing;
    AccumulatedDelta.Y -= ShiftY * GridSpacing;

    return FIntPoint(ShiftX, ShiftY);
}

FIntPoint UCosmicClipmapComponent::ComputeGridShiftSpherical(const FVector& PlayerPos, const FVector& CurrentSurfacePos, int64 GridSpacing)
{
    FVector PlanetCenter = CurrentActorPosition;

    // Si es la primera vez, no hay movimiento
    if (LastPlayerPos.IsZero())
    {
        LastPlayerPos = PlayerPos;
        LastSurfaceAngles = GetSurfaceAngles(CurrentSurfacePos);
        return FIntPoint::ZeroValue;
    }

    // Obtener ángulos esféricos (longitud y latitud) de ambas posiciones
    FVector2D CurrentAngles = GetSurfaceAngles(CurrentSurfacePos - CurrentActorPosition);
    FVector2D PreviousAngles = LastSurfaceAngles;

    // Calcular el desplazamiento angular (en radianes)
    FVector2D DeltaAngles = CurrentAngles - PreviousAngles;

    // Normalizar la longitud al rango [-PI, PI] para tomar el camino más corto
    if (DeltaAngles.X > PI) DeltaAngles.X -= 2 * PI;
    if (DeltaAngles.X < -PI) DeltaAngles.X += 2 * PI;

    // Convertir el desplazamiento angular a distancia lineal en la superficie
    FVector2D LinearDelta = DeltaAngles * PlanetRadius;

    // Acumular el desplazamiento lineal
    AccumulatedLinearDelta += LinearDelta;

    // Calcular cuántos "grid steps" nos hemos movido
    // GridSpacing es la distancia entre vértices en el plano tangente
    int32 ShiftX = FMath::FloorToInt(AccumulatedLinearDelta.X / GridSpacing);
    int32 ShiftY = FMath::FloorToInt(AccumulatedLinearDelta.Y / GridSpacing);

    // Restar lo que ya hemos usado
    AccumulatedLinearDelta.X -= ShiftX * GridSpacing;
    AccumulatedLinearDelta.Y -= ShiftY * GridSpacing;

    // Guardar para el próximo frame
    LastPlayerPos = PlayerPos;
    LastSurfaceAngles = CurrentAngles;
    PreviousSurfacePos = CurrentSurfacePos;

    return FIntPoint(ShiftX, ShiftY);
}

FIntPoint UCosmicClipmapComponent::ComputeGridShift(const FVector& PlayerPos, const FVector& CurrentSurfacePos, float GridSpacing)
{
    if (IsPlanet)
    {
        // Usar la versión esférica
        return ComputeGridShiftSpherical(PlayerPos, CurrentSurfacePos, GridSpacing);
    }
    else
    {
        // Usar la versión plana original
        return ComputeGridShiftPlanar(PlayerPos, GridSpacing);
    }
}

FVector2D UCosmicClipmapComponent::GetSurfaceAngles(const FVector& SurfacePos)
{
    // Asumiendo que el centro del planeta está en (0,0,0) o ajustando
    FVector Normalized = SurfacePos.GetSafeNormal();

    // Longitud: ángulo en el plano XY (-PI a PI)
    double Longitude = FMath::Atan2(Normalized.Y, Normalized.X);

    // Latitud: ángulo desde el ecuador (-PI/2 a PI/2)
    double Latitude = FMath::Asin(FMath::Clamp(Normalized.Z, -0.999999f, 0.999999f));

    return FVector2D(Longitude, Latitude);
}

FVector UCosmicClipmapComponent::GetPlayerLocation()
{
    FVector PlayerLocation = FVector::ZeroVector;

    if (GetWorld()->IsGameWorld())
    {
        APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
        if (PC)
        {
            APawn* Pawn = PC->GetPawn();
            if (Pawn)
            {
                PlayerLocation = Pawn->GetActorLocation();
            }
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

double UCosmicClipmapComponent::GetDistanceToSurface(
    FVector& OutViewerPos,
    FVector& OutSurfacePos,
    FVector& OutN)
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    OutViewerPos = GetPlayerLocation();
    CurrentActorPosition = Owner->GetActorLocation();

    FVector CenterToViewer = OutViewerPos - CurrentActorPosition;
    double DistanceToCenter = CenterToViewer.Length();

    OutN = CenterToViewer.GetSafeNormal();

    FLinearColor Dummy;
    float Height = 0;

    if(NoiseGenerationStrategy)
        NoiseGenerationStrategy->EvaluatePoint(OutN, Height, Dummy);

    double SurfaceRadius = PlanetRadius + Height;

    OutSurfacePos = CurrentActorPosition + OutN * PlanetRadius;

    double DistanceToSurface = DistanceToCenter - SurfaceRadius;

    if (DistanceToCenter <= SurfaceRadius)
    {
        return 0.0;
    }

    return DistanceToSurface;
}

double UCosmicClipmapComponent::GetFastDistanceToSurface(
    FVector& OutViewerPos,
    FVector& OutSurfacePos,
    FVector& OutN)
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    OutViewerPos = GetPlayerLocation();
    CurrentActorPosition = Owner->GetActorLocation();

    FVector CenterToViewer = OutViewerPos - CurrentActorPosition;
    double DistanceToCenter = CenterToViewer.Length();

    OutN = CenterToViewer.GetSafeNormal();

    OutSurfacePos = CurrentActorPosition + OutN * PlanetRadius;

    double DistanceToSurface = DistanceToCenter - PlanetRadius;

    if (DistanceToCenter <= PlanetRadius)
    {
        return 0.0;
    }

    return DistanceToSurface;
}

float UCosmicClipmapComponent::GetDistanceToPlainSurface(FVector& OutViewerPos, FVector& OutSurfacePos, FVector& OutN)
{
    AActor* Owner = GetOwner();
    if (!Owner) return 0.f;

    // Posición del viewer
    OutViewerPos = GetPlayerLocation();

    // Punto base del plano 
    CurrentActorPosition = Owner->GetActorLocation();

    // Normal del plano
    OutN = FVector::UpVector;

    // Vector del plano al viewer
    FVector PlaneToViewer = OutViewerPos - CurrentActorPosition;

    // Distancia firmada al plano
    float Distance = FVector::DotProduct(PlaneToViewer, OutN);

    // Proyeccion del viewer sobre el plano 
    OutSurfacePos = OutViewerPos - Distance * OutN;

    return Distance;
}

int32 UCosmicClipmapComponent::CalculateDecreaseSteps(const double DistanceToSurface) const
{
    // hay que bajar al menos 1, buscamos el mínimo n tal que
    // el último anillo sea visible tras n halvings
    int32 Steps = 1;
    const int64 LastResolution = Levels.Last()->Resolution;
    const int64 LastSpacing = Levels.Last()->GridSpacing;
    const int64 FirstSpacing = Levels[0]->GridSpacing;

    while (!IsClipmapRingVisible(LastSpacing >> Steps, LastResolution, DistanceToSurface))
    {
        // No bajar más si el primer nivel ya tocaría el límite mínimo
        if ((FirstSpacing >> (Steps + 1)) <= MinTriangleSize)
            break;
        Steps++;
    }
    return Steps;
}

int32 UCosmicClipmapComponent::CalculateIncreaseSteps(const double DistanceToSurface) const
{
    // buscamos cuántos doublings
    // consecutivos siguen siendo visibles sin superar el spacing máximo permitido
    int32 Steps = 1;
    const int64 LastResolution = Levels.Last()->Resolution;
    const int64 LastSpacing = Levels.Last()->GridSpacing;
    const int64 MaxSpacing = static_cast<int64>(BaseSpacing * FMath::Pow(2.0f, NumLevels - 1));

    while (IsClipmapRingVisible(LastSpacing << (Steps + 1), LastResolution, DistanceToSurface)
        && (LastSpacing << Steps) < MaxSpacing)
    {
        Steps++;
    }
    return Steps;
}


bool UCosmicClipmapComponent::IsClipmapRingVisible(const int32 LevelIndex, const double DistanceToSurface) const
{  
    
    // Calcular el radio del clipmap en la superficie
    int64 ClipmapSurfaceRadius = Levels[LevelIndex]->GridSpacing * (Levels[LevelIndex]->Resolution - 2) / 2;

    // Radio maximo visible desde esta altura (proyeccion en la superficie)
    double VisibleRadius = PlanetRadius * FMath::Sin(FMath::Acos(PlanetRadius / (PlanetRadius + DistanceToSurface)));

    // El clipmap es visible si su radio es menor que el radio visible
    return ClipmapSurfaceRadius <= VisibleRadius * 2.f; 
}

bool UCosmicClipmapComponent::IsClipmapRingVisible(const int64 GridSpacing, const int64 Resolution, const double DistanceToSurface) const
{
    int64 ClipmapSurfaceRadius = GridSpacing * (Resolution - 2) / 2;

    // Radio maximo visible desde esta altura (proyeccion en la superficie)
    double VisibleRadius = PlanetRadius * FMath::Sin(FMath::Acos(PlanetRadius / (PlanetRadius + DistanceToSurface)));

    // El clipmap es visible si su radio es menor que el radio visible
    return ClipmapSurfaceRadius <= VisibleRadius * 2.f;
}


void UCosmicClipmapComponent::DecreaseClipmapLevelFull(int32 Steps)
{
    if (NumLevels <= 1 || Steps <= 0) return;

    // Aplicar todos los halvings al BaseGridSpacing de una vez
    const int64 Divisor = static_cast<int64>(1) << Steps; // 2^Steps
    BaseGridSpacing /= Divisor;
    TotalShift *= Divisor;

    // Reconstruir spacings desde la nueva base
    float NewGridSpacing = BaseGridSpacing;
    for (int32 i = 0; i < NumLevels; i++)
    {
        Levels[i]->ReScaleLevel(NewGridSpacing);
        NewGridSpacing *= 2.f;
    }
}

void UCosmicClipmapComponent::IncreaseClipmapLevelFull(int32 Steps)
{
    if (NumLevels <= 1 || Steps <= 0) return;

    const int64 Multiplier = static_cast<int64>(1) << Steps; // 2^Steps
    BaseGridSpacing *= Multiplier;
    TotalShift /= Multiplier;

    // Reconstruir spacings desde la nueva base
    float NewGridSpacing = BaseGridSpacing;
    for (int32 i = 0; i < NumLevels; i++)
    {
        Levels[i]->ReScaleLevel(NewGridSpacing);
        NewGridSpacing *= 2.f;
    }
}





