// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "QuickPanel/SShaderShiftColorTab.h"
#include "ShaderShiftSettings.h"

#define LOCTEXT_NAMESPACE "ShaderShiftQuickPanel"

void SShaderShiftColorTab::Construct(const FArguments& InArgs)
{
	Settings = UShaderShiftSettings::Get();
	check(Settings);

	BuildEnumOptions<ECustomTonemapMode>(TonemapOptions);
	BuildEnumOptions<EAgxLook>(AgxLookOptions);
	BuildEnumOptions<ECustomBloomMode>(BloomOptions);
	RefreshFromSettings();

	ChildSlot
	[
		SNew(SVerticalBox)

		// ===================================================
		// Tone Mapping section
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 10, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("ToneMapHeader", "Tone Mapping"),
				LOCTEXT("ToneMapDesc", "PostProcessCombineLUTs.usf"),
				TEXT("/Engine/Private/PostProcessCombineLUTs.usf"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 6, 16, 0)
		[
			MakeComboRow(
				LOCTEXT("TonemapLabel", "Tonemapper"),
				&TonemapOptions, &CurrentTonemapOption,
				[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
				{
					if (NewValue.IsValid() && Settings)
					{
						CurrentTonemapOption = NewValue;
						Settings->TonemapMode = static_cast<ECustomTonemapMode>(NewValue->Value);
					}
				})
		]

		// AgX Look - only visible when an AgX tonemapper is selected
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			SAssignNew(AgxLookRow, SBox)
			.Visibility_Lambda([this]()
			{
				return IsAgxTonemapper()
					? EVisibility::Visible
					: EVisibility::Collapsed;
			})
			[
				MakeComboRow(
					LOCTEXT("AgxLookLabel", "AgX Look"),
					&AgxLookOptions, &CurrentAgxLookOption,
					[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
					{
						if (NewValue.IsValid() && Settings)
						{
							CurrentAgxLookOption = NewValue;
							Settings->AgxLook = static_cast<EAgxLook>(NewValue->Value);
						}
					})
			]
		]

		// ===================================================
		// Bloom section
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 12, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("BloomHeader", "Bloom"),
				LOCTEXT("BloomDesc", "PostProcessBloom.usf"),
				TEXT("/Engine/Private/PostProcessBloom.usf"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 6, 16, 0)
		[
			MakeComboRow(
				LOCTEXT("BloomLabel", "Bloom Mode"),
				&BloomOptions, &CurrentBloomOption,
				[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
				{
					if (NewValue.IsValid() && Settings)
					{
						CurrentBloomOption = NewValue;
						Settings->BloomMode = static_cast<ECustomBloomMode>(NewValue->Value);
					}
				})
		]

		// Preserve-emissive-color checkbox - only meaningful for the
		// non-convolution bloom paths, so collapse it for modes 3/5.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				if (!CurrentBloomOption.IsValid())
				{
					return EVisibility::Visible;
				}
				const uint8 V = CurrentBloomOption->Value;
				const bool bConv =
					V == static_cast<uint8>(ECustomBloomMode::FFTDiffraction)
					|| V == static_cast<uint8>(ECustomBloomMode::SpencerOcular);
				return bConv ? EVisibility::Collapsed : EVisibility::Visible;
			})
			[
				MakeBoolRow(
					LOCTEXT("PreserveColorLabel", "Preserve Color"),
					LOCTEXT("PreserveColorTip",
						"When enabled, the bloom setup preserves the hue of bright "
						"emissive surfaces so their cores keep their colour through "
						"the tonemapper instead of trending to white "
						"(pre-4.17 Unreal bloom behaviour)."),
					[this]() { return Settings && Settings->bPreserveEmissiveColor; },
					[this](bool V) { if (Settings) Settings->bPreserveEmissiveColor = V; })
			]
		]

		// ===================================================
		// Film Halation + Organic Grain section
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 12, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("HalationHeader", "Film Halation & Grain"),
				LOCTEXT("HalationDesc", "PostProcessTonemap.usf"),
				TEXT("/Engine/Private/PostProcessTonemap.usf"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 6, 16, 0)
		[
			MakeBoolRow(
				LOCTEXT("HalationToggleLabel", "Film Halation"),
				LOCTEXT("HalationToggleTip", "Highlight-driven red bleed simulating film halation."),
				[this]() { return Settings && Settings->bFilmHalation; },
				[this](bool V) { if (Settings) Settings->bFilmHalation = V; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 0)
		[
			MakeFloatRow(
				LOCTEXT("HalationStrengthLabel", "Halation Strength"),
				LOCTEXT("HalationStrengthTip",
					"Intensity of the red bleed at saturated highlights.  "
					"Live: drag to preview in real time (no recompile)."),
				0.0f, 1.0f,
				[this]() { return Settings ? Settings->HalationStrength : 0.25f; },
				[this](float V) { if (Settings) Settings->HalationStrength = V; },
				/*LiveParamId*/ 43 /* ShaderShiftLive::Live_HalationStrength (moved off retired ID 1) */)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 0)
		[
			MakeFloatRow(
				LOCTEXT("HalationThresholdLabel", "Halation Threshold"),
				LOCTEXT("HalationThresholdTip",
					"Luminance threshold above which halation begins to bleed."),
				0.0f, 2.0f,
				[this]() { return Settings ? Settings->HalationThreshold : 0.8f; },
				[this](float V) { if (Settings) Settings->HalationThreshold = V; },
					/*LiveParamId*/ 29 /* ShaderShiftLive::Live_HalationThreshold */)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 0)
		[
			MakeBoolRow(
				LOCTEXT("GrainToggleLabel", "Organic Grain"),
				LOCTEXT("GrainToggleTip", "Midtone-weighted chromatic film grain."),
				[this]() { return Settings && Settings->bOrganicGrain; },
				[this](bool V) { if (Settings) Settings->bOrganicGrain = V; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 12)
		[
			MakeFloatRow(
				LOCTEXT("GrainStrengthLabel", "Grain Strength"),
				LOCTEXT("GrainStrengthTip",
					"Per-channel grain amplitude at midtone luminance.  "
					"Live: drag to preview in real time (no recompile)."),
				0.0f, 1.0f,
				[this]() { return Settings ? Settings->GrainStrength : 0.12f; },
				[this](float V) { if (Settings) Settings->GrainStrength = V; },
				/*LiveParamId*/ 2 /* ShaderShiftLive::Live_GrainStrength */)
		]
	];
}

void SShaderShiftColorTab::RefreshFromSettings()
{
	if (!Settings)
	{
		return;
	}
	CurrentTonemapOption = FindOption(TonemapOptions, static_cast<uint8>(Settings->TonemapMode));
	CurrentAgxLookOption = FindOption(AgxLookOptions, static_cast<uint8>(Settings->AgxLook));
	CurrentBloomOption   = FindOption(BloomOptions,   static_cast<uint8>(Settings->BloomMode));
}

bool SShaderShiftColorTab::IsAgxTonemapper() const
{
	if (!CurrentTonemapOption.IsValid()) return false;
	// AgX_Punchy = 1
	const uint8 Val = CurrentTonemapOption->Value;
	return Val == static_cast<uint8>(ECustomTonemapMode::AgX_Punchy);
}

#undef LOCTEXT_NAMESPACE
