// Copyright 2025 Dmitry Karpukhin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"
#include "PostProcess/PostProcessing.h"
#include "PostProcess/PostProcessMaterial.h"
#include "Misc/EngineVersionComparison.h"

#if UE_VERSION_OLDER_THAN(5,6,0)
// Useful macro from UE 5.6
#define UE_VERSION_NEWER_THAN_OR_EQUAL(MajorVersion, MinorVersion, PatchVersion)\
UE_GREATER_SORT(ENGINE_MAJOR_VERSION, MajorVersion, UE_GREATER_SORT(ENGINE_MINOR_VERSION, MinorVersion, UE_GREATER_SORT(ENGINE_PATCH_VERSION, PatchVersion, true)))
#endif

#if UE_VERSION_NEWER_THAN_OR_EQUAL(5,2,0)
#include "DataDrivenShaderPlatformInfo.h"
#endif

class FCMAA2SceneViewExtension : public FSceneViewExtensionBase
{
public:
	FCMAA2SceneViewExtension(const FAutoRegister& AutoRegister);
	
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {};
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override;
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {};

#if UE_VERSION_NEWER_THAN_OR_EQUAL(5,5,0)
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass PassId, const FSceneView& View, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;
#else
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass PassId, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;
#endif

	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override;

private:
	
	FScreenPassTexture RenderCMAA2(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);
	FScreenPassTexture RenderCMAA2DebugEdges(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);
	
	FRDGTextureRef Edges = nullptr;
	bool bGameView = false;
};

// Shader declarations

class FDetectEdgesCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDetectEdgesCS);
	SHADER_USE_PARAMETER_STRUCT(FDetectEdgesCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SAMPLER(SamplerState, g_gather_point_clamp_Sampler)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, g_inoutColorReadonly)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, g_workingEdges)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, g_workingShapeCandidates)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, g_workingDeferredBlendLocationList)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint2>, g_workingDeferredBlendItemList)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, g_workingDeferredBlendItemListHeads)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWByteAddressBuffer, g_workingControlBuffer)
		SHADER_PARAMETER(FVector2f, ViewOffset)
	END_SHADER_PARAMETER_STRUCT()

	class FCMAA2QualityPreset : SHADER_PERMUTATION_INT("CMAA2_STATIC_QUALITY_PRESET", 4);
	class FCMAA2ExtraSharpness : SHADER_PERMUTATION_BOOL("CMAA2_EXTRA_SHARPNESS");

	using FPermutationDomain = TShaderPermutationDomain<FCMAA2QualityPreset, FCMAA2ExtraSharpness>;
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

#if UE_VERSION_NEWER_THAN_OR_EQUAL(5,5,0)
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters,FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters,OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_ForceBindful);
	}
#endif
};

class FComputeDispatchArgsCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FComputeDispatchArgsCS);
	SHADER_USE_PARAMETER_STRUCT(FComputeDispatchArgsCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, g_workingShapeCandidates)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, g_workingDeferredBlendLocationList)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWByteAddressBuffer, g_workingControlBuffer)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWByteAddressBuffer, g_workingExecuteIndirectBuffer)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

#if UE_VERSION_NEWER_THAN_OR_EQUAL(5,5,0)
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters,FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters,OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_ForceBindful);
	}
#endif
};

class FProcessCandidatesCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FProcessCandidatesCS);
	SHADER_USE_PARAMETER_STRUCT(FProcessCandidatesCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SAMPLER(SamplerState, g_gather_point_clamp_Sampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, g_workingEdges)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, g_workingShapeCandidates)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, g_workingDeferredBlendLocationList)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint2>, g_workingDeferredBlendItemList)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, g_workingDeferredBlendItemListHeads)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWByteAddressBuffer, g_workingControlBuffer)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, g_inoutColorReadonly)
		SHADER_PARAMETER(FVector2f, ViewOffset)
		RDG_BUFFER_ACCESS(IndirectArgsBuffer, ERHIAccess::IndirectArgs)
	END_SHADER_PARAMETER_STRUCT()

	class FCMAA2ExtraSharpness : SHADER_PERMUTATION_BOOL("CMAA2_EXTRA_SHARPNESS");
	
	using FPermutationDomain = TShaderPermutationDomain<FCMAA2ExtraSharpness>;

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
    	return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }

#if UE_VERSION_NEWER_THAN_OR_EQUAL(5,5,0)
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters,FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters,OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_ForceBindful);
	}
#endif
};

class FDeferredColorApplyCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDeferredColorApplyCS);
	SHADER_USE_PARAMETER_STRUCT(FDeferredColorApplyCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SceneColorSampler)
		SHADER_PARAMETER(FVector2f, SceneColorTextureSize)
		SHADER_PARAMETER_SAMPLER(SamplerState, g_gather_point_clamp_Sampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, g_inoutColorWriteonly)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, g_workingDeferredBlendLocationList)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint2>, g_workingDeferredBlendItemList)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, g_workingDeferredBlendItemListHeads)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWByteAddressBuffer, g_workingControlBuffer)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, g_inoutColorReadonly)
		SHADER_PARAMETER(FVector2f, ViewOffset)
		RDG_BUFFER_ACCESS(IndirectArgsBuffer, ERHIAccess::IndirectArgs)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

#if UE_VERSION_NEWER_THAN_OR_EQUAL(5,5,0)
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters,FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters,OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_ForceBindful);
	}
#endif
};

class FDebugShowEdgesCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDebugShowEdgesCS);
	SHADER_USE_PARAMETER_STRUCT(FDebugShowEdgesCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, g_inoutColorWriteonly)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<uint>, g_workingEdges)
		SHADER_PARAMETER(FVector2f, ViewOffset)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

#if UE_VERSION_NEWER_THAN_OR_EQUAL(5,5,0)
	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters,FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters,OutEnvironment);
		OutEnvironment.CompilerFlags.Add(CFLAG_ForceBindful);
	}
#endif
};