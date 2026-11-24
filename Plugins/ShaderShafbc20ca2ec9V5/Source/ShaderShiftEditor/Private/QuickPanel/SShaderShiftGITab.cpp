// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "QuickPanel/SShaderShiftGITab.h"
#include "ShaderShiftSettings.h"

#define LOCTEXT_NAMESPACE "ShaderShiftQuickPanel"

void SShaderShiftGITab::Construct(const FArguments& InArgs)
{
	Settings = UShaderShiftSettings::Get();
	check(Settings);

	BuildEnumOptions<ECustomAOMode>(AOOptions);
	BuildEnumOptions<ECustomSSGIMode>(SSGIOptions);
	BuildEnumOptions<ECustomSSGIQuality>(SSGIQualityOptions);
	RefreshFromSettings();

	ChildSlot
	[
		SNew(SVerticalBox)

		// ===================================================
		// Ambient Occlusion section
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 10, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("AOHeader", "Ambient Occlusion"),
				LOCTEXT("AODesc", "PostProcessAmbientOcclusion.usf"),
				TEXT("/Engine/Private/PostProcessAmbientOcclusion.usf"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 6, 16, 0)
		[
			MakeComboRow(
				LOCTEXT("AOLabel", "AO Method"),
				&AOOptions, &CurrentAOOption,
				[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
				{
					if (NewValue.IsValid() && Settings)
					{
						CurrentAOOption = NewValue;
						Settings->AOMode = static_cast<ECustomAOMode>(NewValue->Value);
					}
				})
		]

		// ===================================================
		// Screen-Space GI section
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 12, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("SSGIHeader", "Screen-Space GI"),
				LOCTEXT("SSGIDesc", "SSRTDiffuseIndirect.usf -- needs r.SSGI.Enable=1, Lumen GI off"),
				TEXT("/Engine/Private/SSRT/SSRTDiffuseIndirect.usf"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 6, 16, 0)
		[
			MakeComboRow(
				LOCTEXT("SSGILabel", "SSGI Method"),
				&SSGIOptions, &CurrentSSGIOption,
				[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
				{
					if (NewValue.IsValid() && Settings)
					{
						CurrentSSGIOption = NewValue;
						Settings->SSGIMode = static_cast<ECustomSSGIMode>(NewValue->Value);
					}
				})
		]

		// Sample multiplier -- visible for Stable SSGI (extra rays per lane)
		// AND SSILVB (extra hemisphere slices per lane).
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				if (!CurrentSSGIOption.IsValid()) return EVisibility::Collapsed;
				return (CurrentSSGIOption->Value == static_cast<uint8>(ECustomSSGIMode::StableSSGI) ||
				        CurrentSSGIOption->Value == static_cast<uint8>(ECustomSSGIMode::SSILVB))
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				MakeComboRow(
					LOCTEXT("SSGIQualityLabel", "Quality"),
					&SSGIQualityOptions, &CurrentSSGIQualityOption,
					[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
					{
						if (NewValue.IsValid() && Settings)
						{
							CurrentSSGIQualityOption = NewValue;
							Settings->SSGIQuality = static_cast<ECustomSSGIQuality>(NewValue->Value);
						}
					})
			]
		]

		// SSILVB intensity -- visible only when SSILVB is selected.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				if (!CurrentSSGIOption.IsValid()) return EVisibility::Collapsed;
				return CurrentSSGIOption->Value == static_cast<uint8>(ECustomSSGIMode::SSILVB)
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				MakeFloatRow(
					LOCTEXT("SSILVBIntensityLabel", "Intensity"),
					LOCTEXT("SSILVBIntensityTip",
						"SSILVB GI brightness multiplier.  The estimator is "
						"pi/2-normalised in-shader, so 1.0 = engine-parity "
						"brightness (a uniform surround reports the same value "
						"as stock SSGI).  The 2.0 default compensates for the "
						"emitter-cosine weighting stock SSGI lacks.  Tune ~1-4; "
						"very high values risk a multi-bounce feedback loop."),
					0.0f, 20.0f,
					[this]() { return Settings ? Settings->SSILVBIntensity : 2.0f; },
					[this](float V) { if (Settings) Settings->SSILVBIntensity = V; },
						/*LiveParamId*/ 30 /* ShaderShiftLive::Live_SSILVBIntensity */)
			]
		]

		// ===================================================
		// Lumen GI section (ReSTIR)
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 12, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("LumenGIHeader", "Lumen GI"),
				LOCTEXT("LumenGIDesc", "LumenScreenProbeGather.usf"),
				TEXT("/Engine/Private/Lumen/LumenScreenProbeGather.usf"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 6, 16, 0)
		[
			MakeBoolRow(
				LOCTEXT("RestirToggleLabel", "ReSTIR Probes"),
				LOCTEXT("RestirToggleTip",
					"Spatial-reuse importance resample (RIS) over 8 real screen probes: "
					"the 4 bilinear corners (engine geometry weights) plus a 4-tap cross "
					"of grid probes Search Radius tiles out (plane-distance rejection "
					"weights).  Each candidate is weighted by its ambient-SH luminance - "
					"the ReSTIR target PDF - and the resampled irradiance is applied as a "
					"bounded per-channel correction on the engine's integrated GI, pulling "
					"colour and level toward the proven high-energy neighbour probes.\n\n"
					"NOTE: this is the spatial-reuse half of Bitterli 2020 ReSTIR (no "
					"persistent reservoir buffer is possible from a shader hook).  The "
					"deterministic weighted sum is the expected value of the stochastic "
					"reservoir update, so it does not shimmer and needs no TAA to resolve."),
				[this]() { return Settings && Settings->bRestirGIProbes; },
				[this](bool V) { if (Settings) Settings->bRestirGIProbes = V; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 0)
		[
			MakeFloatRow(
				LOCTEXT("RestirStrengthLabel", "ReSTIR Strength"),
				LOCTEXT("RestirStrengthTip",
					"Lerp amount of the per-channel correction toward the importance-"
					"resampled neighbourhood irradiance.  0=engine, 1=full correction.  "
					"Typical 0.3..0.6."),
				0.0f, 1.0f,
				[this]() { return Settings ? Settings->RestirStrength : 0.5f; },
				[this](float V) { if (Settings) Settings->RestirStrength = V; },
					/*LiveParamId*/ 31 /* ShaderShiftLive::Live_RestirStrength */)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 0)
		[
			MakeFloatRow(
				LOCTEXT("RestirRadiusLabel", "Search Radius"),
				LOCTEXT("RestirRadiusTip",
					"Spatial-reuse search radius in probe-grid tiles (~16px each) for the "
					"4-tap cross of extended neighbour probes.  Rounded to a whole tile "
					"count (1..4) at compile time.  Plane-distance rejection keeps energy "
					"from crossing depth edges, so larger radii stay stable on flat "
					"receivers but contribute less in cluttered geometry."),
				1.0f, 4.0f,
				[this]() { return Settings ? Settings->RestirSearchRadius : 1.0f; },
				[this](float V) { if (Settings) Settings->RestirSearchRadius = V; })
		]

		// ===================================================
		// Lumen Reflections section
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 12, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("LumenReflHeader", "Lumen Reflections"),
				LOCTEXT("LumenReflDesc", "LumenReflectionCommon.ush"),
				TEXT("/Engine/Private/Lumen/LumenReflectionCommon.ush"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 6, 16, 0)
		[
			MakeBoolRow(
				LOCTEXT("GradReflToggleLabel", "Gradient Filter"),
				LOCTEXT("GradReflToggleTip",
					"Gradient-domain bilateral reflection reconstruction, two halves:\n"
					"1) Sharp trace - tracing roughness is pulled toward mirror so rough "
					"surfaces get a sharp, coherent 1-spp trace instead of a wide noisy "
					"GGX lobe.\n"
					"2) Gradient bilateral - the spatial denoiser re-blurs to match the "
					"TRUE material roughness using a roughness-modulated luminance-"
					"gradient neighbour weight that preserves bright reflection edges."),
				[this]() { return Settings && Settings->bGradientReflectionFilter; },
				[this](bool V) { if (Settings) Settings->bGradientReflectionFilter = V; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 0)
		[
			MakeFloatRow(
				LOCTEXT("GradReflStrengthLabel", "Filter Strength"),
				LOCTEXT("GradReflStrengthTip",
					"Lerp between engine bilateral weight and gradient-aware weight."),
				0.0f, 1.0f,
				[this]() { return Settings ? Settings->GradientReflectionStrength : 0.6f; },
				[this](float V) { if (Settings) Settings->GradientReflectionStrength = V; },
					/*LiveParamId*/ 32 /* ShaderShiftLive::Live_GradReflStrength */)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 12)
		[
			MakeFloatRow(
				LOCTEXT("GradReflSharpTraceLabel", "Sharp Trace"),
				LOCTEXT("GradReflSharpTraceTip",
					"How far the tracing roughness is pulled toward mirror.  0=engine "
					"trace, 1=full mirror trace.  The spatial denoiser still sees the "
					"true roughness and reconstructs the rough appearance."),
				0.0f, 1.0f,
				[this]() { return Settings ? Settings->GradientReflectionSharpTrace : 0.5f; },
				[this](float V) { if (Settings) Settings->GradientReflectionSharpTrace = V; },
					/*LiveParamId*/ 33 /* ShaderShiftLive::Live_GradReflSharpTrace */)
		]
	];
}

void SShaderShiftGITab::RefreshFromSettings()
{
	if (!Settings)
	{
		return;
	}
	CurrentAOOption          = FindOption(AOOptions,          static_cast<uint8>(Settings->AOMode));
	CurrentSSGIOption        = FindOption(SSGIOptions,        static_cast<uint8>(Settings->SSGIMode));
	CurrentSSGIQualityOption = FindOption(SSGIQualityOptions, static_cast<uint8>(Settings->SSGIQuality));
}

#undef LOCTEXT_NAMESPACE
