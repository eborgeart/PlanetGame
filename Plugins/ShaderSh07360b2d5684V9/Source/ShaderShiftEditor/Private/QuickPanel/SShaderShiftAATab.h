// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.
//
// Quick Panel "AA & Shadows" tab: Temporal AA / TSR ghosting + sharpen
// tweaks, and contact-aware VSM penumbra hardening.

#pragma once

#include "QuickPanel/ShaderShiftQuickPanelCommon.h"

class SShaderShiftAATab : public SShaderShiftPanelTab
{
public:
	SLATE_BEGIN_ARGS(SShaderShiftAATab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void RefreshFromSettings() override;
};
