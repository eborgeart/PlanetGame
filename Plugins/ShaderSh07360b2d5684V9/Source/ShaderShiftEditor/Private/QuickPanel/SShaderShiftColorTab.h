// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.
//
// Quick Panel "Color & Film" tab: Tone Mapping (+AgX Look), Bloom
// (+Preserve Color), and Film Halation & Organic Grain.

#pragma once

#include "QuickPanel/ShaderShiftQuickPanelCommon.h"

class SShaderShiftColorTab : public SShaderShiftPanelTab
{
public:
	SLATE_BEGIN_ARGS(SShaderShiftColorTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void RefreshFromSettings() override;

private:
	bool IsAgxTonemapper() const;

	TArray<FShaderShiftEnumOptionPtr> TonemapOptions;
	TArray<FShaderShiftEnumOptionPtr> AgxLookOptions;
	TArray<FShaderShiftEnumOptionPtr> BloomOptions;

	FShaderShiftEnumOptionPtr CurrentTonemapOption;
	FShaderShiftEnumOptionPtr CurrentAgxLookOption;
	FShaderShiftEnumOptionPtr CurrentBloomOption;

	TSharedPtr<SBox> AgxLookRow;
};
