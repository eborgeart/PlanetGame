// Copyright 2025 Dmitry Karpukhin. All Rights Reserved.

#include "CMAA2SceneViewExtension.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ScenePrivate.h"
#include "ShaderParameterUtils.h"

IMPLEMENT_GLOBAL_SHADER(FDetectEdgesCS, "/Plugin/CMAA2/CMAA2.usf", "EdgesColor2x2CS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FComputeDispatchArgsCS, "/Plugin/CMAA2/CMAA2.usf", "ComputeDispatchArgsCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FProcessCandidatesCS, "/Plugin/CMAA2/CMAA2.usf", "ProcessCandidatesCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDeferredColorApplyCS, "/Plugin/CMAA2/CMAA2.usf", "DeferredColorApply2x2CS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDebugShowEdgesCS, "/Plugin/CMAA2/CMAA2.usf", "DebugDrawEdgesCS", SF_Compute);

namespace
{
	TAutoConsoleVariable<bool> CVarCMAA2(
	TEXT("r.CMAA2"),
	1,
	TEXT("Enable Conservative Morphological Anti-Aliasing 2 (CMAA2)."),
	ECVF_RenderThreadSafe | ECVF_Scalability);

	TAutoConsoleVariable<int32> CVarCMAA2_Quality(
	TEXT("r.CMAA2.Quality"),
	2,
	TEXT("Quality Preset:\n")
	TEXT("0 = Low\n")
	TEXT("1 = Medium\n")
	TEXT("2 = High\n")
	TEXT("3 = Ultra"),
	ECVF_RenderThreadSafe | ECVF_Scalability);

	TAutoConsoleVariable<bool> CVarCMAA2_ExtraSharpness(
	TEXT("r.CMAA2.ExtraSharpness"),
	0,
	TEXT("Enable 'Extra Sharpness' mode."),
	ECVF_RenderThreadSafe | ECVF_Scalability);

	TAutoConsoleVariable<bool> CVarCMAA2_ShowEdges(
	TEXT("r.CMAA2.ShowEdges"),
	0,
	TEXT("Debug view of the Edge Process pass (Editor only).\n"),
	ECVF_RenderThreadSafe);
}

DECLARE_GPU_STAT_NAMED(CMAA2, TEXT("CMAA2"));

FCMAA2SceneViewExtension::FCMAA2SceneViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister)
{
	UE_LOG(LogTemp, Log, TEXT("SceneViewExtension: CMAA2 is registered"));
}

void FCMAA2SceneViewExtension::SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
{
	bGameView = InView.Family->EngineShowFlags.Game;
}

#if UE_VERSION_NEWER_THAN_OR_EQUAL(5,5,0)
void FCMAA2SceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass PassId, const FSceneView& View, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
#else
void FCMAA2SceneViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass PassId, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
#endif
{
	// Subscribing after SSRInput step since it is very preferred to render it before Motion Blur
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5,1,0)
	if (PassId == EPostProcessingPass::SSRInput)
#else
	if (PassId == EPostProcessingPass::MotionBlur)
#endif
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FCMAA2SceneViewExtension::RenderCMAA2));
	}
	// Debug Edges view mode is better to render separately after tonemapping, so it doesn't interfere with Exposure values
	if (PassId == EPostProcessingPass::Tonemap && CVarCMAA2_ShowEdges.GetValueOnRenderThread() && !bGameView)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(this, &FCMAA2SceneViewExtension::RenderCMAA2DebugEdges));
	}
}

bool FCMAA2SceneViewExtension::IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const
{
	return CVarCMAA2.GetValueOnAnyThread();
}

FScreenPassTexture FCMAA2SceneViewExtension::RenderCMAA2(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5,4,0)
	const FScreenPassTexture& SceneColor = FScreenPassTexture::CopyFromSlice(GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
#else
	const FScreenPassTexture& SceneColor = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);
#endif
	
	if (!SceneColor.IsValid() || !CVarCMAA2.GetValueOnRenderThread() || View.AntiAliasingMethod == AAM_TemporalAA)
	{
		return SceneColor;
	}

	const FSceneViewFamily& ViewFamily = *View.Family;
	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(ViewFamily.GetFeatureLevel());

	const FScreenPassTextureViewport SceneColorTextureViewport(SceneColor);
	
	RDG_EVENT_SCOPE(GraphBuilder, "CMAA2 %dx%d", SceneColorTextureViewport.Rect.Width(), SceneColorTextureViewport.Rect.Height());
	RDG_GPU_STAT_SCOPE(GraphBuilder, CMAA2);

	// Render Target view size that may be bigger than the visible view size (mostly in the Editor)
	const FIntPoint ViewExtent = SceneColorTextureViewport.Extent;
	// Render Target current view size
	const FIntPoint ViewSize = SceneColorTextureViewport.Rect.Size();
	// Render Target current view offset (for Splitscreen/VR and such)
	const FIntPoint ViewOffset = SceneColorTextureViewport.Rect.Min;
	
	const FIntPoint WorkingViewSize = ViewSize;
	
	const FIntPoint HalfWidthSize(FMath::DivideAndRoundUp(WorkingViewSize.X, 2), WorkingViewSize.Y);
	const FIntPoint TileSize(FMath::DivideAndRoundUp(WorkingViewSize.X, 16), FMath::DivideAndRoundUp(WorkingViewSize.Y, 16));
	const uint32 NumPixels = WorkingViewSize.X * WorkingViewSize.Y;
	const uint32 NumTiles = TileSize.X * TileSize.Y;
	const uint32 ControlBytes = sizeof(uint32) * WorkingViewSize.X * WorkingViewSize.Y;

	FRDGTextureDesc OutputDesc = SceneColor.Texture->Desc;
	OutputDesc.Flags |= TexCreate_UAV;
	OutputDesc.Flags &= ~(TexCreate_RenderTargetable | TexCreate_FastVRAM);
	OutputDesc.ClearValue = FClearValueBinding(FLinearColor::Black);

	// Creating Buffers and Textures
	Edges = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(HalfWidthSize, PF_R8_UINT, FClearValueBinding::Black, TexCreate_UAV | TexCreate_ShaderResource),TEXT("CMAA2.Edges"), ERDGTextureFlags::MultiFrame);
	FRDGBufferRef ShapeCandidates = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), NumPixels / 4),TEXT("CMAA2.ShapeCandidates"));
	FRDGBufferRef DeferredBlendLocationList = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), NumPixels),TEXT("CMAA2.DeferredBlendLocationList"));
	FRDGBufferRef DeferredBlendItemList = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32) * 2, NumTiles * 80),TEXT("CMAA2.DeferredBlendItemList"));
	FRDGTextureRef DeferredBlendItemListHeads = GraphBuilder.CreateTexture(FRDGTextureDesc::Create2D(WorkingViewSize / 2, PF_R32_UINT, FClearValueBinding::Black, TexCreate_UAV | TexCreate_ShaderResource),TEXT("CMAA2.DeferredBlendItemListHeads"));
	FRDGBufferRef ControlBuffer = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateByteAddressDesc(ControlBytes),TEXT("CMAA2.ControlBuffer"));
	FRDGTextureRef Output = GraphBuilder.CreateTexture(OutputDesc,TEXT("CMAA2.Output"));
	FRDGBufferRef IndirectArgs1Write = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateByteAddressDesc(sizeof(uint32) * 3),TEXT("CMAA2.IndirectArgs1.Write"));
	FRDGBufferRef IndirectArgs1Read = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateIndirectDesc(sizeof(uint32) * 3), TEXT("CMAA2.IndirectArgs1.Read"));
	FRDGBufferRef IndirectArgs2Write = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateByteAddressDesc(sizeof(uint32) * 3),TEXT("CMAA2.IndirectArgs2.Write"));
	FRDGBufferRef IndirectArgs2Read = GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateIndirectDesc(sizeof(uint32) * 3), TEXT("CMAA2.IndirectArgs2.Read"));

	// Creating UAVs
	FRDGTextureUAVRef EdgesUAV = GraphBuilder.CreateUAV(Edges);
	FRDGBufferUAVRef ShapeCandidatesUAV = GraphBuilder.CreateUAV(ShapeCandidates);
	FRDGBufferUAVRef DeferredBlendLocationListUAV = GraphBuilder.CreateUAV(DeferredBlendLocationList);
	FRDGBufferUAVRef DeferredBlendItemListUAV = GraphBuilder.CreateUAV(DeferredBlendItemList);
	FRDGTextureUAVRef DeferredBlendItemListHeadsUAV = GraphBuilder.CreateUAV(DeferredBlendItemListHeads);
	FRDGBufferUAVRef ControlBufferUAV = GraphBuilder.CreateUAV(ControlBuffer);
	FRDGTextureUAVRef OutputUAV = GraphBuilder.CreateUAV(Output);
	FRDGBufferUAVRef IndirectArgs1WriteUAV = GraphBuilder.CreateUAV(IndirectArgs1Write);
	FRDGBufferUAVRef IndirectArgs2WriteUAV = GraphBuilder.CreateUAV(IndirectArgs2Write);

	// Clear Buffers
	//AddClearUAVPass(GraphBuilder, EdgesUAV, 0xFFFFFFFF);
	//AddClearUAVPass(GraphBuilder, ShapeCandidatesUAV,  0u);
	//AddClearUAVPass(GraphBuilder, DeferredBlendLocationListUAV,  0u);
	//AddClearUAVPass(GraphBuilder, DeferredBlendItemListUAV, 0u);
	//AddClearUAVPass(GraphBuilder, DeferredBlendItemListHeadsUAV, 0xFFFFFFFF);
	AddClearUAVPass(GraphBuilder, ControlBufferUAV, 0u);
	//AddClearUAVPass(GraphBuilder, IndirectArgs1WriteUAV, 0u);
	//AddClearUAVPass(GraphBuilder, IndirectArgs2WriteUAV, 0u);
	//AddClearUAVPass(GraphBuilder, OutputUAV, 0xFFFFFFFF);

	int32 QualityPreset = FMath::Clamp(CVarCMAA2_Quality.GetValueOnRenderThread(), 0, 3);
	bool ExtraSharpness = CVarCMAA2_ExtraSharpness.GetValueOnRenderThread();

	// Main Shader Passes
	{
		FDetectEdgesCS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FDetectEdgesCS::FCMAA2QualityPreset>(QualityPreset);
		PermutationVector.Set<FDetectEdgesCS::FCMAA2ExtraSharpness>(ExtraSharpness);

		TShaderMapRef<FDetectEdgesCS>DetectEdgesCS (GlobalShaderMap, PermutationVector);
		auto* PassParameters = GraphBuilder.AllocParameters<FDetectEdgesCS::FParameters>();
		
		PassParameters->g_gather_point_clamp_Sampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->g_inoutColorReadonly = SceneColor.Texture;
		PassParameters->g_workingEdges = EdgesUAV;
		PassParameters->g_workingShapeCandidates = ShapeCandidatesUAV;
		PassParameters->g_workingDeferredBlendLocationList = DeferredBlendLocationListUAV;
		PassParameters->g_workingDeferredBlendItemList = DeferredBlendItemListUAV;
		PassParameters->g_workingDeferredBlendItemListHeads = DeferredBlendItemListHeadsUAV;
		PassParameters->g_workingControlBuffer = ControlBufferUAV;
		PassParameters->ViewOffset = ViewOffset;

		const FIntPoint ThreadsPerGroup(14, 14);
		const FIntVector Groups((HalfWidthSize.X + ThreadsPerGroup.X - 1) / ThreadsPerGroup.X,(HalfWidthSize.Y + ThreadsPerGroup.Y - 1) / ThreadsPerGroup.Y, 1);

		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("DetectEdges"), ERDGPassFlags::NeverCull | ERDGPassFlags::Compute, DetectEdgesCS, PassParameters, Groups);
	}
	
	{
		TShaderMapRef<FComputeDispatchArgsCS>ComputeDispatchArgsCS (GlobalShaderMap);
		auto* PassParameters = GraphBuilder.AllocParameters<FComputeDispatchArgsCS::FParameters>();
		
		PassParameters->g_workingShapeCandidates = ShapeCandidatesUAV;
		PassParameters->g_workingDeferredBlendLocationList = DeferredBlendLocationListUAV;
		PassParameters->g_workingControlBuffer = ControlBufferUAV;
		PassParameters->g_workingExecuteIndirectBuffer = IndirectArgs1WriteUAV;
		
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ComputeDispatchArgs1CS"), ComputeDispatchArgsCS, PassParameters, FIntVector(2,1,1));
		//AddCopyBufferPass(GraphBuilder, IndirectArgs1Read, 0, IndirectArgs1Write, 0, sizeof(uint32) * 3);
		AddCopyBufferPass(GraphBuilder, IndirectArgs1Read, IndirectArgs1Write);
	}

	{
		FProcessCandidatesCS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FProcessCandidatesCS::FCMAA2ExtraSharpness>(ExtraSharpness);

		TShaderMapRef<FProcessCandidatesCS>ProcessCandidatesCS (GlobalShaderMap, PermutationVector);
		auto* PassParameters = GraphBuilder.AllocParameters<FProcessCandidatesCS::FParameters>();
		
		PassParameters->g_gather_point_clamp_Sampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->g_workingEdges = EdgesUAV;
		PassParameters->g_workingShapeCandidates = ShapeCandidatesUAV;
		PassParameters->g_workingDeferredBlendLocationList = DeferredBlendLocationListUAV;
		PassParameters->g_workingDeferredBlendItemList = DeferredBlendItemListUAV;
		PassParameters->g_workingDeferredBlendItemListHeads = DeferredBlendItemListHeadsUAV;
		PassParameters->g_workingControlBuffer = ControlBufferUAV;
		PassParameters->g_inoutColorReadonly = SceneColor.Texture;
		PassParameters->ViewOffset = ViewOffset;
		PassParameters->IndirectArgsBuffer = IndirectArgs1Read;
		
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ProcessCandidates"), ProcessCandidatesCS, PassParameters, IndirectArgs1Read, 0);
	}

	{
		TShaderMapRef<FComputeDispatchArgsCS>ComputeDispatchArgsCS (GlobalShaderMap);
		auto* PassParameters = GraphBuilder.AllocParameters<FComputeDispatchArgsCS::FParameters>();
		
		PassParameters->g_workingControlBuffer = ControlBufferUAV;
		PassParameters->g_workingShapeCandidates = ShapeCandidatesUAV;
		PassParameters->g_workingDeferredBlendLocationList = DeferredBlendLocationListUAV;
		PassParameters->g_workingExecuteIndirectBuffer = IndirectArgs2WriteUAV;
		
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("ComputeDispatchArgs2CS"), ComputeDispatchArgsCS, PassParameters, FIntVector(1,2,1));
		//AddCopyBufferPass(GraphBuilder, IndirectArgs2Read, 0, IndirectArgs2Write, 0, sizeof(uint32) * 3);
		AddCopyBufferPass(GraphBuilder, IndirectArgs2Read, IndirectArgs2Write);
	}

	// Copying SceneColor (read-only) to OutTexture (write-only)
	AddCopyTexturePass(GraphBuilder, SceneColor.Texture, Output);
	
	{
		TShaderMapRef<FDeferredColorApplyCS>DeferredColorApplyCS (GlobalShaderMap);
		
		auto* PassParameters = GraphBuilder.AllocParameters<FDeferredColorApplyCS::FParameters>();
		PassParameters->g_gather_point_clamp_Sampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		PassParameters->g_inoutColorWriteonly = OutputUAV;
		PassParameters->g_workingDeferredBlendLocationList = DeferredBlendLocationListUAV;
		PassParameters->g_workingDeferredBlendItemList = DeferredBlendItemListUAV;
		PassParameters->g_workingDeferredBlendItemListHeads = DeferredBlendItemListHeadsUAV;
		PassParameters->g_workingControlBuffer = ControlBufferUAV;
		PassParameters->g_inoutColorReadonly = SceneColor.Texture;
		PassParameters->ViewOffset = ViewOffset;
		PassParameters->IndirectArgsBuffer = IndirectArgs2Read;
		
		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("DeferredColorApply"), DeferredColorApplyCS, PassParameters, IndirectArgs2Read, 0);
	}
	
	// Having two texture copy passes adds to the cost, but it seems unavoidable
	AddCopyTexturePass(GraphBuilder, Output, SceneColor.Texture);

	return SceneColor;
}

FScreenPassTexture FCMAA2SceneViewExtension::RenderCMAA2DebugEdges(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs)
{
#if UE_VERSION_NEWER_THAN_OR_EQUAL(5,4,0)
	const FScreenPassTexture& SceneColor = FScreenPassTexture::CopyFromSlice(GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
#else
	const FScreenPassTexture& SceneColor = Inputs.GetInput(EPostProcessMaterialInput::SceneColor);
#endif
	
	if (!SceneColor.IsValid() || !CVarCMAA2_ShowEdges.GetValueOnRenderThread() || !Edges)
	{
		return SceneColor;
	}

	const FScreenPassTextureViewport SceneColorTextureViewport(SceneColor);
	
	RDG_EVENT_SCOPE(GraphBuilder, "CMAA2 Debug Edges %dx%d", SceneColorTextureViewport.Rect.Width(), SceneColorTextureViewport.Rect.Height());
	RDG_GPU_STAT_SCOPE(GraphBuilder, CMAA2);

	const FSceneViewFamily& ViewFamily = *View.Family;
	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(ViewFamily.GetFeatureLevel());

	// Render Target current view size
	const FIntPoint ViewSize = SceneColorTextureViewport.Rect.Size();
	// Render Target current view offset (for Splitscreen/VR and such)
	const FIntPoint ViewOffset = SceneColorTextureViewport.Rect.Min;

	FRDGTextureDesc OutputDesc = SceneColor.Texture->Desc;
	OutputDesc.Flags |= TexCreate_UAV;
	OutputDesc.Flags &= ~(TexCreate_RenderTargetable | TexCreate_FastVRAM);
	OutputDesc.ClearValue = FClearValueBinding(FLinearColor::Black);

	FRDGTextureRef Output = GraphBuilder.CreateTexture(OutputDesc,TEXT("CMAA2.Output"));
	
	{
		TShaderMapRef<FDebugShowEdgesCS>DebugShowEdges (GlobalShaderMap);
		auto* PassParameters = GraphBuilder.AllocParameters<FDebugShowEdgesCS::FParameters>();

		PassParameters->g_inoutColorWriteonly = GraphBuilder.CreateUAV(Output);
		PassParameters->g_workingEdges = GraphBuilder.CreateUAV(Edges);
		PassParameters->ViewOffset = ViewOffset;

		const FIntVector Groups(FMath::DivideAndRoundUp(ViewSize.X, 16),FMath::DivideAndRoundUp(ViewSize.Y, 16),1);

		FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("DebugShowEdges"), DebugShowEdges, PassParameters, Groups);
	}
	
	return FScreenPassTexture(Output, SceneColor.ViewRect);
}