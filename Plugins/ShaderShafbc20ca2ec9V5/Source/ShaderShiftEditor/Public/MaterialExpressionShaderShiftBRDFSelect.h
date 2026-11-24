// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "MaterialExpressionShaderShiftBRDFSelect.generated.h"

/** Per-material BRDF preset. Maps to effective diffuse/specular modes in BRDF.ush. */
UENUM()
enum class EShaderShiftBRDFPreset : uint8
{
	None           UMETA(DisplayName = "None (use global BRDF)"),
	EngineDefault  UMETA(DisplayName = "Engine Default (Lambert + GGX)"),
	Callisto       UMETA(DisplayName = "Callisto (Diffuse Fresnel/Retro/Smooth + Dual GGX)"),
	Regolith       UMETA(DisplayName = "Regolith (Micrograin Dust Layer)"),
	Burley         UMETA(DisplayName = "Burley"),
	LambertSphere  UMETA(DisplayName = "Lambert-Sphere"),
	Gotanda        UMETA(DisplayName = "Gotanda"),
	Proxima        UMETA(DisplayName = "Proxima (diffuse AA)"),
};

/**
 * ShaderShift BRDF Select custom output (ShaderShift).
 *
 * Tags a material with a per-material BRDF preset that overrides the global
 * Diffuse/Specular BRDF mode for that material only.  Emits a single scalar
 * custom output `ShaderShiftBRDFSelect0(Parameters)` (the preset index 0..7)
 * and defines the compile macro NUM_MATERIAL_OUTPUTS_SHADERSHIFTBRDFSELECT.
 *
 * The patched BasePassPixelShader.usf encodes the preset into the reserved
 * Substrate BSDF state bits (gated on the project's "Per-Material BRDF
 * Selection" toggle, CALLISTO_PER_MATERIAL_SELECT); the patched
 * SubstrateEvaluation.ush decodes it and the BRDF runtime-switches on it.
 *
 * Requirements / limitations (same as per-material tuning): the reserved state
 * bits only survive the Substrate Complex path — set sg.ShadingQuality 2, and
 * the material may need a complexity-triggering input.  Preset "None" is a
 * no-op (the material keeps the global BRDF).  Do NOT wire this node's output
 * into any material pin — adding the node and setting the preset is sufficient.
 */
UCLASS(MinimalAPI, collapsecategories, hidecategories = Object)
class UMaterialExpressionShaderShiftBRDFSelect : public UMaterialExpressionCustomOutput
{
	GENERATED_UCLASS_BODY()

	/** Which BRDF this material uses, overriding the global mode.
	    Used only when the PresetInput pin below is unconnected. */
	UPROPERTY(EditAnywhere, Category = "ShaderShift", meta = (ToolTip = "Per-material BRDF preset. 'None' keeps the global BRDF. Ignored when the Preset input pin is connected."))
	EShaderShiftBRDFPreset Preset = EShaderShiftBRDFPreset::Callisto;

	/** Optional scalar preset input (0..7, rounded/clamped at encode).  Wire a
	    Scalar Parameter here to make the preset overridable per MATERIAL
	    INSTANCE (and settable at runtime via SetScalarParameterValue) -- node
	    properties can never be instanced, pins fed by parameters can.  The
	    compiled permutation (complex Substrate path) is shared by all
	    instances, so switching preset per instance needs no recompile.
	    0=None 1=EngineDefault 2=Callisto 3=Regolith 4=Burley 5=LambertSphere
	    6=Gotanda 7=Proxima. */
	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Optional: scalar preset 0-7. Overrides the Preset dropdown; wire a Scalar Parameter for per-instance selection."))
	FExpressionInput PresetInput;

#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif

	virtual int32 GetNumOutputs() const override { return 1; }
	virtual FString GetFunctionName() const override { return TEXT("ShaderShiftBRDFSelect"); }
	virtual FString GetDisplayName() const override { return TEXT("ShaderShift BRDF Select"); }
};
