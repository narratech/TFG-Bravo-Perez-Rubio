// Javier Bravo, David Rubio, Sergio Perez 2026 All Rights Reserved.

#include "Terrain/CosmicFastMeshComponent.h"
#include "PrimitiveSceneProxy.h"
#include "PrimitiveViewRelevance.h"
#include "DynamicMeshBuilder.h"
#include "StaticMeshResources.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "Engine/Engine.h"
#include "SceneInterface.h"
#include "SceneView.h"
#include "RenderUtils.h"
#include "PrimitiveUniformShaderParametersBuilder.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CosmicFastMeshComponent)

/**
 * Lightweight scene proxy tailored for UCosmicFastMeshComponent.
 */
class FCosmicFastMeshSceneProxy final : public FPrimitiveSceneProxy
{
public:

	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	FCosmicFastMeshSceneProxy(UCosmicFastMeshComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, VertexFactory(GetScene().GetFeatureLevel(), "FCosmicFastMeshSceneProxy")
		, Material(Component->GetMaterial(0))
		, bSectionVisible(Component->bSectionVisible)
		, MaterialRelevance(Component->GetMaterialRelevance(GetScene().GetShaderPlatform()))
	{
		const FCosmicFastMeshSectionData& SrcSection = Component->SectionData;
		const int32 NumVerts = SrcSection.Positions.Num();
		if (NumVerts > 0 && SrcSection.Indices.Num() > 0)
		{
			TArray<FDynamicMeshVertex> Vertices;
			Vertices.SetNumUninitialized(NumVerts);

			const bool bHasNormals = SrcSection.Normals.Num() == NumVerts;
			const bool bHasTangents = SrcSection.Tangents.Num() == NumVerts;
			const bool bHasUVs = SrcSection.UV0.Num() == NumVerts;
			const bool bHasColors = SrcSection.Colors.Num() == NumVerts;

			for (int32 i = 0; i < NumVerts; ++i)
			{
				FDynamicMeshVertex& V = Vertices[i];
				V.Position = SrcSection.Positions[i];
				V.Color = bHasColors ? SrcSection.Colors[i] : FColor::White;
				V.TextureCoordinate[0] = bHasUVs ? SrcSection.UV0[i] : FVector2f::ZeroVector;

				const FVector3f Normal = bHasNormals ? SrcSection.Normals[i] : FVector3f(0.f, 0.f, 1.f);
				const FVector3f Tangent = bHasTangents ? SrcSection.Tangents[i] : FVector3f(1.f, 0.f, 0.f);
				V.SetTangents(Tangent, FVector3f::CrossProduct(Normal, Tangent).GetSafeNormal(), Normal);
			}

			IndexBuffer.Indices = SrcSection.Indices;

			// Initialize static mesh buffers with 1 UV channel
			VertexBuffers.InitFromDynamicVertex(&VertexFactory, Vertices, 1);

			BeginInitResource(&VertexBuffers.PositionVertexBuffer);
			BeginInitResource(&VertexBuffers.StaticMeshVertexBuffer);
			BeginInitResource(&VertexBuffers.ColorVertexBuffer);
			BeginInitResource(&IndexBuffer);
			BeginInitResource(&VertexFactory);

			if (Material == nullptr)
			{
				Material = UMaterial::GetDefaultMaterial(MD_Surface);
			}
		}
	}

	virtual ~FCosmicFastMeshSceneProxy()
	{
		VertexBuffers.PositionVertexBuffer.ReleaseResource();
		VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
		VertexBuffers.ColorVertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
	}

	/**
	 * Direct block memory copy to GPU vertex buffers on the Render Thread.
	 * Bypasses UV buffer (UVs are static) and index buffer (topology is static).
	 */
	void UpdateSection_RenderThread(FRHICommandListBase& RHICmdList, TSharedPtr<FCosmicFastMeshUpdatePayload> UpdateData)
	{
		if (!UpdateData.IsValid())
		{
			return;
		}

		const int32 NumVerts = VertexBuffers.PositionVertexBuffer.GetNumVertices();
		if (NumVerts <= 0)
		{
			return;
		}

		// 1. Position buffer upload
		if (UpdateData->Positions.Num() > 0 && VertexBuffers.PositionVertexBuffer.VertexBufferRHI.IsValid())
		{
			const int32 CountToCopy = FMath::Min(UpdateData->Positions.Num(), NumVerts);
			const uint32 Stride = VertexBuffers.PositionVertexBuffer.GetStride();
			const uint32 BufferBytes = CountToCopy * Stride;

			// Update CPU mirror
			FMemory::Memcpy(VertexBuffers.PositionVertexBuffer.GetVertexData(), UpdateData->Positions.GetData(), BufferBytes);

			// Direct block copy into GPU memory
			void* VertexBufferData = RHICmdList.LockBuffer(VertexBuffers.PositionVertexBuffer.VertexBufferRHI, 0, BufferBytes, RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, UpdateData->Positions.GetData(), BufferBytes);
			RHICmdList.UnlockBuffer(VertexBuffers.PositionVertexBuffer.VertexBufferRHI);
		}

		// 2. Normal / Tangent buffer upload
		if (UpdateData->Normals.Num() > 0 && VertexBuffers.StaticMeshVertexBuffer.TangentsVertexBuffer.VertexBufferRHI.IsValid())
		{
			const int32 CountToCopy = FMath::Min(UpdateData->Normals.Num(), NumVerts);
			for (int32 i = 0; i < CountToCopy; ++i)
			{
				const FVector3f TangentX = VertexBuffers.StaticMeshVertexBuffer.VertexTangentX(i);
				const FVector3f TangentZ = UpdateData->Normals[i];
				const FVector3f TangentY = FVector3f::CrossProduct(TangentZ, TangentX).GetSafeNormal();
				VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i, TangentX, TangentY, TangentZ);
			}

			void* VertexBufferData = RHICmdList.LockBuffer(
				VertexBuffers.StaticMeshVertexBuffer.TangentsVertexBuffer.VertexBufferRHI,
				0,
				VertexBuffers.StaticMeshVertexBuffer.GetTangentSize(),
				RLM_WriteOnly
			);
			FMemory::Memcpy(VertexBufferData, VertexBuffers.StaticMeshVertexBuffer.GetTangentData(), VertexBuffers.StaticMeshVertexBuffer.GetTangentSize());
			RHICmdList.UnlockBuffer(VertexBuffers.StaticMeshVertexBuffer.TangentsVertexBuffer.VertexBufferRHI);
		}

		// 3. Color buffer upload
		if (UpdateData->Colors.Num() > 0 && VertexBuffers.ColorVertexBuffer.VertexBufferRHI.IsValid())
		{
			const int32 CountToCopy = FMath::Min(UpdateData->Colors.Num(), NumVerts);
			const uint32 Stride = VertexBuffers.ColorVertexBuffer.GetStride();
			const uint32 BufferBytes = CountToCopy * Stride;

			// Update CPU mirror
			FMemory::Memcpy(VertexBuffers.ColorVertexBuffer.GetVertexData(), UpdateData->Colors.GetData(), BufferBytes);

			// Direct block copy into GPU memory
			void* VertexBufferData = RHICmdList.LockBuffer(VertexBuffers.ColorVertexBuffer.VertexBufferRHI, 0, BufferBytes, RLM_WriteOnly);
			FMemory::Memcpy(VertexBufferData, UpdateData->Colors.GetData(), BufferBytes);
			RHICmdList.UnlockBuffer(VertexBuffers.ColorVertexBuffer.VertexBufferRHI);
		}
	}

	void SetSectionVisibility_RenderThread(bool bNewVisibility)
	{
		check(IsInRenderingThread());
		bSectionVisible = bNewVisibility;
	}

	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views,
		const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap,
		FMeshElementCollector& Collector
	) const override
	{
		if (!bSectionVisible || IndexBuffer.Indices.Num() == 0 || VertexBuffers.PositionVertexBuffer.GetNumVertices() == 0)
		{
			return;
		}

		const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

		FColoredMaterialRenderProxy* WireframeMaterialInstance = nullptr;
		if (bWireframe)
		{
			WireframeMaterialInstance = new FColoredMaterialRenderProxy(
				GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr,
				FLinearColor(0.f, 0.5f, 1.f)
			);
			Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);
		}

		FMaterialRenderProxy* MaterialProxy = bWireframe
			? WireframeMaterialInstance
			: (Material ? Material->GetRenderProxy() : UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy());

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				FMeshBatch& Mesh = Collector.AllocateMesh();
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = &IndexBuffer;
				Mesh.bWireframe = bWireframe;
				Mesh.VertexFactory = &VertexFactory;
				Mesh.MaterialRenderProxy = MaterialProxy;

				FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
				FPrimitiveUniformShaderParametersBuilder Builder;
				BuildUniformShaderParameters(Builder);
				DynamicPrimitiveUniformBuffer.Set(Collector.GetRHICommandList(), Builder);

				BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;
				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = SDPG_World;
				Mesh.bCanApplyViewModeOverrides = false;
				Collector.AddMesh(ViewIndex, Mesh);
			}
		}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				RenderBounds(Collector.GetPDI(ViewIndex), ViewFamily.EngineShowFlags, GetBounds(), IsSelected());
			}
		}
#endif
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
		return Result;
	}

	virtual bool CanBeOccluded() const override
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual uint32 GetMemoryFootprint() const override
	{
		return sizeof(*this) + GetAllocatedSize();
	}

private:

	FDynamicMeshIndexBuffer32 IndexBuffer;
	FStaticMeshVertexBuffers VertexBuffers;
	FLocalVertexFactory VertexFactory;
	UMaterialInterface* Material = nullptr;
	bool bSectionVisible = true;
	FMaterialRelevance MaterialRelevance;
};

//////////////////////////////////////////////////////////////////////////

UCosmicFastMeshComponent::UCosmicFastMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	bCastDynamicShadow = true;
}

void UCosmicFastMeshComponent::CreateMeshSection_LinearColor(
	int32 SectionIndex,
	const TArray<FVector>& Vertices,
	const TArray<int32>& Triangles,
	const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0,
	const TArray<FLinearColor>& VertexColors,
	const TArray<FProcMeshTangent>& Tangents,
	bool bCreateCollision,
	bool bSRGBConversion
)
{
	const int32 NumVerts = Vertices.Num();
	if (NumVerts <= 0 || Triangles.Num() <= 0)
	{
		ClearAllMeshSections();
		return;
	}

	SectionData.Positions.Reset(NumVerts);
	SectionData.Positions.SetNumUninitialized(NumVerts);

	SectionData.LocalBox.Init();

	for (int32 i = 0; i < NumVerts; ++i)
	{
		const FVector3f Pos = (FVector3f)Vertices[i];
		SectionData.Positions[i] = Pos;
		SectionData.LocalBox += (FVector)Pos;
	}

	// Normals
	if (Normals.Num() == NumVerts)
	{
		SectionData.Normals.Reset(NumVerts);
		SectionData.Normals.SetNumUninitialized(NumVerts);
		for (int32 i = 0; i < NumVerts; ++i)
		{
			SectionData.Normals[i] = (FVector3f)Normals[i];
		}
	}
	else
	{
		SectionData.Normals.Reset();
	}

	// UV0
	if (UV0.Num() == NumVerts)
	{
		SectionData.UV0.Reset(NumVerts);
		SectionData.UV0.SetNumUninitialized(NumVerts);
		for (int32 i = 0; i < NumVerts; ++i)
		{
			SectionData.UV0[i] = (FVector2f)UV0[i];
		}
	}
	else
	{
		SectionData.UV0.Reset();
	}

	// Colors
	if (VertexColors.Num() == NumVerts)
	{
		SectionData.Colors.Reset(NumVerts);
		SectionData.Colors.SetNumUninitialized(NumVerts);
		for (int32 i = 0; i < NumVerts; ++i)
		{
			SectionData.Colors[i] = VertexColors[i].ToFColor(bSRGBConversion);
		}
	}
	else
	{
		SectionData.Colors.Reset();
	}

	// Tangents
	if (Tangents.Num() == NumVerts)
	{
		SectionData.Tangents.Reset(NumVerts);
		SectionData.Tangents.SetNumUninitialized(NumVerts);
		for (int32 i = 0; i < NumVerts; ++i)
		{
			SectionData.Tangents[i] = (FVector3f)Tangents[i].TangentX;
		}
	}
	else
	{
		SectionData.Tangents.Reset();
	}

	// Indices
	const int32 NumIndices = (Triangles.Num() / 3) * 3;
	SectionData.Indices.Reset(NumIndices);
	SectionData.Indices.SetNumUninitialized(NumIndices);
	for (int32 i = 0; i < NumIndices; ++i)
	{
		SectionData.Indices[i] = (uint32)Triangles[i];
	}

	bSectionCreated = true;
	bSectionVisible = true;
	LocalBounds = FBoxSphereBounds(SectionData.LocalBox);

	UpdateBounds();
	MarkRenderStateDirty();
}

void UCosmicFastMeshComponent::UpdateMeshSection_LinearColor(
	int32 SectionIndex,
	const TArray<FVector>& Vertices,
	const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0,
	const TArray<FLinearColor>& VertexColors,
	const TArray<FProcMeshTangent>& Tangents,
	bool bSRGBConversion
)
{
	const int32 NumVerts = Vertices.Num();
	if (!bSectionCreated || NumVerts == 0)
	{
		return;
	}

	TSharedPtr<FCosmicFastMeshUpdatePayload> Payload = MakeShared<FCosmicFastMeshUpdatePayload>();
	Payload->Positions.SetNumUninitialized(NumVerts);
	Payload->LocalBox.Init();

	for (int32 i = 0; i < NumVerts; ++i)
	{
		const FVector3f Pos = (FVector3f)Vertices[i];
		Payload->Positions[i] = Pos;
		Payload->LocalBox += (FVector)Pos;
	}

	if (Normals.Num() == NumVerts)
	{
		Payload->Normals.SetNumUninitialized(NumVerts);
		for (int32 i = 0; i < NumVerts; ++i)
		{
			Payload->Normals[i] = (FVector3f)Normals[i];
		}
	}

	if (VertexColors.Num() == NumVerts)
	{
		Payload->Colors.SetNumUninitialized(NumVerts);
		for (int32 i = 0; i < NumVerts; ++i)
		{
			Payload->Colors[i] = VertexColors[i].ToFColor(bSRGBConversion);
		}
	}

	LocalBounds = FBoxSphereBounds(Payload->LocalBox);

	if (SceneProxy && !IsRenderStateDirty())
	{
		FCosmicFastMeshSceneProxy* FastProxy = static_cast<FCosmicFastMeshSceneProxy*>(SceneProxy);
		ENQUEUE_RENDER_COMMAND(FCosmicFastMeshUpdate)(
			[FastProxy, Payload](FRHICommandListImmediate& RHICmdList)
			{
				FastProxy->UpdateSection_RenderThread(RHICmdList, Payload);
			}
		);
	}

	UpdateBounds();
	MarkRenderTransformDirty();
}

void UCosmicFastMeshComponent::UpdateMeshSection_Fast(
	TArray<FVector3f>&& InPositions,
	TArray<FVector3f>&& InNormals,
	TArray<FColor>&& InColors,
	const FBox& InBounds
)
{
	if (!bSectionCreated || InPositions.Num() == 0)
	{
		return;
	}

	TSharedPtr<FCosmicFastMeshUpdatePayload> Payload = MakeShared<FCosmicFastMeshUpdatePayload>();
	Payload->Positions = MoveTemp(InPositions);
	Payload->Normals = MoveTemp(InNormals);
	Payload->Colors = MoveTemp(InColors);
	Payload->LocalBox = InBounds;

	LocalBounds = FBoxSphereBounds(InBounds);

	if (SceneProxy && !IsRenderStateDirty())
	{
		FCosmicFastMeshSceneProxy* FastProxy = static_cast<FCosmicFastMeshSceneProxy*>(SceneProxy);
		ENQUEUE_RENDER_COMMAND(FCosmicFastMeshUpdate)(
			[FastProxy, Payload](FRHICommandListImmediate& RHICmdList)
			{
				FastProxy->UpdateSection_RenderThread(RHICmdList, Payload);
			}
		);
	}

	UpdateBounds();
	MarkRenderTransformDirty();
}

void UCosmicFastMeshComponent::UpdateMeshSection_FastVectors(
	TArray<FVector>&& InPositions,
	TArray<FVector>&& InNormals,
	TArray<FLinearColor>&& InColors,
	bool bSRGBConversion
)
{
	const int32 NumVerts = InPositions.Num();
	if (!bSectionCreated || NumVerts == 0)
	{
		return;
	}

	TSharedPtr<FCosmicFastMeshUpdatePayload> Payload = MakeShared<FCosmicFastMeshUpdatePayload>();
	Payload->Positions.SetNumUninitialized(NumVerts);
	Payload->LocalBox.Init();

	for (int32 i = 0; i < NumVerts; ++i)
	{
		const FVector3f Pos = (FVector3f)InPositions[i];
		Payload->Positions[i] = Pos;
		Payload->LocalBox += (FVector)Pos;
	}

	if (InNormals.Num() == NumVerts)
	{
		Payload->Normals.SetNumUninitialized(NumVerts);
		for (int32 i = 0; i < NumVerts; ++i)
		{
			Payload->Normals[i] = (FVector3f)InNormals[i];
		}
	}

	if (InColors.Num() == NumVerts)
	{
		Payload->Colors.SetNumUninitialized(NumVerts);
		for (int32 i = 0; i < NumVerts; ++i)
		{
			Payload->Colors[i] = InColors[i].ToFColor(bSRGBConversion);
		}
	}

	LocalBounds = FBoxSphereBounds(Payload->LocalBox);

	if (SceneProxy && !IsRenderStateDirty())
	{
		FCosmicFastMeshSceneProxy* FastProxy = static_cast<FCosmicFastMeshSceneProxy*>(SceneProxy);
		ENQUEUE_RENDER_COMMAND(FCosmicFastMeshUpdate)(
			[FastProxy, Payload](FRHICommandListImmediate& RHICmdList)
			{
				FastProxy->UpdateSection_RenderThread(RHICmdList, Payload);
			}
		);
	}

	UpdateBounds();
	MarkRenderTransformDirty();
}

void UCosmicFastMeshComponent::ClearMeshSection(int32 SectionIndex)
{
	SectionData.Positions.Empty();
	SectionData.Normals.Empty();
	SectionData.UV0.Empty();
	SectionData.Colors.Empty();
	SectionData.Tangents.Empty();
	SectionData.Indices.Empty();
	SectionData.LocalBox.Init();

	bSectionCreated = false;
	LocalBounds = FBoxSphereBounds(ForceInit);

	UpdateBounds();
	MarkRenderStateDirty();
}

void UCosmicFastMeshComponent::ClearAllMeshSections()
{
	ClearMeshSection(0);
}

void UCosmicFastMeshComponent::SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility)
{
	if (bSectionVisible != bNewVisibility)
	{
		bSectionVisible = bNewVisibility;
		if (SceneProxy)
		{
			FCosmicFastMeshSceneProxy* FastProxy = static_cast<FCosmicFastMeshSceneProxy*>(SceneProxy);
			ENQUEUE_RENDER_COMMAND(FCosmicFastMeshSetVisibility)(
				[FastProxy, bNewVisibility](FRHICommandListImmediate& RHICmdList)
				{
					FastProxy->SetSectionVisibility_RenderThread(bNewVisibility);
				}
			);
		}
	}
}

bool UCosmicFastMeshComponent::IsMeshSectionVisible(int32 SectionIndex) const
{
	return bSectionVisible;
}

int32 UCosmicFastMeshComponent::GetNumSections() const
{
	return bSectionCreated ? 1 : 0;
}

FPrimitiveSceneProxy* UCosmicFastMeshComponent::CreateSceneProxy()
{
	if (!bSectionCreated || SectionData.Positions.Num() == 0 || SectionData.Indices.Num() == 0)
	{
		return nullptr;
	}
	return new FCosmicFastMeshSceneProxy(this);
}

int32 UCosmicFastMeshComponent::GetNumMaterials() const
{
	return 1;
}

UMaterialInterface* UCosmicFastMeshComponent::GetMaterial(int32 ElementIndex) const
{
	if (ElementIndex == 0)
	{
		return Super::GetMaterial(0);
	}
	return nullptr;
}

FBoxSphereBounds UCosmicFastMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return LocalBounds.TransformBy(LocalToWorld);
}
