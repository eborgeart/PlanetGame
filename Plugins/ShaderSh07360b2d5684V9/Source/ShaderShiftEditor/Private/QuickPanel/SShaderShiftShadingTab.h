// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.
//
// Quick Panel "Shading" tab: Diffuse / Specular BRDF overrides.

#pragma once

#include "QuickPanel/ShaderShiftQuickPanelCommon.h"

class SShaderShiftShadingTab : public SShaderShiftPanelTab
{
public:
	SLATE_BEGIN_ARGS(SShaderShiftShadingTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void RefreshFromSettings() override;

private:
	/** "Fix Up Tagged Materials": wires a 0.0001 constant into the
	    Anisotropy pin of every loaded tagged material that still misses
	    one, forcing the complex Substrate slab permutation that the
	    per-material transport rides.  Undoable; result shown as a toast. */
	FReply OnFixUpTaggedMaterialsClicked();

	TArray<FShaderShiftEnumOptionPtr> DiffuseOptions;
	TArray<FShaderShiftEnumOptionPtr> SpecularOptions;
	TArray<FShaderShiftEnumOptionPtr> CallistoDiffPresetOptions;
	TArray<FShaderShiftEnumOptionPtr> CallistoSpecPresetOptions;
	TArray<FShaderShiftEnumOptionPtr> MultiLobePresetOptions;

	FShaderShiftEnumOptionPtr CurrentDiffuseOption;
	FShaderShiftEnumOptionPtr CurrentSpecularOption;
	FShaderShiftEnumOptionPtr CurrentCallistoDiffPresetOption;
	FShaderShiftEnumOptionPtr CurrentCallistoSpecPresetOption;
	FShaderShiftEnumOptionPtr CurrentMultiLobePresetOption;
};
