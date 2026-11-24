// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "MaterialExpressionProximaTuning.h"
#include "MaterialCompiler.h"

#define LOCTEXT_NAMESPACE "ShaderShiftProxima"

UMaterialExpressionProximaTuning::UMaterialExpressionProximaTuning(const FObjectInitializer& ObjectInitializer)
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
int32 UMaterialExpressionProximaTuning::Compile(class FMaterialCompiler* Compiler, int32 OutputIndex)
{
	// Compile each scalar input, falling back to its Const* default when the
	// pin is not connected.  The output is packed in the shared tuning-node
	// float4 convention (Lane A, Lane B, Lane C, Enabled) that the
	// preset-aware encode in BasePassPixelShader.usf expects; the Proxima
	// preset uses lane A only (absolute diffuse-AA alpha scale).
	const int32 Alpha  = AlphaScale.IsConnected() ? AlphaScale.Compile(Compiler) : Compiler->Constant(ConstAlphaScale);
	const int32 Enable = Enabled.IsConnected()    ? Enabled.Compile(Compiler)    : Compiler->Constant(ConstEnabled);

	if (Alpha == INDEX_NONE || Enable == INDEX_NONE)
	{
		return Compiler->Errorf(TEXT("ProximaTuning: failed to compile an input."));
	}

	// Pack into a float4: (AlphaScale, 0, 0, Enabled).  Lanes B / C are unused
	// by the Proxima preset and never written by the encode.
	const int32 Zero   = Compiler->Constant(0.0f);
	const int32 RG     = Compiler->AppendVector(Alpha, Zero); // float2
	const int32 BA     = Compiler->AppendVector(Zero, Enable); // float2
	const int32 Packed = Compiler->AppendVector(RG, BA);       // float4

	return Compiler->CustomOutput(this, OutputIndex, Packed);
}

void UMaterialExpressionProximaTuning::GetCaption(TArray<FString>& OutCaptions) const
{
	OutCaptions.Add(TEXT("Proxima Per-Material Tuning"));
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
