// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.
//
// Quick Panel "GI & Reflections" tab: Ambient Occlusion, Screen-Space GI
// (SSILVB / Stable SSGI), Lumen ReSTIR GI Probes, Lumen Reflections.

#pragma once

#include "QuickPanel/ShaderShiftQuickPanelCommon.h"

class SShaderShiftGITab : public SShaderShiftPanelTab
{
public:
	SLATE_BEGIN_ARGS(SShaderShiftGITab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void RefreshFromSettings() override;

private:
	TArray<FShaderShiftEnumOptionPtr> AOOptions;
	TArray<FShaderShiftEnumOptionPtr> SSGIOptions;
	TArray<FShaderShiftEnumOptionPtr> SSGIQualityOptions;

	FShaderShiftEnumOptionPtr CurrentAOOption;
	FShaderShiftEnumOptionPtr CurrentSSGIOption;
	FShaderShiftEnumOptionPtr CurrentSSGIQualityOption;
};
