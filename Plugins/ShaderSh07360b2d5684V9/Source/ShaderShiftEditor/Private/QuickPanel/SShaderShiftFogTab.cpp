// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "QuickPanel/SShaderShiftFogTab.h"
#include "ShaderShiftSettings.h"

#define LOCTEXT_NAMESPACE "ShaderShiftQuickPanel"

void SShaderShiftFogTab::Construct(const FArguments& InArgs)
{
	Settings = UShaderShiftSettings::Get();
	check(Settings);

	BuildEnumOptions<ECustomVolPhaseMode>(VolPhaseOptions);
	BuildEnumOptions<ECustomVolSelfShadow>(VolSelfShadowOptions);
	BuildEnumOptions<ECustomVolMultiScatter>(VolMultiScatterOptions);
	RefreshFromSettings();

	ChildSlot
	[
		SNew(SVerticalBox)

		// ===================================================
		// Volumetric Fog section
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 10, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("VolFogHeader", "Volumetric Fog"),
				LOCTEXT("VolFogDesc", "VolumetricFog.usf"),
				TEXT("/Engine/Private/VolumetricFog.usf"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 6, 16, 0)
		[
			MakeComboRow(
				LOCTEXT("VolPhaseLabel", "Phase Function"),
				&VolPhaseOptions, &CurrentVolPhaseOption,
				[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
				{
					if (NewValue.IsValid() && Settings)
					{
						CurrentVolPhaseOption = NewValue;
						Settings->VolPhaseMode = static_cast<ECustomVolPhaseMode>(NewValue->Value);
					}
				})
		]

		// Self-Shadow Steps -- always visible (Phase A)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			MakeComboRow(
				LOCTEXT("VolSelfShadowLabel", "Self-Shadow"),
				&VolSelfShadowOptions, &CurrentVolSelfShadowOption,
				[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
				{
					if (NewValue.IsValid() && Settings)
					{
						CurrentVolSelfShadowOption = NewValue;
						Settings->VolSelfShadow = static_cast<ECustomVolSelfShadow>(NewValue->Value);
					}
				})
		]

		// Multi-Scatter Octaves -- always visible (Phase B)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			MakeComboRow(
				LOCTEXT("VolMultiScatterLabel", "Multi-Scatter"),
				&VolMultiScatterOptions, &CurrentVolMultiScatterOption,
				[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
				{
					if (NewValue.IsValid() && Settings)
					{
						CurrentVolMultiScatterOption = NewValue;
						Settings->VolMultiScatter = static_cast<ECustomVolMultiScatter>(NewValue->Value);
					}
				})
		]

		// Spectral Strength -- always visible (Phase C)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			MakeFloatRow(
				LOCTEXT("VolSpectralLabel", "Spectral Tint"),
				LOCTEXT("VolSpectralTip",
					"Phase C atmospheric spectral tint.  Modulates the directional "
					"light contribution to fog scattering by per-wavelength Rayleigh "
					"extinction based on sun elevation, producing the blue-hour / "
					"golden-hour color shift.  0 = off, 1 = physical-ish, 2 = cinematic."),
				0.0f, 2.0f,
				[this]() { return Settings ? Settings->VolSpectralStrength : 0.0f; },
				[this](float V) { if (Settings) Settings->VolSpectralStrength = V; },
					/*LiveParamId*/ 34 /* ShaderShiftLive::Live_VolSpectralStrength */)
		]

		// Dual-Lobe Secondary G -- only visible for DualLobeHG mode
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				if (!CurrentVolPhaseOption.IsValid()) return EVisibility::Collapsed;
				return CurrentVolPhaseOption->Value == static_cast<uint8>(ECustomVolPhaseMode::DualLobeHG)
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				MakeFloatRow(
					LOCTEXT("VolPhaseG2Label", "Secondary G"),
					LOCTEXT("VolPhaseG2Tip",
						"Anisotropy of the second HG lobe in Dual-Lobe mode. "
						"-0.3 = soft backscatter halo, +0.95 = sharp forward peak."),
					-1.0f, 1.0f,
					[this]() { return Settings ? Settings->VolPhaseG2 : -0.3f; },
					[this](float V) { if (Settings) Settings->VolPhaseG2 = V; },
						/*LiveParamId*/ 35 /* ShaderShiftLive::Live_VolPhaseG2 */)
			]
		]

		// Dual-Lobe Blend -- only visible for DualLobeHG mode
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				if (!CurrentVolPhaseOption.IsValid()) return EVisibility::Collapsed;
				return CurrentVolPhaseOption->Value == static_cast<uint8>(ECustomVolPhaseMode::DualLobeHG)
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				MakeFloatRow(
					LOCTEXT("VolPhaseBlendLabel", "Lobe Blend"),
					LOCTEXT("VolPhaseBlendTip",
						"Blend between primary and secondary HG lobes. "
						"0 = primary lobe only, 1 = secondary lobe only. Typical: 0.3."),
					0.0f, 1.0f,
					[this]() { return Settings ? Settings->VolPhaseBlend : 0.3f; },
					[this](float V) { if (Settings) Settings->VolPhaseBlend = V; },
						/*LiveParamId*/ 36 /* ShaderShiftLive::Live_VolPhaseBlend */)
			]
		]

		// Mie Droplet Diameter -- only visible for MieApprox mode
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				if (!CurrentVolPhaseOption.IsValid()) return EVisibility::Collapsed;
				return CurrentVolPhaseOption->Value == static_cast<uint8>(ECustomVolPhaseMode::MieApprox)
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				MakeFloatRow(
					LOCTEXT("VolMieDropletLabel", "Droplet (μm)"),
					LOCTEXT("VolMieDropletTip",
						"Droplet diameter in micrometers for the Jendersie-d'Eon Mie "
						"approximation. 5 = fine mist, 15 = fog, 50 = light cloud, 100+ = dense cloud."),
					1.0f, 200.0f,
					[this]() { return Settings ? Settings->VolMieDropletDiameter : 15.0f; },
					[this](float V) { if (Settings) Settings->VolMieDropletDiameter = V; },
						/*LiveParamId*/ 37 /* ShaderShiftLive::Live_VolMieDroplet */)
			]
		]

		// Powder Edge Glow -- always visible
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			MakeFloatRow(
				LOCTEXT("VolPowderLabel", "Powder Glow"),
				LOCTEXT("VolPowderTip",
					"Schneider 2015 powder term in the final integration pass. "
					"0 = engine Beer-Lambert only. ~0.5 produces the characteristic "
					"silver-lining glow at fog and cloud edges. Free perf."),
				0.0f, 1.0f,
				[this]() { return Settings ? Settings->VolPowderBlend : 0.0f; },
				[this](float V) { if (Settings) Settings->VolPowderBlend = V; },
					/*LiveParamId*/ 38 /* ShaderShiftLive::Live_VolPowderBlend */)
		]

		// R2 Quasirandom Jitter checkbox -- always visible (free win)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 12)
		[
			MakeBoolRow(
				LOCTEXT("VolBlueNoiseLabel", "R2 Jitter"),
				LOCTEXT("VolBlueNoiseTip",
					"Replaces the engine's per-supersample PCG hash with a "
					"Martin Roberts R2 quasirandom sequence. R2 has a blue-noise-like "
					"spectrum, so TAA absorbs the frame-to-frame jitter as smooth "
					"motion rather than residual sparkle. Zero perf cost."),
				[this]() { return Settings && Settings->bVolBlueNoise; },
				[this](bool V) { if (Settings) Settings->bVolBlueNoise = V; })
		]
	];
}

void SShaderShiftFogTab::RefreshFromSettings()
{
	if (!Settings)
	{
		return;
	}
	CurrentVolPhaseOption        = FindOption(VolPhaseOptions,        static_cast<uint8>(Settings->VolPhaseMode));
	CurrentVolSelfShadowOption   = FindOption(VolSelfShadowOptions,   static_cast<uint8>(Settings->VolSelfShadow));
	CurrentVolMultiScatterOption = FindOption(VolMultiScatterOptions, static_cast<uint8>(Settings->VolMultiScatter));
}

#undef LOCTEXT_NAMESPACE
