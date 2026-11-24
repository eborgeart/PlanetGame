// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "QuickPanel/SShaderShiftQuickPanel.h"

#include "QuickPanel/SShaderShiftColorTab.h"
#include "QuickPanel/SShaderShiftShadingTab.h"
#include "QuickPanel/SShaderShiftGITab.h"
#include "QuickPanel/SShaderShiftFogTab.h"
#include "QuickPanel/SShaderShiftAATab.h"
#include "QuickPanel/SShaderShiftSystemTab.h"

#include "ShaderShift.h"
#include "ShaderShiftEditor.h"
#include "ShaderShiftSettings.h"

#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/SlateTypes.h"

#define LOCTEXT_NAMESPACE "ShaderShiftQuickPanel"

DEFINE_LOG_CATEGORY_STATIC(LogShaderShiftQuickPanel, Log, All);

void SShaderShiftQuickPanel::Construct(const FArguments& InArgs)
{
	Settings = UShaderShiftSettings::Get();
	check(Settings);

	const FLinearColor AccentColor    = FShaderShiftPanelStyle::AccentColor;
	const FLinearColor DimTextColor   = FShaderShiftPanelStyle::DimTextColor;
	const FLinearColor BrightTextColor = FShaderShiftPanelStyle::BrightTextColor;

	// Build the category tabs.  Order here == tab-bar order == switcher slots.
	Tabs.Empty();
	Tabs.Add(SNew(SShaderShiftColorTab));
	Tabs.Add(SNew(SShaderShiftShadingTab));
	Tabs.Add(SNew(SShaderShiftGITab));
	Tabs.Add(SNew(SShaderShiftFogTab));
	Tabs.Add(SNew(SShaderShiftAATab));
	Tabs.Add(SNew(SShaderShiftSystemTab));

	TabSwitcher = SNew(SWidgetSwitcher);
	for (const TSharedRef<SShaderShiftPanelTab>& Tab : Tabs)
	{
		TabSwitcher->AddSlot()
		[
			// Each tab scrolls independently so no category can push the
			// Apply / Restore footer off-screen.
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				Tab
			]
		];
	}
	TabSwitcher->SetActiveWidgetIndex(ActiveTabIndex);

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(FShaderShiftPanelStyle::PanelWidth)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			.Padding(0)
			.ColorAndOpacity(FLinearColor::White)
			[
				SNew(SVerticalBox)

				// ===================================================
				// Header bar
				// ===================================================
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(FMargin(16.f, 10.f))
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(LOCTEXT("PanelTitle", "ShaderShift"))
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
								.ColorAndOpacity(BrightTextColor)
							]

							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0, 2, 0, 0)
							[
								SNew(STextBlock)
								.Text(LOCTEXT("PanelSubtitle",
									"Engine shader overrides"))
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
								.ColorAndOpacity(DimTextColor)
							]
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							// Status badge
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
							.Padding(FMargin(8.f, 4.f))
							[
								SNew(STextBlock)
								.Text_Lambda([]()
								{
									return ShaderShiftQuickPanel::IsAnyHookActive()
										? LOCTEXT("StatusActive", "ACTIVE")
										: LOCTEXT("StatusInactive", "INACTIVE");
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
								.ColorAndOpacity_Lambda([AccentColor, DimTextColor]()
								{
									return ShaderShiftQuickPanel::IsAnyHookActive()
										? FSlateColor(AccentColor)
										: FSlateColor(DimTextColor);
								})
							]
						]
					]
				]

				// ===================================================
				// Category tab bar
				// ===================================================
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(FMargin(8.f, 4.f))
					[
						MakeTabBar()
					]
				]

				// ===================================================
				// Active category content (scrollable)
				// ===================================================
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				[
					TabSwitcher.ToSharedRef()
				]

				// ===================================================
				// Live status strip -- persistent replacement for the old
				// tab-switch warning dialog.  Shows how many knobs are
				// live-previewing unbaked values; Revert discards them.
				// ===================================================
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(16, 8, 16, 0)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
							.Text_Lambda([]()
							{
								const int32 N = ShaderShiftQuickPanel::GetLiveDirtyCount();
								return N > 0
									? FText::Format(LOCTEXT("LiveStripDirty",
										"LIVE PREVIEW · {0} unbaked {0}|plural(one=change,other=changes)"), N)
									: LOCTEXT("LiveStripClean", "All changes baked");
							})
							.ToolTipText(LOCTEXT("LiveStripTip",
								"Dragged sliders preview instantly in the viewport (no recompile) and stay "
								"live across tabs.  Bake writes them into the compile-time defines in one "
								"recompile; Revert discards them.  Mode/feature dropdowns still need a Bake "
								"before their sliders can preview.  Live preview pauses automatically in "
								"debug viewmodes (Lighting Only etc.) and never touches scene captures."))
							.ColorAndOpacity_Lambda([AccentColor, DimTextColor]()
							{
								return ShaderShiftQuickPanel::GetLiveDirtyCount() > 0
									? FSlateColor(AccentColor)
									: FSlateColor(DimTextColor);
							})
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(SButton)
							.ContentPadding(FMargin(8, 2))
							.Visibility_Lambda([]()
							{
								return ShaderShiftQuickPanel::GetLiveDirtyCount() > 0
									? EVisibility::Visible
									: EVisibility::Collapsed;
							})
							.Text(LOCTEXT("RevertLiveBtn", "Revert"))
							.ToolTipText(LOCTEXT("RevertLiveTip",
								"Discard the live preview: every slider returns to its last baked value. "
								"No recompile."))
							.OnClicked(this, &SShaderShiftQuickPanel::OnRevertLiveClicked)
						]
					]

					// Secondary line: which diffuse/spec mode groups currently own
					// the shared live BRDF bank lanes.  Re-evaluates every frame via
					// Text_Lambda, the same binding style as the dirty count above.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 2, 0, 0)
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
						.Text_Lambda([]()
						{
							return ShaderShiftQuickPanel::GetLiveBankOwnersText();
						})
						.ToolTipText(LOCTEXT("LiveBankOwnersTip",
							"Mode-exclusive BRDF tweakables share a fixed set of live transport "
							"lanes, so one diffuse mode and one spec mode own the banks at a "
							"time: the ones listed here (your current Diffuse / Specular BRDF "
							"selections).  The listed modes' sliders preview together, batch-"
							"live.  Other modes' sliders are live only while dragged (focus "
							"lane) and show their baked values otherwise."))
						.ColorAndOpacity(DimTextColor)
					]
				]

				// ===================================================
				// Buttons
				// ===================================================
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(16, 10, 16, 14)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.Padding(0, 0, 4, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ContentPadding(FMargin(0, 6))
						.Text_Lambda([]()
						{
							const int32 N = ShaderShiftQuickPanel::GetLiveDirtyCount();
							return N > 0
								? FText::Format(LOCTEXT("BakeBtn", "Bake {0} {0}|plural(one=Change,other=Changes)"), N)
								: LOCTEXT("ApplyBtn", "Apply");
						})
						.ToolTipText(LOCTEXT("ApplyTip",
							"Write patched shaders and recompile.  Bakes every live-previewed "
							"slider into its compile-time define in the same pass."))
						.OnClicked(this, &SShaderShiftQuickPanel::OnApplyClicked)
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.Padding(4, 0, 0, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.ContentPadding(FMargin(0, 6))
						.Text(LOCTEXT("RestoreBtn", "Restore Engine Defaults"))
						.ToolTipText(LOCTEXT("RestoreTip",
							"Revert all engine shaders to originals"))
						.OnClicked(this, &SShaderShiftQuickPanel::OnRestoreClicked)
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> SShaderShiftQuickPanel::MakeTabBar()
{
	TSharedRef<SHorizontalBox> Bar = SNew(SHorizontalBox);

	struct FTabDesc { FText Label; FText ToolTip; };
	const FTabDesc Descs[] =
	{
		{ LOCTEXT("TabColor",   "Color"),    LOCTEXT("TabColorTip",   "Tone mapping, bloom, film halation & grain") },
		{ LOCTEXT("TabShading", "Shading"),  LOCTEXT("TabShadingTip", "Diffuse / specular BRDF overrides") },
		{ LOCTEXT("TabGI",      "GI"),       LOCTEXT("TabGITip",      "Ambient occlusion, screen-space GI, Lumen GI & reflections") },
		{ LOCTEXT("TabFog",     "Fog"),      LOCTEXT("TabFogTip",     "Volumetric fog phase, self-shadow, multi-scatter") },
		{ LOCTEXT("TabAA",      "AA"),       LOCTEXT("TabAATip",      "Temporal AA / TSR tweaks and VSM shadows") },
		{ LOCTEXT("TabDeploy",  "Deploy"),   LOCTEXT("TabDeployTip",  "Parameter binding (compile-time vs runtime) and packaging / persistence") },
	};

	for (int32 i = 0; i < UE_ARRAY_COUNT(Descs); ++i)
	{
		Bar->AddSlot()
		.FillWidth(1.f)
		.Padding(2, 0)
		[
			MakeTabButton(i, Descs[i].Label, Descs[i].ToolTip)
		];
	}

	return Bar;
}

TSharedRef<SWidget> SShaderShiftQuickPanel::MakeTabButton(
	int32 TabIndex, const FText& Label, const FText& ToolTip)
{
	// Toggle-style buttons matching the panel's dark look: the active tab
	// keeps its accent chip + bold face, and inactive tabs now read as
	// clickable chips instead of bare labels.
	//
	// The app's "ToggleButtonCheckbox" style already draws the ACTIVE tab
	// as a rounded chip (its Checked* rounded-box images) but gives the
	// unchecked state no background at all.  Extend that same style -- same
	// brush type, same 4px corner radius -- so INACTIVE tabs get a subtle
	// rounded chip from the panel's own palette (ButtonBg at rest, nudged
	// brighter on hover).  Text colour rides the style's per-state
	// foreground (SCheckBox::GetForegroundColor): dim at rest, bright on
	// hover, accent when active -- the same colours the old per-widget
	// lambdas produced, plus the hover brightening.
	static const FCheckBoxStyle TabChipStyle = []()
	{
		const FLinearColor ChipBg = FShaderShiftPanelStyle::ButtonBgColor;
		const FLinearColor ChipBgHover(   // ButtonBg, slightly brighter
			ChipBg.R * 1.5f, ChipBg.G * 1.5f, ChipBg.B * 1.5f, ChipBg.A);

		FCheckBoxStyle Style = FAppStyle::Get()
			.GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckbox");
		Style.SetUncheckedImage       (FSlateRoundedBoxBrush(ChipBg,      4.0f));
		Style.SetUncheckedHoveredImage(FSlateRoundedBoxBrush(ChipBgHover, 4.0f));
		Style.SetUncheckedPressedImage(FSlateRoundedBoxBrush(ChipBgHover, 4.0f));
		Style.SetForegroundColor(FShaderShiftPanelStyle::DimTextColor);
		Style.SetHoveredForegroundColor(FShaderShiftPanelStyle::BrightTextColor);
		Style.SetPressedForegroundColor(FShaderShiftPanelStyle::BrightTextColor);
		Style.SetCheckedForegroundColor(FShaderShiftPanelStyle::AccentColor);
		Style.SetCheckedHoveredForegroundColor(FShaderShiftPanelStyle::AccentColor);
		Style.SetCheckedPressedForegroundColor(FShaderShiftPanelStyle::AccentColor);
		return Style;
	}();

	return SNew(SCheckBox)
		.Style(&TabChipStyle)
		.ToolTipText(ToolTip)
		.IsChecked_Lambda([this, TabIndex]()
		{
			return ActiveTabIndex == TabIndex
				? ECheckBoxState::Checked
				: ECheckBoxState::Unchecked;
		})
		.OnCheckStateChanged_Lambda([this, TabIndex](ECheckBoxState NewState)
		{
			if (NewState == ECheckBoxState::Checked && TabSwitcher.IsValid() && TabIndex != ActiveTabIndex)
			{
				// Live previews ride per-knob slot lanes now, so they persist
				// across tabs -- switching is always free, no confirmation
				// needed.  The status strip above the buttons keeps showing
				// the unbaked count until the user Bakes or Reverts.
				ActiveTabIndex = TabIndex;
				TabSwitcher->SetActiveWidgetIndex(TabIndex);
			}
		})
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.Padding(FMargin(2.f, 3.f))
			[
				SNew(STextBlock)
				.Text(Label)
				.Font_Lambda([this, TabIndex]()
				{
					return ActiveTabIndex == TabIndex
						? FCoreStyle::GetDefaultFontStyle("Bold", 9)
						: FCoreStyle::GetDefaultFontStyle("Regular", 9);
				})
				// Inherit the chip style's per-state foreground: dim at rest,
				// bright on hover, accent when this tab is active.
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
		];
}

FReply SShaderShiftQuickPanel::OnApplyClicked()
{
	if (Settings)
	{
		Settings->SyncEnumsToDefines();
		Settings->SaveConfig();

		// Apply is an explicit "make my changes live" action, so it ALWAYS
		// recompiles: bIsManualApply=true bypasses the bAutoRecompile setting
		// (a leftover from the abandoned live-update feature, which would otherwise
		// block the Apply button whenever the user has auto-recompile off).  Global
		// vs. 'recompileshaders changed' is still decided by each hook's
		// RequiresGlobalRecompile config flag inside ApplyAndRecompileIfNeeded
		// (BRDF/shading -> global, tonemapper/bloom -> changed).
		FShaderShiftEditorModule& EditorModule =
			FModuleManager::GetModuleChecked<FShaderShiftEditorModule>(
				TEXT("ShaderShiftEditor"));
		EditorModule.ApplyAndRecompileIfNeeded(/*bIsManualApply=*/true);

		// Everything just previewed is now baked into the compile-time
		// defines: re-seed the snapshot (dirty count -> 0) and park the lanes.
		ShaderShiftQuickPanel::ReseedBakedSnapshot();
	}
	return FReply::Handled();
}

FReply SShaderShiftQuickPanel::OnRevertLiveClicked()
{
	// Discard the live preview: restore every knob's Settings value from the
	// baked snapshot and park the lanes -- the viewport snaps back to the last
	// baked look with no recompile.
	ShaderShiftQuickPanel::RevertLiveKnobs();

	for (const TSharedRef<SShaderShiftPanelTab>& Tab : Tabs)
	{
		Tab->RefreshFromSettings();
	}
	return FReply::Handled();
}

FReply SShaderShiftQuickPanel::OnRestoreClicked()
{
	if (Settings)
	{
		// Reset every user-facing setting to its engine-default value, then
		// let each tab re-point its combo selections.  Without this reset
		// the dropdowns kept showing whatever the user had selected, and
		// the next PostEditChangeProperty would re-apply the old values
		// onto the engine shaders we just reverted - which made the
		// Restore button look like it did nothing.
		ShaderShiftQuickPanel::ResetSettingsToEngineDefaults(Settings);

		// Generic define entries -- e.g. the per-material BRDF toggles on the
		// Shading tab -- live directly in ShaderHooks[].Defines[].Value and
		// have no typed UPROPERTY for the reset above to touch.  Restore each
		// one to its plugin-ini DefaultValue too, otherwise the
		// ForceRecompile() below would immediately re-patch the shaders with
		// the stale values.  For defines that ARE mapped from typed properties
		// this is a harmless no-op: ForceRecompile's SyncEnumsToDefines
		// overwrites them from the just-reset properties anyway.
		for (FShaderShiftHookConfig& Hook : Settings->ShaderHooks)
		{
			for (FShaderShiftDefineEntry& Def : Hook.Defines)
			{
				if (!Def.DefaultValue.IsEmpty())
				{
					Def.Value = Def.DefaultValue;
				}
			}
		}

		for (const TSharedRef<SShaderShiftPanelTab>& Tab : Tabs)
		{
			Tab->RefreshFromSettings();
		}

		// Persist the reset values so the runtime module's
		// EarlyApplySavedSettings reads them on next editor startup
		// and doesn't re-patch the engine shaders with stale values.
		Settings->SaveConfig();
	}

	// Now revert the patched shader files on disk back to the
	// engine originals.
	FShaderShiftHookRegistry& Registry = FShaderShiftHookRegistry::Get();
	const int32 Reverted = Registry.RevertAllHooks();

	UE_LOG(LogShaderShiftQuickPanel, Log,
		TEXT("Quick Panel - Restored %d hook(s) to engine defaults (settings reset)"),
		Reverted);

	FShaderShiftEditorModule& EditorModule =
		FModuleManager::GetModuleChecked<FShaderShiftEditorModule>(
			TEXT("ShaderShiftEditor"));
	EditorModule.ForceRecompile();

	// Restore reverts every hook + setting: the just-reset Settings values ARE
	// the baked state now, so re-seed the snapshot (parks focus + slot lanes).
	ShaderShiftQuickPanel::ReseedBakedSnapshot();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
