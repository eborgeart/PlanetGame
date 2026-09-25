// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "MaterialExpressionShaderShiftBRDFSelect.h"
#include "MaterialCompiler.h"

#define LOCTEXT_NAMESPACE "ShaderShiftBRDF"

UMaterialExpressionShaderShiftBRDFSelect::UMaterialExpressionShaderShiftBRDFSelect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	struct FConstructorStatics
	{
		FText NAME_ShaderShift;
		FConstructorStatics()
			: NAME_ShaderShift(LOCTEXT("ShaderShift", "ShaderShift"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;
	MenuCategories.Add(ConstructorStatics.NAME_ShaderShift);
#endif
}

#if WITH_EDITOR
int32 UMaterialExpressionShaderShiftBRDFSelect::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	// Emit the preset index (0..7) as a scalar custom output.  The patched
	// BasePassPixelShader.usf rounds/clamps it and packs it into the reserved
	// Substrate BSDF state bits.  When the optional PresetInput pin is
	// connected (typically to a Scalar Parameter) it wins over the dropdown --
	// parameters evaluate per-instance at runtime, so Material Instances (and
	// SetScalarParameterValue) can switch the BRDF without any recompile.
	const int32 PresetCode = PresetInput.GetTracedInput().Expression
		? PresetInput.Compile(Compiler)
		: Compiler->Constant(static_cast<float>(static_cast<uint8>(Preset)));
	return Compiler->CustomOutput(this, OutputIndex, PresetCode);
}

void UMaterialExpressionShaderShiftBRDFSelect::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("ShaderShift BRDF Select"));
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
