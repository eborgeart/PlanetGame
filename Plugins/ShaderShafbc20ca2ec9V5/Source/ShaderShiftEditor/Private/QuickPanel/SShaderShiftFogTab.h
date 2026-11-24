// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.
//
// Quick Panel "Fog" tab: Volumetric Fog phase function, self-shadowing,
// multi-scatter, spectral tint, powder glow, and R2 jitter.

#pragma once

#include "QuickPanel/ShaderShiftQuickPanelCommon.h"

class SShaderShiftFogTab : public SShaderShiftPanelTab
{
public:
	SLATE_BEGIN_ARGS(SShaderShiftFogTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void RefreshFromSettings() override;

private:
	TArray<FShaderShiftEnumOptionPtr> VolPhaseOptions;
	TArray<FShaderShiftEnumOptionPtr> VolSelfShadowOptions;
	TArray<FShaderShiftEnumOptionPtr> VolMultiScatterOptions;

	FShaderShiftEnumOptionPtr CurrentVolPhaseOption;
	FShaderShiftEnumOptionPtr CurrentVolSelfShadowOption;
	FShaderShiftEnumOptionPtr CurrentVolMultiScatterOption;
};
