// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.
//
// Quick Panel "Deploy" tab: packaging / persistence options.

#pragma once

#include "QuickPanel/ShaderShiftQuickPanelCommon.h"

class SShaderShiftSystemTab : public SShaderShiftPanelTab
{
public:
	SLATE_BEGIN_ARGS(SShaderShiftSystemTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void RefreshFromSettings() override;
};
