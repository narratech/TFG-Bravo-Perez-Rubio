// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "ProceduralMeshComponent.h" // For FProcMeshTangent backward compatibility
#include "CosmicFastMeshComponent.generated.h"

class FPrimitiveSceneProxy;

/**
 * Payload passed from Game Thread to Render Thread when updating dynamic buffers.
 */
struct FCosmicFastMeshUpdatePayload
{
	int32 SectionIndex = 0;
	TArray<FVector3f> Positions;
	TArray<FVector3f> Normals;
	TArray<FColor> Colors;
	FBox LocalBox = FBox(ForceInit);
};

/**
 * Internal section data representation in UCosmicFastMeshComponent.
 */
struct FCosmicFastMeshSectionData
{
	TArray<FVector3f> Positions;
	TArray<FVector3f> Normals;
	TArray<FVector2f> UV0;
	TArray<FColor> Colors;
	TArray<FVector3f> Tangents;
	TArray<uint32> Indices;
	FBox LocalBox = FBox(ForceInit);
	bool bSectionVisible = true;
};

/**
 * Highly optimized mesh component tailored for planetary clipmap rendering.
 *
 * Replaces UProceduralMeshComponent by:
 * - Eliminating all collision / physics cooking overhead (managed externally by UCosmicCollisionComponent).
 * - Separating static topology (triangles, UVs) from dynamic attributes (positions, normals, colors).
 * - Avoiding the heavy 152-byte FProcMeshVertex intermediate structure and deep copies in Game Thread.
 * - Supporting fast direct block memory copies to GPU vertex buffers on the Render Thread.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COSMICARCHITECTRUNTIME_API UCosmicFastMeshComponent : public UMeshComponent
{
	GENERATED_BODY()

public:

	UCosmicFastMeshComponent(const FObjectInitializer& ObjectInitializer);

	/**
	 * Creates the single procedural mesh section with static topology and initial attributes.
	 *
	 * @param SectionIndex Must be 0 for clipmap levels.
	 * @param Vertices Vertex positions.
	 * @param Triangles Triangle index list.
	 * @param Normals Optional normal vectors.
	 * @param UV0 Optional texture coordinates.
	 * @param VertexColors Optional linear vertex colors.
	 * @param Tangents Optional tangent vectors.
	 * @param bCreateCollision Ignored (collision is managed by UCosmicCollisionComponent).
	 * @param bSRGBConversion Whether to convert linear color to sRGB FColor.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|CosmicFastMesh")
	void CreateMeshSection_LinearColor(
		int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0,
		const TArray<FLinearColor>& VertexColors,
		const TArray<FProcMeshTangent>& Tangents,
		bool bCreateCollision = false,
		bool bSRGBConversion = false
	);

	/**
	 * Creates mesh section with multiple UV channels (UV1..UV3 ignored for clipmap, keeping 1st UV channel).
	 */
	void CreateMeshSection_LinearColor(
		int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FVector2D>& UV2,
		const TArray<FVector2D>& UV3,
		const TArray<FLinearColor>& VertexColors,
		const TArray<FProcMeshTangent>& Tangents,
		bool bCreateCollision = false,
		bool bSRGBConversion = false
	)
	{
		CreateMeshSection_LinearColor(SectionIndex, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, bCreateCollision, bSRGBConversion);
	}

	/**
	 * Updates the procedural mesh section attributes.
	 * Bypasses FProcMeshVertex and updates only position, normal and color buffers.
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|CosmicFastMesh")
	void UpdateMeshSection_LinearColor(
		int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0,
		const TArray<FLinearColor>& VertexColors,
		const TArray<FProcMeshTangent>& Tangents,
		bool bSRGBConversion = true
	);

	void UpdateMeshSection_LinearColor(
		int32 SectionIndex,
		const TArray<FVector>& Vertices,
		const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1,
		const TArray<FVector2D>& UV2,
		const TArray<FVector2D>& UV3,
		const TArray<FLinearColor>& VertexColors,
		const TArray<FProcMeshTangent>& Tangents,
		bool bSRGBConversion = true
	)
	{
		UpdateMeshSection_LinearColor(SectionIndex, Vertices, Normals, UV0, VertexColors, Tangents, bSRGBConversion);
	}

	/**
	 * Fast move-based update using GPU-ready float buffers (Zero-Copy on Game Thread).
	 */
	void UpdateMeshSection_Fast(
		TArray<FVector3f>&& InPositions,
		TArray<FVector3f>&& InNormals,
		TArray<FColor>&& InColors,
		const FBox& InBounds
	);

	/**
	 * Fast move-based update taking FVector arrays and converting directly without intermediate struct overhead.
	 */
	void UpdateMeshSection_FastVectors(
		TArray<FVector>&& InPositions,
		TArray<FVector>&& InNormals,
		TArray<FLinearColor>&& InColors,
		bool bSRGBConversion = true
	);

	/** Clears section data and releases render proxy */
	UFUNCTION(BlueprintCallable, Category = "Components|CosmicFastMesh")
	void ClearMeshSection(int32 SectionIndex);

	/** Clears all mesh sections */
	UFUNCTION(BlueprintCallable, Category = "Components|CosmicFastMesh")
	void ClearAllMeshSections();

	/** Toggles visibility of section without recreating scene proxy */
	UFUNCTION(BlueprintCallable, Category = "Components|CosmicFastMesh")
	void SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility);

	/** Returns visibility of section */
	UFUNCTION(BlueprintCallable, Category = "Components|CosmicFastMesh")
	bool IsMeshSectionVisible(int32 SectionIndex) const;

	/** Returns number of sections currently created (0 or 1) */
	UFUNCTION(BlueprintCallable, Category = "Components|CosmicFastMesh")
	int32 GetNumSections() const;

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	//~ End UMeshComponent Interface.

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End USceneComponent Interface.

protected:

	/** Stored section data for initializing scene proxy */
	FCosmicFastMeshSectionData SectionData;

	/** Indicates if mesh section is currently created */
	bool bSectionCreated = false;

	/** Section visibility */
	bool bSectionVisible = true;

	/** Local space bounds */
	FBoxSphereBounds LocalBounds;

	friend class FCosmicFastMeshSceneProxy;
};
