// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MaterialExpressionIO.h"
#include "MaterialValueType.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "MaterialExpressionCallistoTuning.generated.h"

/**
 * Callisto Per-Material Tuning custom output.
 *
 * Emits a single packed float4 custom output `CallistoTuning0(Parameters)`:
 *   .x = RhoF multiplier,  .y = RhoR multiplier,
 *   .z = Smooth Terminator multiplier,  .w = Enabled (0/1)
 * and defines the compile macro NUM_MATERIAL_OUTPUTS_CALLISTOTUNING.
 *
 * The patched BasePassPixelShader.usf reads this (gated on the macro and the
 * global CALLISTO_PER_MATERIAL_TUNING toggle) and ENCODES the lanes into the
 * Substrate slab's F0 / Anisotropy / Fuzz channels; the patched
 * SubstrateEvaluation.ush decodes them back per pixel and the Callisto BRDF
 * scales its global RhoF / RhoR / Smooth Terminator tunables by them
 * (1 = global default).
 * For Regolith/Proxima materials prefer the dedicated Regolith/Proxima
 * Per-Material Tuning nodes.
 *
 * IMPORTANT (matches LSC UX): this is a *custom output* — DO NOT wire its
 * output into any material pin.  Adding the node and setting its inputs is
 * sufficient.  When Enabled = 0 (or disconnected default), the material is
 * left untouched.  Using this node repurposes the material's Specular,
 * Anisotropy and Fuzz channels (their normal behaviour is lost); do not use
 * it on materials that need those channels.  RhoR / Smooth Terminator
 * additionally require an anisotropy-capable permutation
 * (MATERIAL_USES_ANISOTROPY) and the Substrate Complex path
 * (sg.ShadingQuality 2).
 */
UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionCallistoTuning : public UMaterialExpressionCustomOutput
{
	GENERATED_UCLASS_BODY()

	/** 1 = apply this node's tuning. 0 / disconnected = material unchanged. */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "1 = apply tuning, 0 = material unchanged. Defaults to ConstEnabled."))
	FExpressionInput Enabled;

	/** Diffuse-Fresnel (RhoF) multiplier. 1 = global default. */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Diffuse-Fresnel (RhoF) multiplier for this material. 1 = global default."))
	FExpressionInput RhoFMultiplier;

	/** Retroreflection (RhoR) multiplier. 1 = global default. */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Retroreflection (RhoR) multiplier for this material. 1 = global default. Needs an anisotropy-capable permutation (MATERIAL_USES_ANISOTROPY) and the Substrate Complex path."))
	FExpressionInput RhoRMultiplier;

	/** Smooth Terminator multiplier. 1 = global default. */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Smooth Terminator multiplier for this material. 1 = global default. Needs an anisotropy-capable permutation (MATERIAL_USES_ANISOTROPY) and the Substrate Complex path."))
	FExpressionInput SmoothTerminatorMultiplier;

	/** Used when Enabled is not connected. */
	UPROPERTY(EditAnywhere, Category = "Callisto", meta = (OverridingInputProperty = "Enabled", UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f))
	float ConstEnabled = 1.0f;

	/** Used when RhoFMultiplier is not connected. */
	UPROPERTY(EditAnywhere, Category = "Callisto", meta = (OverridingInputProperty = "RhoFMultiplier", UIMin = 0.0f, UIMax = 4.0f, ClampMin = 0.0f, ClampMax = 4.0f))
	float ConstRhoFMultiplier = 1.0f;

	/** Used when RhoRMultiplier is not connected. */
	UPROPERTY(EditAnywhere, Category = "Callisto", meta = (OverridingInputProperty = "RhoRMultiplier", UIMin = 0.0f, UIMax = 2.0f, ClampMin = 0.0f, ClampMax = 2.0f))
	float ConstRhoRMultiplier = 1.0f;

	/** Used when SmoothTerminatorMultiplier is not connected. */
	UPROPERTY(EditAnywhere, Category = "Callisto", meta = (OverridingInputProperty = "SmoothTerminatorMultiplier", UIMin = 0.0f, UIMax = 2.0f, ClampMin = 0.0f, ClampMax = 2.0f))
	float ConstSmoothTerminatorMultiplier = 1.0f;

#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif

	virtual int32 GetNumOutputs() const override { return 1; }
	virtual FString GetFunctionName() const override { return TEXT("CallistoTuning"); }
	virtual FString GetDisplayName() const override { return TEXT("Callisto Per-Material Tuning"); }
};
