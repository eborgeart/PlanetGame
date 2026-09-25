// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "QuickPanel/SShaderShiftSystemTab.h"
#include "ShaderShiftEditor.h"
#include "ShaderShiftSettings.h"
#include "ShaderShift.h"

#define LOCTEXT_NAMESPACE "ShaderShiftQuickPanel"

void SShaderShiftSystemTab::Construct(const FArguments& InArgs)
{
	Settings = UShaderShiftSettings::Get();
	check(Settings);

	ChildSlot
	[
		SNew(SVerticalBox)

		// ===================================================
		// Packaging section
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 10, 16, 0)
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
					.Text(LOCTEXT("PackagingHeader", "Packaging"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PackagingDesc",
						"Keep patched shaders on disk across shutdown"))
					.Font(FCoreStyle::GetDefaultFontStyle("Italic", 7))
					.ColorAndOpacity(FShaderShiftPanelStyle::DimTextColor)
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 6, 16, 12)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(FShaderShiftPanelStyle::LabelWidth)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PersistLabel", "Persist Changes"))
					.ToolTipText(LOCTEXT("PersistTip",
						"When enabled, ShaderShift does NOT revert the patched "
						"engine shaders on editor shutdown, crash, or pre-exit, "
						"and skips its stale-backup recovery on next startup. "
						"Use this when packaging the game.\n\n"
						"Cook commandlets auto-persist regardless of this setting. "
						"While this is on, the engine shader files in your install "
						"remain modified - turn it off and click Restore Engine "
						"Defaults before uninstalling the plugin."))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FShaderShiftPanelStyle::DimTextColor)
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]()
				{
					return Settings && Settings->bPersistShaderChangesInEngine
						? ECheckBoxState::Checked
						: ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					if (Settings)
					{
						const bool bNewValue =
							(NewState == ECheckBoxState::Checked);
						Settings->bPersistShaderChangesInEngine = bNewValue;

						// Mirror into the runtime module so the
						// shutdown/crash handlers honor the new
						// value without waiting for a settings-panel
						// PostEditChangeProperty round-trip.
						FShaderShiftModule::SetPersistShaderChanges(bNewValue);

						Settings->SaveConfig();

						// On OFF->ON transition (now that the runtime
						// flag is updated), check out the engine
						// shader files for source-engine builds.
						if (bNewValue)
						{
							FShaderShiftEditorModule::CheckOutPatchedShadersForPersist();
						}
					}
				})
			]
		]
	];
}

void SShaderShiftSystemTab::RefreshFromSettings()
{
	// Checkbox reads the settings object live; nothing to re-point.
}

#undef LOCTEXT_NAMESPACE
