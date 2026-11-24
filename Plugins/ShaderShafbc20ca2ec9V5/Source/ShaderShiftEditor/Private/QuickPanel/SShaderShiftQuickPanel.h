// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.
//
// ============================================================================
// SShaderShiftQuickPanel - toolbar popup window (shell)
//
// A polished dark panel with status indicators, dropdowns for the main
// shader settings, and Apply / Restore buttons.  The settings rows live in
// self-contained per-category tab widgets under QuickPanel/; this shell owns:
//   - the header bar (title + ACTIVE/INACTIVE badge)
//   - the category tab bar
//   - a scrollable switcher hosting the active tab's content
//   - the Apply / Restore Engine Defaults footer
// ============================================================================

#pragma once

#include "QuickPanel/ShaderShiftQuickPanelCommon.h"

class SWidgetSwitcher;

class SShaderShiftQuickPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SShaderShiftQuickPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> MakeTabBar();
	TSharedRef<SWidget> MakeTabButton(int32 TabIndex, const FText& Label, const FText& ToolTip);

	FReply OnApplyClicked();
	FReply OnRestoreClicked();
	FReply OnRevertLiveClicked();

	UShaderShiftSettings* Settings = nullptr;

	/** All category tabs, in tab-bar order.  Index == switcher slot. */
	TArray<TSharedRef<SShaderShiftPanelTab>> Tabs;
	TSharedPtr<SWidgetSwitcher> TabSwitcher;
	int32 ActiveTabIndex = 0;
};
