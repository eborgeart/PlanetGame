// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "QuickPanel/SShaderShiftShadingTab.h"

#include "HAL/IConsoleManager.h"   // r.Substrate prerequisite check
#include "ShaderShiftSettings.h"

// "Fix Up Tagged Materials" (this module owns both tag node classes)
#include "MaterialExpressionCallistoTuning.h"
#include "MaterialExpressionProximaTuning.h"
#include "MaterialExpressionRegolithTuning.h"
#include "MaterialExpressionShaderShiftBRDFSelect.h"

#include "Framework/Notifications/NotificationManager.h"
#include "MaterialShared.h"                       // FMaterialUpdateContext
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "SceneTypes.h"                           // MP_Anisotropy / MP_FrontMaterial
#include "ScopedTransaction.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "ShaderShiftQuickPanel"

// Every BRDF tunable is LIVE: dragging any slider previews instantly in the
// viewport with no recompile, and every dirty knob previews SIMULTANEOUSLY
// (focus lane = the knob under the cursor at full precision, slot lanes =
// the rest at 10-bit).  The high-impact knobs sit directly under their preset
// combo; the fine-grain knobs (falloffs, tints, terminator length) live in
// the collapsed "Advanced" areas below them.  Bake persists everything in one
// recompile.  Focus ids come from ShaderShiftLive::ELiveParamId and MUST match
// the generated SSL_KNOB_* macros (Build/gen_live_slots.py).

namespace
{
	// --- Per-material define toggles ---------------------------------------
	// CALLISTO_PER_MATERIAL_TUNING / CALLISTO_PER_MATERIAL_SELECT have no
	// typed UPROPERTY mirror: the persisted FShaderShiftDefineEntry::Value
	// inside Settings->ShaderHooks IS the setting (the generic-define path).
	// Toggling only edits that stored value -- nothing recompiles until the
	// user presses Apply/Bake, which runs the exact same pending-apply flow
	// as every other global-recompile define: SyncEnumsToDefines (which
	// leaves unmapped names like these untouched), SaveConfig, then
	// ApplyAndRecompileIfNeeded stamps PendingDefines and issues the global
	// recompile.  The values substitute into BRDF.ush (whose hook sets
	// RequiresGlobalRecompile); the Substrate / BasePass templates read the
	// substituted macros through #include "BRDF.ush".

	bool GetHookDefineBool(const UShaderShiftSettings* Settings, const TCHAR* DefineName)
	{
		if (!Settings) { return false; }
		for (const FShaderShiftHookConfig& Hook : Settings->ShaderHooks)
		{
			for (const FShaderShiftDefineEntry& Def : Hook.Defines)
			{
				if (Def.DefineName == DefineName)
				{
					return Def.Value.TrimStartAndEnd() == TEXT("1");
				}
			}
		}
		return false;
	}

	void SetHookDefineBool(UShaderShiftSettings* Settings, const TCHAR* DefineName, bool bValue)
	{
		if (!Settings) { return; }
		// Write EVERY entry with this name (SyncEnumsToDefines loops the same
		// way) so any hook carrying a copy of the define stays in sync.
		for (FShaderShiftHookConfig& Hook : Settings->ShaderHooks)
		{
			for (FShaderShiftDefineEntry& Def : Hook.Defines)
			{
				if (Def.DefineName == DefineName)
				{
					Def.Value = bValue ? TEXT("1") : TEXT("0");
				}
			}
		}
	}
}

void SShaderShiftShadingTab::Construct(const FArguments& InArgs)
{
	Settings = UShaderShiftSettings::Get();
	check(Settings);

	BuildEnumOptions<ECustomDiffuseMode>(DiffuseOptions);
	BuildEnumOptions<ECustomSpecMode>(SpecularOptions);
	BuildEnumOptions<ECallistoDiffusePreset>(CallistoDiffPresetOptions);
	BuildEnumOptions<ECallistoSpecPreset>(CallistoSpecPresetOptions);
	BuildEnumOptions<EMultiLobeSpecPreset>(MultiLobePresetOptions);
	RefreshFromSettings();

	// Shared row shorthand: a live Callisto-diffuse knob that flips the preset
	// back to Custom on edit (identical pattern for every knob in this tab).
	auto DiffCustom = [this]()
	{
		Settings->CallistoDiffusePreset = ECallistoDiffusePreset::Custom;
		CurrentCallistoDiffPresetOption = FindOption(CallistoDiffPresetOptions, (uint8)ECallistoDiffusePreset::Custom);
	};
	auto SpecCustom = [this]()
	{
		Settings->CallistoSpecPreset = ECallistoSpecPreset::Custom;
		CurrentCallistoSpecPresetOption = FindOption(CallistoSpecPresetOptions, (uint8)ECallistoSpecPreset::Custom);
	};
	auto LobeCustom = [this]()
	{
		Settings->MultiLobeSpecPreset = EMultiLobeSpecPreset::Custom;
		CurrentMultiLobePresetOption = FindOption(MultiLobePresetOptions, (uint8)EMultiLobeSpecPreset::Custom);
	};

	ChildSlot
	[
		SNew(SVerticalBox)

		// ===================================================
		// Shading Model section
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 10, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("ShadingHeader", "Shading Model"),
				LOCTEXT("ShadingDesc", "ShadingModels.ush"),
				TEXT("/Engine/Private/ShadingModels.ush"))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 6, 16, 0)
		[
			MakeComboRow(
				LOCTEXT("DiffuseLabel", "Diffuse BRDF"),
				&DiffuseOptions, &CurrentDiffuseOption,
				[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
				{
					if (NewValue.IsValid() && Settings)
					{
						CurrentDiffuseOption = NewValue;
						Settings->DiffuseMode = static_cast<ECustomDiffuseMode>(NewValue->Value);
					}
				})
		]

		// --- Callisto diffuse preset (Diffuse BRDF == Callisto) ---
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return (CurrentDiffuseOption.IsValid() && CurrentDiffuseOption->Value == static_cast<uint8>(ECustomDiffuseMode::Callisto)) ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				MakeComboRow(
					LOCTEXT("CallistoDiffPresetLabel", "Callisto Preset"),
					&CallistoDiffPresetOptions, &CurrentCallistoDiffPresetOption,
					[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
					{
						if (NewValue.IsValid() && Settings)
						{
							CurrentCallistoDiffPresetOption = NewValue;
							Settings->CallistoDiffusePreset = static_cast<ECallistoDiffusePreset>(NewValue->Value);
							Settings->ApplyCallistoDiffusePresetToKnobs();
							// Seeded knobs preview immediately -- all at once.
							ShaderShiftQuickPanel::PushLiveSlots();
						}
					})
			]
		]

		// --- Callisto diffuse live knobs (all preview at once) ---
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return (CurrentDiffuseOption.IsValid() && CurrentDiffuseOption->Value == static_cast<uint8>(ECustomDiffuseMode::Callisto)) ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_CallistoDiffFresnel", "Diffuse Fresnel"),
						LOCTEXT("KT_CallistoDiffFresnel", "Live (no recompile); previews together with every other dirty knob. Editing sets the preset to Custom."),
						0.0f, 8.0f,
						[this]() { return Settings ? Settings->CallistoDiffFresnel : 1.0f; },
						[this, DiffCustom](float V) { if (Settings) { Settings->CallistoDiffFresnel = V; DiffCustom(); } },
						/*LiveParamId*/ 11 /* Live_CallistoDiffFresnel */)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_CallistoRetro", "Retroreflection"),
						LOCTEXT("KT_CallistoRetro", "Live (no recompile). Editing sets the preset to Custom."),
						0.0f, 8.0f,
						[this]() { return Settings ? Settings->CallistoRetro : 1.0f; },
						[this, DiffCustom](float V) { if (Settings) { Settings->CallistoRetro = V; DiffCustom(); } },
						/*LiveParamId*/ 17 /* Live_CallistoRetro */)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_CallistoSmoothTerm", "Smooth Terminator"),
						LOCTEXT("KT_CallistoSmoothTerm", "Live (no recompile). Editing sets the preset to Custom."),
						-1.0f, 1.0f,
						[this]() { return Settings ? Settings->CallistoSmoothTerm : 0.0f; },
						[this, DiffCustom](float V) { if (Settings) { Settings->CallistoSmoothTerm = V; DiffCustom(); } },
						/*LiveParamId*/ 23 /* Live_CallistoSmoothTerm */)
				]

				// --- Advanced Callisto diffuse tunables (collapsed) ---
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 6, 16, 12)
				[
					SNew(SExpandableArea)
					.InitiallyCollapsed(true)
					.HeaderContent()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CallistoAdvHeader", "Advanced (falloffs / tints / terminator)"))
						.ToolTipText(LOCTEXT("CallistoAdvTip", "Fine-grain Callisto diffuse tunables. All live, all simultaneous."))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FShaderShiftPanelStyle::DimTextColor)
					]
					.BodyContent()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CDFF", "Fresnel Falloff"),
								LOCTEXT("KT_CDFF", "Diffuse Fresnel falloff n_f. Live."),
								0.0f, 1.0f,
								[this]() { return Settings ? Settings->CallistoDiffFresnelFalloff : 0.75f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoDiffFresnelFalloff = V; DiffCustom(); } },
								/*LiveParamId*/ 12)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CDFTF", "Fresnel Tan Falloff"),
								LOCTEXT("KT_CDFTF", "Diffuse Fresnel tangent falloff m_f. Live."),
								0.0f, 1.0f,
								[this]() { return Settings ? Settings->CallistoDiffFresnelTanFalloff : 0.75f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoDiffFresnelTanFalloff = V; DiffCustom(); } },
								/*LiveParamId*/ 13)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CDFTR", "Fresnel Tint R"),
								LOCTEXT("KT_CDFTR", "Diffuse Fresnel tint, red. 1 = white. Live."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->CallistoDiffFresnelTintR : 1.0f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoDiffFresnelTintR = V; DiffCustom(); } },
								/*LiveParamId*/ 14)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CDFTG", "Fresnel Tint G"),
								LOCTEXT("KT_CDFTG", "Diffuse Fresnel tint, green. Live."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->CallistoDiffFresnelTintG : 1.0f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoDiffFresnelTintG = V; DiffCustom(); } },
								/*LiveParamId*/ 15)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CDFTB", "Fresnel Tint B"),
								LOCTEXT("KT_CDFTB", "Diffuse Fresnel tint, blue. Live."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->CallistoDiffFresnelTintB : 1.0f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoDiffFresnelTintB = V; DiffCustom(); } },
								/*LiveParamId*/ 16)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CRF", "Retro Falloff"),
								LOCTEXT("KT_CRF", "Retroreflection falloff n_r. Live."),
								0.0f, 1.0f,
								[this]() { return Settings ? Settings->CallistoRetroFalloff : 0.75f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoRetroFalloff = V; DiffCustom(); } },
								/*LiveParamId*/ 18)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CRTF", "Retro Tan Falloff"),
								LOCTEXT("KT_CRTF", "Retroreflection tangent falloff m_r. Live."),
								0.0f, 1.0f,
								[this]() { return Settings ? Settings->CallistoRetroTanFalloff : 0.75f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoRetroTanFalloff = V; DiffCustom(); } },
								/*LiveParamId*/ 19)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CRTR", "Retro Tint R"),
								LOCTEXT("KT_CRTR", "Retroreflection tint, red. Live."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->CallistoRetroTintR : 1.0f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoRetroTintR = V; DiffCustom(); } },
								/*LiveParamId*/ 20)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CRTG", "Retro Tint G"),
								LOCTEXT("KT_CRTG", "Retroreflection tint, green. Live."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->CallistoRetroTintG : 1.0f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoRetroTintG = V; DiffCustom(); } },
								/*LiveParamId*/ 21)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CRTB", "Retro Tint B"),
								LOCTEXT("KT_CRTB", "Retroreflection tint, blue. Live."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->CallistoRetroTintB : 1.0f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoRetroTintB = V; DiffCustom(); } },
								/*LiveParamId*/ 22)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CSTL", "Terminator Length"),
								LOCTEXT("KT_CSTL", "Smooth Terminator length p. Live."),
								0.0f, 1.0f,
								[this]() { return Settings ? Settings->CallistoSmoothTermLength : 0.5f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoSmoothTermLength = V; DiffCustom(); } },
								/*LiveParamId*/ 24)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CSTR", "Terminator Tint R"),
								LOCTEXT("KT_CSTR", "Smooth Terminator tint, red. Live."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->CallistoSmoothTermTintR : 1.0f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoSmoothTermTintR = V; DiffCustom(); } },
								/*LiveParamId*/ 25)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_CSTG", "Terminator Tint G"),
								LOCTEXT("KT_CSTG", "Smooth Terminator tint, green. Live."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->CallistoSmoothTermTintG : 1.0f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoSmoothTermTintG = V; DiffCustom(); } },
								/*LiveParamId*/ 26)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 4)
						[
							MakeFloatRow(
								LOCTEXT("K_CSTB", "Terminator Tint B"),
								LOCTEXT("KT_CSTB", "Smooth Terminator tint, blue. Live."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->CallistoSmoothTermTintB : 1.0f; },
								[this, DiffCustom](float V) { if (Settings) { Settings->CallistoSmoothTermTintB = V; DiffCustom(); } },
								/*LiveParamId*/ 27)
						]
					]
				]
			]
		]

		// --- Proxima single knob (Diffuse BRDF == Proxima) ---
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 12)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return (CurrentDiffuseOption.IsValid() && CurrentDiffuseOption->Value == static_cast<uint8>(ECustomDiffuseMode::Proxima)) ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				MakeFloatRow(
					LOCTEXT("ProximaAlphaLabel", "Diffuse Roughness"),
					LOCTEXT("ProximaAlphaTip", "Proxima diffuse roughness scale (live, no recompile). 0 = Lambert, higher = rougher / more backscatter."),
					0.0f, 4.0f,
					[this]() { return Settings ? Settings->ProximaAlphaScale : 1.0f; },
					[this](float V) { if (Settings) { Settings->ProximaAlphaScale = V; } },
					/*LiveParamId*/ 28 /* Live_ProximaAlphaScale */)
			]
		]

		// --- Regolith dust-layer knobs (Diffuse == Regolith OR Spec == Regolith Sheen) ---
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				const bool bDiff = CurrentDiffuseOption.IsValid() && CurrentDiffuseOption->Value == static_cast<uint8>(ECustomDiffuseMode::Regolith);
				const bool bSpec = CurrentSpecularOption.IsValid() && CurrentSpecularOption->Value == static_cast<uint8>(ECustomSpecMode::RegolithSheen);
				return (bDiff || bSpec) ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_RegolithCoverage", "Dust Coverage"),
						LOCTEXT("KT_RegolithCoverage", "Coverage ceiling: each pixel's roughness gates how much dust it holds (polished sheds, rough holds; high coverage buries all). Grazing views/lights see more dust. 0 = engine-exact. Live."),
						0.0f, 1.0f,
						[this]() { return Settings ? Settings->RegolithCoverage : 0.35f; },
						[this](float V) { if (Settings) { Settings->RegolithCoverage = V; } },
						/*LiveParamId*/ 44 /* Live_RegolithCoverage */)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_RegolithGrainRough", "Grain Roughness"),
						LOCTEXT("KT_RegolithGrainRough", "Roughness of the dust sheen specular lobe. Live."),
						0.05f, 1.0f,
						[this]() { return Settings ? Settings->RegolithGrainRoughness : 0.6f; },
						[this](float V) { if (Settings) { Settings->RegolithGrainRoughness = V; } },
						/*LiveParamId*/ 45 /* Live_RegolithGrainRoughness */)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_RegolithRetro", "Opposition Glow"),
						LOCTEXT("KT_RegolithRetro", "Retro-surge when the light sits behind the camera (lunar-dust heiligenschein). Live."),
						0.0f, 2.0f,
						[this]() { return Settings ? Settings->RegolithRetro : 0.8f; },
						[this](float V) { if (Settings) { Settings->RegolithRetro = V; } },
						/*LiveParamId*/ 46 /* Live_RegolithRetro */)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_RegolithSheen", "Dust Sheen"),
						LOCTEXT("KT_RegolithSheen", "Grain specular gain replacing base gloss under dust. 0 = chalk. Live."),
						0.0f, 2.0f,
						[this]() { return Settings ? Settings->RegolithSheen : 0.6f; },
						[this](float V) { if (Settings) { Settings->RegolithSheen = V; } },
						/*LiveParamId*/ 47 /* Live_RegolithSheen */)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_RegolithRoughAffinity", "Dust Roughness Affinity"),
						LOCTEXT("KT_RegolithRoughAffinity", "1 = dust holds on rough surfaces only (previous behavior); 0 = Dust Coverage applies flat, ignoring the roughness gate. Live."),
						0.0f, 1.0f,
						[this]() { return Settings ? Settings->RegolithRoughnessAffinity : 1.0f; },
						[this](float V) { if (Settings) { Settings->RegolithRoughnessAffinity = V; } },
						/*LiveParamId*/ 48 /* Live_RegolithRoughnessAffinity */)
				]

				// Dust tint (baked-only: live-slot budget is full at 46/46).
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 6, 16, 12)
				[
					SNew(SExpandableArea)
					.InitiallyCollapsed(true)
					.HeaderContent()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("RegolithTintHeader", "Dust Tint (applies on Bake)"))
						.ToolTipText(LOCTEXT("RegolithTintTip", "Dust albedo target and blend. These four are compile-time only -- press Bake to see them."))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
						.ColorAndOpacity(FShaderShiftPanelStyle::DimTextColor)
					]
					.BodyContent()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_RegolithTintR", "Tint R"),
								LOCTEXT("KT_RegolithTintR", "Dust albedo tint target, red. Applies on Bake."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->RegolithTintR : 1.0f; },
								[this](float V) { if (Settings) { Settings->RegolithTintR = V; } })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_RegolithTintG", "Tint G"),
								LOCTEXT("KT_RegolithTintG", "Dust albedo tint target, green. Applies on Bake."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->RegolithTintG : 1.0f; },
								[this](float V) { if (Settings) { Settings->RegolithTintG = V; } })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							MakeFloatRow(
								LOCTEXT("K_RegolithTintB", "Tint B"),
								LOCTEXT("KT_RegolithTintB", "Dust albedo tint target, blue. Applies on Bake."),
								0.0f, 2.0f,
								[this]() { return Settings ? Settings->RegolithTintB : 1.0f; },
								[this](float V) { if (Settings) { Settings->RegolithTintB = V; } })
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 4)
						[
							MakeFloatRow(
								LOCTEXT("K_RegolithTintBlend", "Tint Blend"),
								LOCTEXT("KT_RegolithTintBlend", "0 = dust keeps the base color, 1 = pure tint (chalk / rust / verdigris). Applies on Bake."),
								0.0f, 1.0f,
								[this]() { return Settings ? Settings->RegolithTintBlend : 0.0f; },
								[this](float V) { if (Settings) { Settings->RegolithTintBlend = V; } })
						]
					]
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 12)
		[
			MakeComboRow(
				LOCTEXT("SpecularLabel", "Specular BRDF"),
				&SpecularOptions, &CurrentSpecularOption,
				[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
				{
					if (NewValue.IsValid() && Settings)
					{
						CurrentSpecularOption = NewValue;
						Settings->SpecularMode = static_cast<ECustomSpecMode>(NewValue->Value);
					}
				})
		]

		// --- Dual-Lobe Specular header (MultiLobe or Callisto Dual GGX) ---
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 10, 16, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				if (!CurrentSpecularOption.IsValid()) return EVisibility::Collapsed;
				const uint8 SpecV = CurrentSpecularOption->Value;
				return (SpecV == static_cast<uint8>(ECustomSpecMode::MultiLobe)
					|| SpecV == static_cast<uint8>(ECustomSpecMode::CallistoDualGGX))
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				MakeSectionHeader(
					LOCTEXT("DualSpecHeader", "Dual-Lobe Specular"),
					LOCTEXT("DualSpecDesc", "BRDF.ush"),
					TEXT("/Engine/Private/BRDF.ush"))
			]
		]

		// --- Callisto Dual GGX preset (Specular BRDF == Callisto Dual GGX) ---
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 6, 16, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return (CurrentSpecularOption.IsValid() && CurrentSpecularOption->Value == static_cast<uint8>(ECustomSpecMode::CallistoDualGGX)) ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				MakeComboRow(
					LOCTEXT("CallistoSpecPresetLabel", "Callisto Preset"),
					&CallistoSpecPresetOptions, &CurrentCallistoSpecPresetOption,
					[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
					{
						if (NewValue.IsValid() && Settings)
						{
							CurrentCallistoSpecPresetOption = NewValue;
							Settings->CallistoSpecPreset = static_cast<ECallistoSpecPreset>(NewValue->Value);
							Settings->ApplyCallistoSpecPresetToKnobs();
							ShaderShiftQuickPanel::PushLiveSlots();
						}
					})
			]
		]

		// --- Callisto Dual GGX live knobs ---
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return (CurrentSpecularOption.IsValid() && CurrentSpecularOption->Value == static_cast<uint8>(ECustomSpecMode::CallistoDualGGX)) ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_CallistoDualSpecOpacity", "Dual Spec Opacity"),
						LOCTEXT("KT_CallistoDualSpecOpacity", "Live (no recompile). Editing sets the preset to Custom."),
						0.0f, 1.0f,
						[this]() { return Settings ? Settings->CallistoDualSpecOpacity : 0.0f; },
						[this, SpecCustom](float V) { if (Settings) { Settings->CallistoDualSpecOpacity = V; SpecCustom(); } },
						/*LiveParamId*/ 10 /* Live_CallistoDualSpecOpacity */)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_CallistoDualSpecRoughnessScale", "Dual Spec Roughness"),
						LOCTEXT("KT_CallistoDualSpecRoughnessScale", "Live (no recompile). Editing sets the preset to Custom."),
						1.0f, 14.0f,
						[this]() { return Settings ? Settings->CallistoDualSpecRoughnessScale : 2.0f; },
						[this, SpecCustom](float V) { if (Settings) { Settings->CallistoDualSpecRoughnessScale = V; SpecCustom(); } },
						/*LiveParamId*/ 9 /* Live_CallistoDualSpecRoughScale */)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 12)
				[
					MakeFloatRow(
						LOCTEXT("K_CallistoSpecFresnelFalloff", "Spec Fresnel Falloff"),
						LOCTEXT("KT_CallistoSpecFresnelFalloff", "Live (no recompile). Editing sets the preset to Custom."),
						0.0f, 1.0f,
						[this]() { return Settings ? Settings->CallistoSpecFresnelFalloff : 1.0f; },
						[this, SpecCustom](float V) { if (Settings) { Settings->CallistoSpecFresnelFalloff = V; SpecCustom(); } },
						/*LiveParamId*/ 8 /* Live_CallistoSpecFresnelFalloff */)
				]
			]
		]

		// --- Multi-Lobe Cinematic preset (Specular BRDF == Multi-Lobe) ---
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 6, 16, 0)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return (CurrentSpecularOption.IsValid() && CurrentSpecularOption->Value == static_cast<uint8>(ECustomSpecMode::MultiLobe)) ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				MakeComboRow(
					LOCTEXT("MultiLobePresetLabel", "Multi-Lobe Preset"),
					&MultiLobePresetOptions, &CurrentMultiLobePresetOption,
					[this](FShaderShiftEnumOptionPtr NewValue, ESelectInfo::Type)
					{
						if (NewValue.IsValid() && Settings)
						{
							CurrentMultiLobePresetOption = NewValue;
							Settings->MultiLobeSpecPreset = static_cast<EMultiLobeSpecPreset>(NewValue->Value);
							Settings->ApplyMultiLobePresetToKnobs();
							ShaderShiftQuickPanel::PushLiveSlots();
						}
					})
			]
		]

		// --- Multi-Lobe live knobs ---
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 12)
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return (CurrentSpecularOption.IsValid() && CurrentSpecularOption->Value == static_cast<uint8>(ECustomSpecMode::MultiLobe)) ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_SpecPrimaryWeight", "Primary Weight"),
						LOCTEXT("KT_SpecPrimaryWeight", "Live (no recompile). Editing sets the preset to Custom."),
						0.0f, 1.0f,
						[this]() { return Settings ? Settings->SpecPrimaryWeight : 0.7f; },
						[this, LobeCustom](float V) { if (Settings) { Settings->SpecPrimaryWeight = V; LobeCustom(); } },
						/*LiveParamId*/ 4 /* Live_SpecPrimaryWeight */)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_SpecPrimaryRoughnessScale", "Primary Roughness"),
						LOCTEXT("KT_SpecPrimaryRoughnessScale", "Live (no recompile). Editing sets the preset to Custom."),
						0.0f, 2.0f,
						[this]() { return Settings ? Settings->SpecPrimaryRoughnessScale : 0.45f; },
						[this, LobeCustom](float V) { if (Settings) { Settings->SpecPrimaryRoughnessScale = V; LobeCustom(); } },
						/*LiveParamId*/ 5 /* Live_SpecPrimaryRoughScale */)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 0)
				[
					MakeFloatRow(
						LOCTEXT("K_SpecSecondaryRoughnessScale", "Secondary Roughness"),
						LOCTEXT("KT_SpecSecondaryRoughnessScale", "Live (no recompile). Editing sets the preset to Custom."),
						0.0f, 4.0f,
						[this]() { return Settings ? Settings->SpecSecondaryRoughnessScale : 2.2f; },
						[this, LobeCustom](float V) { if (Settings) { Settings->SpecSecondaryRoughnessScale = V; LobeCustom(); } },
						/*LiveParamId*/ 6 /* Live_SpecSecondaryRoughScale */)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(16, 2, 16, 12)
				[
					MakeFloatRow(
						LOCTEXT("K_SpecSecondaryMaxRoughness", "Secondary Max Roughness"),
						LOCTEXT("KT_SpecSecondaryMaxRoughness", "Clamp for the soft secondary lobe. Live (no recompile). Editing sets the preset to Custom."),
						0.0f, 1.0f,
						[this]() { return Settings ? Settings->SpecSecondaryMaxRoughness : 0.95f; },
						[this, LobeCustom](float V) { if (Settings) { Settings->SpecSecondaryMaxRoughness = V; LobeCustom(); } },
						/*LiveParamId*/ 7 /* Live_SpecSecondaryMaxRough */)
				]
			]
		]

		// ===================================================
		// Per-Material Overrides section
		// ===================================================
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 10, 16, 0)
		[
			MakeSectionHeader(
				LOCTEXT("PerMaterialHeader", "Per-Material Overrides"),
				LOCTEXT("PerMaterialDesc", "BasePassPixelShader.usf"),
				TEXT("/Engine/Private/BasePassPixelShader.usf"))
		]

		// Substrate prerequisite warning: the per-material transport rides
		// Substrate BSDF state bits + slab channels (F0 / anisotropy / fuzz).
		// On a legacy-deferred project the encode/decode compile out entirely
		// and tagged materials silently fall back to the global BRDF.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			SNew(STextBlock)
			.Visibility_Lambda([]()
			{
				static const auto* SubstrateCVar =
					IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Substrate"));
				const bool bSubstrate = SubstrateCVar && SubstrateCVar->GetValueOnGameThread() != 0;
				return bSubstrate ? EVisibility::Collapsed : EVisibility::Visible;
			})
			.Text(LOCTEXT("PerMaterialNeedsSubstrate",
				"Substrate is disabled in this project -- these overrides have no effect.\n"
				"Per-material data rides Substrate BSDF state (r.Substrate=1, "
				"r.Substrate.ShadingQuality=2); legacy materials use the global BRDF."))
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
			.ColorAndOpacity(FLinearColor(1.0f, 0.75f, 0.35f, 1.0f))
			.AutoWrapText(true)
			.ToolTipText(LOCTEXT("PerMaterialNeedsSubstrateTip",
				"Enable Substrate materials (Project Settings > Rendering > Substrate, requires restart) "
				"and set r.Substrate.ShadingQuality=2 so the reserved BSDF state bits survive. "
				"Materials may also need a complexity-triggering input (e.g. a small constant into "
				"Anisotropy) so the complex slab path is compiled."))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 6, 16, 0)
		[
			MakeBoolRow(
				LOCTEXT("PerMatTuningLabel", "Per-Material Tuning"),
				LOCTEXT("PerMatTuningTip",
					"1 = decode per-material Diffuse Fresnel multipliers from the "
					"Specular channel (set by the Callisto Per-Material Tuning "
					"material node). Repurposes the Specular channel on tagged "
					"materials."),
				[this]() { return GetHookDefineBool(Settings, TEXT("CALLISTO_PER_MATERIAL_TUNING")); },
				[this](bool bValue) { SetHookDefineBool(Settings, TEXT("CALLISTO_PER_MATERIAL_TUNING"), bValue); })
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 4, 16, 0)
		[
			MakeBoolRow(
				LOCTEXT("PerMatSelectLabel", "Per-Material BRDF Selection"),
				LOCTEXT("PerMatSelectTip",
					"A material tagged with the Callisto BRDF Select node overrides "
					"the global Diffuse/Specular BRDF mode per-material; Complex "
					"path / sg.ShadingQuality 2 required. Adds a runtime BRDF "
					"switch to the lighting shader."),
				[this]() { return GetHookDefineBool(Settings, TEXT("CALLISTO_PER_MATERIAL_SELECT")); },
				[this](bool bValue) { SetHookDefineBool(Settings, TEXT("CALLISTO_PER_MATERIAL_SELECT"), bValue); })
		]

		// One-click fixer for the manual "Constant -> Anisotropy" wire the
		// per-material transport depends on: the translator only compiles
		// the complex Substrate slab permutation when Anisotropy is
		// connected, so tagged materials with a bare pin silently fall
		// back to the global BRDF.
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(16, 8, 16, 12)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.ContentPadding(FMargin(0, 6))
			.Text(LOCTEXT("FixUpTaggedMaterialsBtn", "Fix Up Tagged Materials"))
			.ToolTipText(LOCTEXT("FixUpTaggedMaterialsTip",
				"Scans loaded materials containing ShaderShift per-material nodes "
				"and wires a 0.0001 constant into unconnected Anisotropy pins, "
				"forcing the complex Substrate permutation the per-material "
				"transport requires. Load the materials you want fixed first."))
			.OnClicked(this, &SShaderShiftShadingTab::OnFixUpTaggedMaterialsClicked)
		]
	];
}

FReply SShaderShiftShadingTab::OnFixUpTaggedMaterialsClicked()
{
	// Pass 1 (read-only): classify every LOADED project material carrying
	// one of the ShaderShift per-material tag nodes.  Collecting first
	// keeps the undo transaction below from ever being recorded empty.
	TArray<UMaterial*> MaterialsToFix;
	int32 NumAlreadyOK = 0;
	int32 NumSkipped   = 0;

	for (TObjectIterator<UMaterial> It; It; ++It)
	{
		UMaterial* Material = *It;
		if (!IsValid(Material) ||
			Material->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject | RF_Transient))
		{
			continue;
		}

		// Project content only -- skips transient previews and
		// engine / plugin materials.
		const UPackage* Package = Material->GetOutermost();
		if (!Package || Package == GetTransientPackage() ||
			!Package->GetName().StartsWith(TEXT("/Game")))
		{
			continue;
		}

		bool bTagged = false;
		for (UMaterialExpression* Expression : Material->GetExpressions())
		{
			if (Expression &&
				(Expression->IsA<UMaterialExpressionCallistoTuning>() ||
				 Expression->IsA<UMaterialExpressionRegolithTuning>() ||
				 Expression->IsA<UMaterialExpressionProximaTuning>() ||
				 Expression->IsA<UMaterialExpressionShaderShiftBRDFSelect>()))
			{
				bTagged = true;
				break;
			}
		}
		if (!bTagged)
		{
			continue;
		}

		if (Material->IsPropertyConnected(MP_Anisotropy))
		{
			++NumAlreadyOK;
		}
		else if (Material->bUseMaterialAttributes ||
				 Material->IsPropertyConnected(MP_FrontMaterial))
		{
			// The legacy Anisotropy pin never reaches the translator on
			// attribute- or Substrate-authored graphs, so the wire would be
			// a silent no-op -- report these instead of "fixing" them.
			++NumSkipped;
		}
		else
		{
			MaterialsToFix.Add(Material);
		}
	}

	// Pass 2: wire the constant, one undoable transaction for the batch.
	int32 NumFixed = 0;
	if (MaterialsToFix.Num() > 0)
	{
		FScopedTransaction Transaction(
			LOCTEXT("FixUpTaggedMaterialsTransaction", "ShaderShift Fix Up Tagged Materials"));

		for (UMaterial* Material : MaterialsToFix)
		{
			// Expressions and input connections live on the editor-only object.
			UMaterialEditorOnlyData* EditorOnly = Material->GetEditorOnlyData();
			if (!EditorOnly)
			{
				++NumSkipped;
				continue;
			}

			Material->Modify();
			EditorOnly->Modify();

			// Park the constant just below the existing graph (bottom-left)
			// so it never overlaps the user's nodes.
			int32 NodePosX = 0;
			int32 NodePosY = 0;
			bool  bFirstExpression = true;
			for (const UMaterialExpression* Expression : Material->GetExpressions())
			{
				if (!Expression)
				{
					continue;
				}
				NodePosX = bFirstExpression ? Expression->MaterialExpressionEditorX
					: FMath::Min(NodePosX, Expression->MaterialExpressionEditorX);
				NodePosY = bFirstExpression ? Expression->MaterialExpressionEditorY
					: FMath::Max(NodePosY, Expression->MaterialExpressionEditorY);
				bFirstExpression = false;
			}

			// Same construction steps as UMaterialEditingLibrary::
			// CreateMaterialExpressionEx, inlined so we do not pull in a
			// MaterialEditor module dependency.
			UMaterialExpressionConstant* Constant =
				NewObject<UMaterialExpressionConstant>(Material, NAME_None, RF_Transactional);
			Constant->R = 0.0001f;
			Constant->Material = Material;
			Constant->MaterialExpressionEditorX = NodePosX;
			Constant->MaterialExpressionEditorY = NodePosY + 150;
			Constant->UpdateMaterialExpressionGuid(/*bForceGeneration*/ true, /*bAllowMarkingPackageDirty*/ false);
			Material->GetExpressionCollection().AddExpression(Constant);

			EditorOnly->Anisotropy.Connect(0, Constant);

			// Same recompile flow as UMaterialEditingLibrary::RecompileMaterial
			// (the update context propagates to dependent material instances).
			{
				FMaterialUpdateContext UpdateContext;
				UpdateContext.AddMaterial(Material);
				Material->PreEditChange(nullptr);
				Material->PostEditChange();
			}
			Material->MarkPackageDirty();
			++NumFixed;
		}
	}

	FText Message = FText::Format(
		LOCTEXT("FixUpTaggedMaterialsResult", "Fixed {0} material(s); {1} already OK"),
		FText::AsNumber(NumFixed), FText::AsNumber(NumAlreadyOK));
	if (NumSkipped > 0)
	{
		Message = FText::Format(
			LOCTEXT("FixUpTaggedMaterialsResultSkipped",
				"{0}; {1} skipped (Material Attributes / Substrate-authored graph)"),
			Message, FText::AsNumber(NumSkipped));
	}

	FNotificationInfo Info(Message);
	Info.ExpireDuration = 6.0f;
	TSharedPtr<SNotificationItem> Notification =
		FSlateNotificationManager::Get().AddNotification(Info);
	if (Notification.IsValid())
	{
		Notification->SetCompletionState(
			NumFixed > 0 ? SNotificationItem::CS_Success : SNotificationItem::CS_None);
	}

	return FReply::Handled();
}

void SShaderShiftShadingTab::RefreshFromSettings()
{
	if (!Settings)
	{
		return;
	}
	CurrentDiffuseOption  = FindOption(DiffuseOptions,  static_cast<uint8>(Settings->DiffuseMode));
	CurrentSpecularOption = FindOption(SpecularOptions, static_cast<uint8>(Settings->SpecularMode));
	CurrentCallistoDiffPresetOption = FindOption(CallistoDiffPresetOptions, static_cast<uint8>(Settings->CallistoDiffusePreset));
	CurrentCallistoSpecPresetOption = FindOption(CallistoSpecPresetOptions, static_cast<uint8>(Settings->CallistoSpecPreset));
	CurrentMultiLobePresetOption    = FindOption(MultiLobePresetOptions,    static_cast<uint8>(Settings->MultiLobeSpecPreset));
}

#undef LOCTEXT_NAMESPACE
