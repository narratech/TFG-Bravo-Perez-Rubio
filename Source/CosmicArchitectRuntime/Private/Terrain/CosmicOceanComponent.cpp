// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.


#include "Terrain/CosmicOceanComponent.h"
#include "Terrain/CosmicMeshComponent.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"
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
    UMaterialInterface* BaseMaterial = nullptr;

    if (bUseGeneratedMaterial)
    {
        // Load the default Gerstner material instance asset
        const TCHAR* DefaultOceanMatPath = TEXT("/CosmicArchitect/Resources/Materials/MI_CosmicOceanGerstner.MI_CosmicOceanGerstner");
        UMaterialInterface* DefaultMat = LoadObject<UMaterialInterface>(nullptr, DefaultOceanMatPath);

        if (DefaultMat)
        {
            BaseMaterial = DefaultMat;
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("CosmicOceanComponent: Default ocean material '%s' not found. "
                     "Falling back to OceanMaterial."), DefaultOceanMatPath);
            BaseMaterial = OceanMaterial;
        }
    }
    else
    {
        // Use the manually assigned material
        BaseMaterial = OceanMaterial;
    }

    // Create dynamic instance
    if (BaseMaterial)
    {
        DynamicOceanMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    }
    else
    {
        DynamicOceanMat = nullptr;
    }

    // Apply material to mesh
    if (OceanMesh)
    {
        OceanMesh->SetMaterial(0, DynamicOceanMat);
    }

    // Set initial wave parameters
    if (DynamicOceanMat && bUseGeneratedMaterial)
    {
        UpdateWaveParameters();
    }
}

void UCosmicOceanComponent::UpdateWaveParameters()
{
    if (!DynamicOceanMat || !bUseGeneratedMaterial) return;

    // Planet data
    DynamicOceanMat->SetScalarParameterValue(FName("PlanetRadius"), static_cast<float>(PlanetRadiusCm));

    // Wave globals
    DynamicOceanMat->SetScalarParameterValue(FName("WaveAmplitudeScale"), WaveAmplitudeScale);
    DynamicOceanMat->SetScalarParameterValue(FName("WaveSteepnessScale"), WaveSteepness);
    DynamicOceanMat->SetScalarParameterValue(FName("WaveSpeedScale"), WaveSpeed);

    // Action radius (convert km to cm)
    DynamicOceanMat->SetScalarParameterValue(FName("ActionRadius"), WaveActionRadiusKm * 100000.0f);
    DynamicOceanMat->SetScalarParameterValue(FName("ActionFalloff"), WaveActionFalloffKm * 100000.0f);

    // Appearance
    DynamicOceanMat->SetVectorParameterValue(FName("WaterShallowColor"), WaterShallowColor);
    DynamicOceanMat->SetVectorParameterValue(FName("WaterDeepColor"), WaterDeepColor);
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

    // Material mode switch or manual material change — rebuild dynamic material
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, bUseGeneratedMaterial) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, OceanMaterial))
    {
        BuildDynamicMaterial();
        return;
    }

    // Wave parameter changes — update dynamic material without rebuilding
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveAmplitudeScale) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveSteepness) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveSpeed) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveActionRadiusKm) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaveActionFalloffKm) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaterShallowColor) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(UCosmicOceanComponent, WaterDeepColor))
    {
        UpdateWaveParameters();
        return;
    }
}
#endif


void UCosmicOceanComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Update dynamic material parameters every frame
    if (bInit && DynamicOceanMat)
    {
        // Planet center always needs updating (planet may move in orbit)
        DynamicOceanMat->SetVectorParameterValue("PlanetCenter", GetOwner()->GetActorLocation());

        // Update wave parameters for generated material mode
        // (only PlanetRadius might change dynamically; wave params are typically static
        //  but we keep the radius in sync in case the planet is rescaled)
        if (bUseGeneratedMaterial)
        {
            DynamicOceanMat->SetScalarParameterValue(FName("PlanetRadius"), static_cast<float>(PlanetRadiusCm));
        }
    }
}
