// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "QuickPanel/SShaderShiftAATab.h"
#include "ShaderShiftSettings.h"

#define LOCTEXT_NAMESPACE "ShaderShiftQuickPanel"

void SShaderShiftAATab::Construct(const FArguments& InArgs)
{
	Settings = UShaderShiftSettings::Get();
	check(Settings);

	ChildSlot
	[
		SNew(SVerticalBox)

		// ===================================================
		// Temporal AA section (drives both TAA + TSR hooks)
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 10, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("TaaHeader", "Temporal AA / TSR"),
				LOCTEXT("TaaDesc", "TemporalAA.usf + TSRUpdateHistory.usf"),
				TEXT("/Engine/Private/TemporalSuperResolution/TSRUpdateHistory.usf"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 6, 16, 0)
		[
			MakeBoolRow(
				LOCTEXT("TaaGhostToggleLabel", "Ghosting Tweak"),
				LOCTEXT("TaaGhostToggleTip",
					"Trade ghosting vs stability, driven by Clamp Factor.\n\n"
					"This single toggle drives TWO shader hooks under the hood:\n"
					"  - TemporalAA.usf::ClampHistory (legacy TAA / DOF / Hair / Light Shaft / SSR):\n"
					"    history-clamp neighbour AABB rescale.\n"
					"  - TSRUpdateHistory.usf (default UE5 viewport AA): AABB rescale PLUS\n"
					"    velocity-driven history invalidation PLUS a scale-down of the\n"
					"    contrast-stability floor that normally keeps ghost trails alive.\n"
					"    (The AABB rescale alone barely moves TSR ghosting - the validity\n"
					"    hooks are what actually kill trails.)\n\n"
					"So enabling the toggle covers both legacy-TAA and TSR-using projects."),
				[this]() { return Settings && Settings->bTaaGhostingTweak; },
				[this](bool V) { if (Settings) Settings->bTaaGhostingTweak = V; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 0)
		[
			MakeFloatRow(
				LOCTEXT("TaaClampLabel", "Clamp Factor"),
				LOCTEXT("TaaClampTip",
					"1.0=engine. <1 = stronger ghost rejection (tighter clamp; on TSR also faster "
					"history invalidation under motion and a weaker contrast-stability floor), "
					"at the cost of more flicker/noise on moving content. >1 = smoother motion, "
					"more ghosting. Typical anti-ghosting value 0.4-0.7."),
				0.1f, 4.0f,
				[this]() { return Settings ? Settings->TaaClampFactor : 1.0f; },
				[this](float V) { if (Settings) Settings->TaaClampFactor = V; },
					/*LiveParamId*/ 39 /* ShaderShiftLive::Live_TaaClampFactor */)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 0)
		[
			MakeBoolRow(
				LOCTEXT("TaaSharpenToggleLabel", "TAA Sharpen"),
				LOCTEXT("TaaSharpenToggleTip",
					"Flatness-weighted temporal contrast push on the clamped history.\n\n"
					"NOT a true spatial CAS sharpen (we only have temporal neighbour AABB in scope, "
					"not the 3x3 spatial ring) - the effect is to push the clamped colour away from "
					"the temporal-mean centre, which reads as added image contrast in flat regions.\n\n"
					"Legacy TAA path only.  TSR is deliberately left untouched here because TSR has "
					"its own frequency-band reaccumulation that handles sharpness."),
				[this]() { return Settings && Settings->bTaaSharpen; },
				[this](bool V) { if (Settings) Settings->bTaaSharpen = V; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 0)
		[
			MakeFloatRow(
				LOCTEXT("TaaSharpenLabel", "Sharpen Strength"),
				LOCTEXT("TaaSharpenTip",
					"Blend weight of the contrast push.  0=off, 0.4=subtle, 0.8=aggressive.  "
					"Relative-contrast flatness weight already attenuates the push on high-contrast "
					"edges so over-sharpening is rare."),
				0.0f, 2.0f,
				[this]() { return Settings ? Settings->TaaSharpenStrength : 0.4f; },
				[this](float V) { if (Settings) Settings->TaaSharpenStrength = V; },
					/*LiveParamId*/ 40 /* ShaderShiftLive::Live_TaaSharpenStrength */)
		]

		// ===================================================
		// Shadows section (VSM contact-aware hardening)
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 12, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("ShadowsHeader", "VSM Shadows"),
				LOCTEXT("ShadowsDesc", "VirtualShadowMapProjectionFilter.ush"),
				TEXT("/Engine/Private/VirtualShadowMaps/VirtualShadowMapProjectionFilter.ush"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 6, 16, 0)
		[
			MakeBoolRow(
				LOCTEXT("ContactShadToggleLabel", "Contact-Aware"),
				LOCTEXT("ContactShadToggleTip",
					"PCSS-style contact-aware penumbra hardening for Virtual Shadow Maps.\n\n"
					"Uses In.OccluderDistance from the SMRT trace (the actual receiver-to-occluder "
					"distance in world units) to drive a smoothstep remap of the partial-occlusion "
					"ShadowFactor: close occluders -> narrow band -> hard contact; distant occluders "
					"-> wide band -> soft penumbra.  Runs after the engine's built-in blue-noise "
					"dither so the dither survives as fine grain on the hardened transition.\n\n"
					"VSM-only.  Legacy CSM / forward shadow paths are not affected by this toggle."),
				[this]() { return Settings && Settings->bContactAwareShadows; },
				[this](bool V) { if (Settings) Settings->bContactAwareShadows = V; })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 0)
		[
			MakeFloatRow(
				LOCTEXT("ContactShadStrengthLabel", "Hardening"),
				LOCTEXT("ContactShadStrengthTip",
					"Lerp between engine-filtered VSM result and the contact-hardened curve. "
					"0=engine, 1=full hardening."),
				0.0f, 1.0f,
				[this]() { return Settings ? Settings->ContactShadowHardening : 0.5f; },
				[this](float V) { if (Settings) Settings->ContactShadowHardening = V; },
					/*LiveParamId*/ 41 /* ShaderShiftLive::Live_ContactShadowHardening */)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(16, 4, 16, 12)
		[
			MakeFloatRow(
				LOCTEXT("ContactShadSoftnessLabel", "Softness Scale"),
				LOCTEXT("ContactShadSoftnessTip",
					"World-space distance multiplier applied to OccluderDistance before "
					"normalisation.  1.0 = 100cm to full softness (interior-scale default).  "
					"<1 = harder shadows even at distance; >1 = softer transitions."),
				0.1f, 4.0f,
				[this]() { return Settings ? Settings->ContactShadowSoftness : 1.0f; },
				[this](float V) { if (Settings) Settings->ContactShadowSoftness = V; },
					/*LiveParamId*/ 42 /* ShaderShiftLive::Live_ContactShadowSoftness */)
		]
	];
}

void SShaderShiftAATab::RefreshFromSettings()
{
	// No combo selections to re-point: every row reads the settings object
	// live through its getter lambda.
}

#undef LOCTEXT_NAMESPACE
