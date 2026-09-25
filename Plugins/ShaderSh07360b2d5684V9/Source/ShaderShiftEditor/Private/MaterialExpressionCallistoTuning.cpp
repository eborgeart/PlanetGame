// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "MaterialExpressionCallistoTuning.h"
#include "MaterialCompiler.h"

#define LOCTEXT_NAMESPACE "ShaderShiftCallisto"

UMaterialExpressionCallistoTuning::UMaterialExpressionCallistoTuning(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	// Right-click menu category in the material graph.
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
int32 UMaterialExpressionCallistoTuning::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	// Compile each scalar input, falling back to its Const* default when the
	// pin is not connected.  The three Callisto multipliers are packed
	// verbatim; the shader encode (BasePassPixelShader.usf) / decode
	// (SubstrateEvaluation.ush) applies them on top of the global RhoF /
	// RhoR / Smooth Terminator tunables.
	const int32 RhoF   = RhoFMultiplier.IsConnected()             ? RhoFMultiplier.Compile(Compiler)             : Compiler->Constant(ConstRhoFMultiplier);
	const int32 RhoR   = RhoRMultiplier.IsConnected()             ? RhoRMultiplier.Compile(Compiler)             : Compiler->Constant(ConstRhoRMultiplier);
	const int32 Smooth = SmoothTerminatorMultiplier.IsConnected() ? SmoothTerminatorMultiplier.Compile(Compiler) : Compiler->Constant(ConstSmoothTerminatorMultiplier);
	const int32 Enable = Enabled.IsConnected()                    ? Enabled.Compile(Compiler)                    : Compiler->Constant(ConstEnabled);

	if (RhoF == INDEX_NONE || RhoR == INDEX_NONE || Smooth == INDEX_NONE || Enable == INDEX_NONE)
	{
		return Compiler->Errorf(TEXT("CallistoTuning: failed to compile an input."));
	}

	// Pack into a float4: (RhoF, RhoR, Smooth, Enabled).
	const int32 RG     = Compiler->AppendVector(RhoF, RhoR);     // float2
	const int32 BA     = Compiler->AppendVector(Smooth, Enable); // float2
	const int32 Packed = Compiler->AppendVector(RG, BA);         // float4

	return Compiler->CustomOutput(this, OutputIndex, Packed);
}

void UMaterialExpressionCallistoTuning::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Callisto Per-Material Tuning"));
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
