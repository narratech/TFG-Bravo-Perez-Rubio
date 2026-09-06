// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "Terrain/CosmicOceanComponent.h"
#include "Terrain/CosmicMeshComponent.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"

UCosmicOceanComponent::UCosmicOceanComponent()
{

    bTickInEditor = true;
	PrimaryComponentTick.bCanEverTick = true;
}

void UCosmicOceanComponent::InitOcean(double PlanetRadiusKm, USceneComponent* Parent)
{
    PlanetRadiusCm = PlanetRadiusKm * 100000;
    ParentRoot = Parent;
}

void UCosmicOceanComponent::RegenerateOcean()
{
    // Clear previous ocean before regenerating
    if (bInit)
    { 
        ClearOcean();
    }

    FName ComponentName = *FString::Printf(TEXT("TerrainOceanMesh_%d"), 0);

    UCosmicMeshComponent* Mesh = NewObject<UCosmicMeshComponent>(
        GetOwner(),
        ComponentName,
        RF_Transient | RF_DuplicateTransient  // Mark as transient
    );

    if (Mesh)
    {
        Mesh->RegisterComponent();

        // Attach mesh to indicated root component
        if (ParentRoot)
        {
            Mesh->AttachToComponent(ParentRoot, FAttachmentTransformRules::KeepRelativeTransform);
        }

        Mesh->Resolution = OceanResolution;
        Mesh->bIsRing = false;
        Mesh->PlanetRadius = PlanetRadiusCm + SeaLevelKm * 100000;
        Mesh->bIsPlanet = false;

        Mesh->BuildSphereMesh();
        Mesh->SetMeshActive(true);

        OceanMesh = Mesh;

        BuildDynamicMaterial();
    }

    bInit = true;
}

void UCosmicOceanComponent::BuildDynamicMaterial()
{
    // Create dynamic instance of ocean material
    if (OceanMaterial) {

        DynamicOceanMat = UMaterialInstanceDynamic::Create(OceanMaterial, this);

    }
    else {
        DynamicOceanMat = nullptr;
    }

    // Apply material to mesh
    if (OceanMesh)
    {
        OceanMesh->SetMaterial(0, DynamicOceanMat);
    }
}


void UCosmicOceanComponent::ClearOcean()
{
    if (!bInit || !OceanMesh) return;

    // Clean up resources associated with procedural mesh
    OceanMesh->ClearAllMeshSections();
    OceanMesh->CancelAsyncWork();
    OceanMesh->DestroyComponent();
    OceanMesh = nullptr;

    bInit = false;
}

void UCosmicOceanComponent::ResetPointersAfterDuplicate(USceneComponent* NewRoot)
{
    ParentRoot = NewRoot;
    OceanMesh = nullptr;
    DynamicOceanMat = nullptr;
    bInit = false;
}

#if WITH_EDITOR
void UCosmicOceanComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, bHasOcean))
    {
        bHasOcean ? RegenerateOcean() : ClearOcean();
        return;
    }

    // Changes that require full rebuild
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, OceanResolution) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, SeaLevelKm))
    {
        RegenerateOcean();
        return;
    }

    // Update ocean base material
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, OceanMaterial))
    {
        BuildDynamicMaterial();
        return;
    }
}
#endif


void UCosmicOceanComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Update global planet position in dynamic material
    if (bInit && DynamicOceanMat)
    {
        DynamicOceanMat->SetVectorParameterValue("PlanetCenter", GetOwner()->GetActorLocation());
    }
}

