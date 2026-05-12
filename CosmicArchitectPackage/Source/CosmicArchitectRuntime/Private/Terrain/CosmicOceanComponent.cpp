// Fill out your copyright notice in the Description page of Project Settings.


#include "Terrain/CosmicOceanComponent.h"
#include "Terrain/CosmicMeshComponent.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"

// Sets default values for this component's properties
UCosmicOceanComponent::UCosmicOceanComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
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
    if (OceanMaterial) {

        DynamicOceanMat = UMaterialInstanceDynamic::Create(OceanMaterial, this);

    }
    else {
        DynamicOceanMat = nullptr;
    }

    if (OceanMesh)
    {
        OceanMesh->SetMaterial(0, DynamicOceanMat);
    }
}


void UCosmicOceanComponent::ClearOcean()
{
    if (!bInit || !OceanMesh) return;

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

    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, OceanResolution) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, SeaLevelKm))
    {
        RegenerateOcean();
        return;
    }

    //  MATERIAL BASE
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, OceanMaterial))
    {
        BuildDynamicMaterial();
        return;
    }
}
#endif

// Called every frame
void UCosmicOceanComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bInit && DynamicOceanMat)
    {
        DynamicOceanMat->SetVectorParameterValue("PlanetCenter", GetOwner()->GetActorLocation());
    }
}

