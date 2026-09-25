// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MaterialExpressionIO.h"
#include "MaterialValueType.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "MaterialExpressionProximaTuning.generated.h"

/**
 * Proxima Per-Material Tuning custom output.
 *
 * Per-material tuning for materials whose effective BRDF preset is Proxima
 * (a ShaderShift BRDF Select node set to Proxima, or the global Proxima
 * diffuse-AA mode).  Emits a single packed float4 custom output
 * `ProximaTuning0(Parameters)`:
 *   .x = Alpha Scale,  .y = 0,  .z = 0,  .w = Enabled (0/1)
 * and defines the compile macro NUM_MATERIAL_OUTPUTS_PROXIMATUNING.
 *
 * The patched BasePassPixelShader.usf reads this (gated on the macro and the
 * global CALLISTO_PER_MATERIAL_TUNING toggle) and ENCODES the alpha scale
 * into the Substrate slab's F0 (Specular) channel; the patched
 * SubstrateEvaluation.ush decodes it back per pixel.  All tuning wrapper
 * nodes (Callisto / Regolith / Proxima) share the same packed float4 lane
 * convention; use at most one tuning node per material.  Proxima uses lane A
 * only — the Anisotropy / Fuzz channels are never hijacked.
 *
 * IMPORTANT (matches LSC UX): this is a *custom output* — DO NOT wire its
 * output into any material pin.  Adding the node and setting its inputs is
 * sufficient.  When Enabled = 0 (or disconnected default), the material is
 * left untouched.  Using this node repurposes the material's Specular
 * channel (its normal behaviour is lost); do not use it on materials that
 * need it.
 */
UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionProximaTuning : public UMaterialExpressionCustomOutput
{
	GENERATED_UCLASS_BODY()

	/** 1 = apply this node's tuning. 0 / disconnected = material unchanged. */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "1 = apply tuning, 0 = material unchanged. Defaults to ConstEnabled."))
	FExpressionInput Enabled;

	/** Absolute Proxima diffuse-AA alpha scale [0-4]. */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Absolute Proxima diffuse-AA alpha scale 0-4."))
	FExpressionInput AlphaScale;

	/** Used when Enabled is not connected. */
	UPROPERTY(EditAnywhere, Category = "Proxima", meta = (OverridingInputProperty = "Enabled", UIMin = 0.0f, UIMax = 1.0f, ClampMin = 0.0f, ClampMax = 1.0f))
	float ConstEnabled = 1.0f;

	/** Used when AlphaScale is not connected. */
	UPROPERTY(EditAnywhere, Category = "Proxima", meta = (OverridingInputProperty = "AlphaScale", UIMin = 0.0f, UIMax = 4.0f, ClampMin = 0.0f, ClampMax = 4.0f))
	float ConstAlphaScale = 1.0f;

#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif

	virtual int32 GetNumOutputs() const override { return 1; }
	virtual FString GetFunctionName() const override { return TEXT("ProximaTuning"); }
	virtual FString GetDisplayName() const override { return TEXT("Proxima Per-Material Tuning"); }
};
