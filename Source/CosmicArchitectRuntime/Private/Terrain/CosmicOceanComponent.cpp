

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
    // Limpiar oceano anterior antes de regenerar
    if (bInit)
    { 
        ClearOcean();
    }

    FName ComponentName = *FString::Printf(TEXT("TerrainOceanMesh_%d"), 0);

    UCosmicMeshComponent* Mesh = NewObject<UCosmicMeshComponent>(
        GetOwner(),
        ComponentName,
        RF_Transient | RF_DuplicateTransient  // Marcar como transitorio
    );

    if (Mesh)
    {
        Mesh->RegisterComponent();

        // Adjuntar la malla al componente raiz indicado
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
    // Crear instancia dinamica del material del oceano
    if (OceanMaterial) {

        DynamicOceanMat = UMaterialInstanceDynamic::Create(OceanMaterial, this);

    }
    else {
        DynamicOceanMat = nullptr;
    }

    // Aplicar material a la malla
    if (OceanMesh)
    {
        OceanMesh->SetMaterial(0, DynamicOceanMat);
    }
}


void UCosmicOceanComponent::ClearOcean()
{
    if (!bInit || !OceanMesh) return;

    // Limpiar recursos asociados a la malla procedural
    OceanMesh->ClearAllMeshSections();
    OceanMesh->CancelAsyncWork();
    OceanMesh->DestroyComponent();
    OceanMesh = nullptr;

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

    // Cambios que requieren reconstruccion completa
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, OceanResolution) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, SeaLevelKm))
    {
        RegenerateOcean();
        return;
    }

    // Actualizar material base del oceano
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

    // Actualizar posicion global del planeta en el material dinamico
    if (bInit && DynamicOceanMat)
    {
        DynamicOceanMat->SetVectorParameterValue("PlanetCenter", GetOwner()->GetActorLocation());
    }
}

