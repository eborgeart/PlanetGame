// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "MaterialExpressionRegolithTuning.h"
#include "MaterialCompiler.h"

#define LOCTEXT_NAMESPACE "ShaderShiftRegolith"

UMaterialExpressionRegolithTuning::UMaterialExpressionRegolithTuning(const FObjectInitializer& ObjectInitializer)
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
int32 UMaterialExpressionRegolithTuning::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	// Compile each scalar input, falling back to its Const* default when the
	// pin is not connected.  The lanes are packed in the shared tuning-node
	// float4 convention (Lane A, Lane B, Lane C, Enabled) that the
	// preset-aware encode in BasePassPixelShader.usf expects; for the Regolith
	// preset they mean (Dust Coverage, Opposition Glow, Dust Sheen).
	const int32 Coverage = DustCoverage.IsConnected()   ? DustCoverage.Compile(Compiler)   : Compiler->Constant(ConstDustCoverage);
	const int32 Glow     = OppositionGlow.IsConnected() ? OppositionGlow.Compile(Compiler) : Compiler->Constant(ConstOppositionGlow);
	const int32 Sheen    = DustSheen.IsConnected()      ? DustSheen.Compile(Compiler)      : Compiler->Constant(ConstDustSheen);
	const int32 Enable   = Enabled.IsConnected()        ? Enabled.Compile(Compiler)        : Compiler->Constant(ConstEnabled);

	if (Coverage == INDEX_NONE || Glow == INDEX_NONE || Sheen == INDEX_NONE || Enable == INDEX_NONE)
	{
		return Compiler->Errorf(TEXT("RegolithTuning: failed to compile an input."));
	}

	// The glow lane's SIGN carries the per-material "Ignore Roughness Gate"
	// flag: a negative lane tells the BasePass encode to store a negative
	// aniso-channel magnitude, which the SubstrateEvaluation decode reads as
	// "apply Dust Coverage flat" (forces the roughness affinity to 0).
	const int32 SignedGlow = Compiler->Mul(Glow, Compiler->Constant(bIgnoreRoughnessGate ? -1.0f : 1.0f));

	// Pack into a float4: (Coverage, Glow, Sheen, Enabled).
	const int32 RG     = Compiler->AppendVector(Coverage, SignedGlow); // float2
	const int32 BA     = Compiler->AppendVector(Sheen, Enable);  // float2
	const int32 Packed = Compiler->AppendVector(RG, BA);         // float4

	return Compiler->CustomOutput(this, OutputIndex, Packed);
}

void UMaterialExpressionRegolithTuning::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Regolith Per-Material Tuning"));
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
