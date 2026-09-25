// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MaterialExpressionIO.h"
#include "MaterialValueType.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "MaterialExpressionRegolithTuning.generated.h"

/**
 * Regolith Per-Material Tuning custom output.
 *
 * Per-material tuning for materials whose effective BRDF preset is Regolith
 * (a ShaderShift BRDF Select node set to Regolith, or the global Regolith
 * micrograin dust-layer mode).  Emits a single packed float4 custom output
 * `RegolithTuning0(Parameters)`:
 *   .x = Dust Coverage,  .y = Opposition Glow,  .z = Dust Sheen,  .w = Enabled (0/1)
 * (the Ignore Roughness Gate flag rides the glow lane's sign: .y is negated)
 * and defines the compile macro NUM_MATERIAL_OUTPUTS_REGOLITHTUNING.
 *
 * The patched BasePassPixelShader.usf reads this (gated on the macro and the
 * global CALLISTO_PER_MATERIAL_TUNING toggle) and ENCODES the lanes into the
 * Substrate slab's F0 / Anisotropy / Fuzz channels; the patched
 * SubstrateEvaluation.ush decodes them back per pixel.  All tuning wrapper
 * nodes (Callisto / Regolith / Proxima) share the same packed float4 lane
 * convention; use at most one tuning node per material.  Grain roughness is
 * global-only by design and has no per-material lane.
 *
 * IMPORTANT (matches LSC UX): this is a *custom output* — DO NOT wire its
 * output into any material pin.  Adding the node and setting its inputs is
 * sufficient.  When Enabled = 0 (or disconnected default), the material is
 * left untouched.  Using this node repurposes the material's Specular,
 * Anisotropy and Fuzz channels (their normal behaviour is lost); do not use
 * it on materials that need those channels.  Opposition Glow and Dust Sheen
 * additionally require an anisotropy-capable permutation
 * (MATERIAL_USES_ANISOTROPY) and the Substrate Complex path
 * (sg.ShadingQuality 2).
 */
UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionRegolithTuning : public UMaterialExpressionCustomOutput
{
	GENERATED_UCLASS_BODY()

	/** 1 = apply this node's tuning. 0 / disconnected = material unchanged. */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "1 = apply tuning, 0 = material unchanged. Defaults to ConstEnabled."))
	FExpressionInput Enabled;

	/** Absolute dust coverage [0-1] for this material. 1 = fully dusted; 0 = engine-exact/no dust. */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Absolute dust coverage 0-1 for this material. 1 = fully dusted (also the unwired default); 0 = engine-exact/no dust."))
	FExpressionInput DustCoverage;

	/** Opposition surge strength [0-2] (absolute). */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Opposition surge strength 0-2 (absolute). Needs an anisotropy-capable permutation (MATERIAL_USES_ANISOTROPY) and the Substrate Complex path."))
	FExpressionInput OppositionGlow;

	/** Grain sheen [0-2] (absolute). */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Grain sheen 0-2 (absolute). Needs an anisotropy-capable permutation (MATERIAL_USES_ANISOTROPY) and the Substrate Complex path."))
	FExpressionInput DustSheen;

	/** Used when Enabled is not connected. */
	UPROPERTY(EditAnywhere, Category = "Regolith", meta = (OverridingInputProperty = "Enabled", UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f))
	float ConstEnabled = 1.0f;

	/** Used when DustCoverage is not connected. 1 = fully dusted; 0 = engine-exact/no dust. */
	UPROPERTY(EditAnywhere, Category = "Regolith", meta = (OverridingInputProperty = "DustCoverage", UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f))
	float ConstDustCoverage = 1.0f;

	/** Used when OppositionGlow is not connected. */
	UPROPERTY(EditAnywhere, Category = "Regolith", meta = (OverridingInputProperty = "OppositionGlow", UIMin = 0.0f, UIMax = 2.0f, ClampMin = 0.0f, ClampMax = 2.0f))
	float ConstOppositionGlow = 1.0f;

	/** Used when DustSheen is not connected. */
	UPROPERTY(EditAnywhere, Category = "Regolith", meta = (OverridingInputProperty = "DustSheen", UIMin = 0.0f, UIMax = 2.0f, ClampMin = 0.0f, ClampMax = 2.0f))
	float ConstDustSheen = 1.0f;

	/** Apply Dust Coverage flat on this material, ignoring the roughness gate.  Rides the glow lane's sign bit. */
	UPROPERTY(EditAnywhere, Category = "ShaderShift", meta = (DisplayName = "Ignore Roughness Gate", ToolTip = "Apply Dust Coverage flat, ignoring the roughness-based accumulation gate (and the global Dust Roughness Affinity) on this material."))
	bool bIgnoreRoughnessGate = false;

#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif

	virtual int32 GetNumOutputs() const override { return 1; }
	virtual FString GetFunctionName() const override { return TEXT("RegolithTuning"); }
	virtual FString GetDisplayName() const override { return TEXT("Regolith Per-Material Tuning"); }
};
