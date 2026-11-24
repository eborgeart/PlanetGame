// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "QuickPanel/ShaderShiftQuickPanelCommon.h"

#include "ShaderShift.h"          // FShaderShiftHookRegistry / FShaderShiftFileHook
#include "ShaderShiftRuntimeParams.h"  // ShaderShiftLive channel
#include "ShaderShiftSettings.h"

#define LOCTEXT_NAMESPACE "ShaderShiftQuickPanel"

// ----------------------------------------------------------------------------
// Style constants -- byte-identical to the original monolithic panel's locals.
// ----------------------------------------------------------------------------
const FLinearColor FShaderShiftPanelStyle::AccentColor(0.0f, 0.867f, 1.0f, 1.0f);
const FLinearColor FShaderShiftPanelStyle::DimTextColor(0.55f, 0.55f, 0.55f, 1.0f);
const FLinearColor FShaderShiftPanelStyle::BrightTextColor(0.9f, 0.9f, 0.9f, 1.0f);
const FLinearColor FShaderShiftPanelStyle::PanelBgColor(0.08f, 0.08f, 0.09f, 1.0f);
const FLinearColor FShaderShiftPanelStyle::SectionBgColor(0.11f, 0.11f, 0.13f, 1.0f);
const FLinearColor FShaderShiftPanelStyle::ButtonBgColor(0.14f, 0.14f, 0.16f, 1.0f);

// ----------------------------------------------------------------------------
// Free helpers
// ----------------------------------------------------------------------------
namespace ShaderShiftQuickPanel
{

// --- Live slot channel + baked snapshot -----------------------------------
// The generated knob table is the single source of truth for focus ids, slot
// indices and [lo,hi] quantization ranges.  It is emitted together with the
// matching SSL_KNOB_* shader macros by Build/gen_live_slots.py -- regenerate
// both sides together, never hand-edit one.
#include "ShaderShiftLiveKnobTable.gen.inl"

namespace
{
	// Baked snapshot: per-knob value currently compiled into the #defines.
	// Seeded at startup (Settings == last-baked at that point), re-seeded on
	// every bake/apply path.  dirty == Settings differs from this snapshot.
	static TMap<int32, float> GBakedSnapshot;

	// Baked preset-dropdown state, snapshotted/restored alongside the knobs.
	struct FShaderShiftBakedPresets
	{
		ECallistoDiffusePreset CallistoDiffuse = ECallistoDiffusePreset::Custom;
		ECallistoSpecPreset    CallistoSpec    = ECallistoSpecPreset::Custom;
		EMultiLobeSpecPreset   MultiLobeSpec   = EMultiLobeSpecPreset::Custom;
		bool                   bValid          = false;
	};
	static FShaderShiftBakedPresets GBakedPresets;

	const FShaderShiftLiveKnobDef* FindKnob(int32 FocusId)
	{
		for (const FShaderShiftLiveKnobDef& K : GShaderShiftLiveKnobs)
		{
			if (K.FocusId == FocusId) { return &K; }
		}
		return nullptr;
	}

	bool IsKnobDirtyInternal(const FShaderShiftLiveKnobDef& K, const UShaderShiftSettings& S)
	{
		const float* Baked = GBakedSnapshot.Find(K.FocusId);
		return Baked && !FMath::IsNearlyEqual(K.Get(S), *Baked, 1.e-6f);
	}
}

namespace
{
	// gen.inl Group values: 0=none 1=MultiLobe(S1) 2=CallistoDualSpec(S2)
	// 3=RegolithSpec(S3) 4=CallistoDiffuse(D5) 5=Proxima(D6) 6=RegolithDiffuse(D7)
	bool IsBankGroupOwned(int32 Group, const UShaderShiftSettings& S)
	{
		switch (Group)
		{
		case 1: return S.SpecularMode == ECustomSpecMode::MultiLobe;
		case 2: return S.SpecularMode == ECustomSpecMode::CallistoDualGGX;
		case 3: return S.SpecularMode == ECustomSpecMode::RegolithSheen;
		case 4: return S.DiffuseMode  == ECustomDiffuseMode::Callisto;
		case 5: return S.DiffuseMode  == ECustomDiffuseMode::Proxima;
		case 6: return S.DiffuseMode  == ECustomDiffuseMode::Regolith;
		default: return false;
		}
	}
}

FText GetLiveBankOwnersText()
{
	const UShaderShiftSettings* S = UShaderShiftSettings::Get();
	if (!S) { return FText::GetEmpty(); }
	auto DiffName = [](ECustomDiffuseMode M) -> FText
	{
		switch (M)
		{
		case ECustomDiffuseMode::Callisto: return LOCTEXT("BankDiffCallisto", "Callisto");
		case ECustomDiffuseMode::Proxima:  return LOCTEXT("BankDiffProxima",  "Proxima");
		case ECustomDiffuseMode::Regolith: return LOCTEXT("BankDiffRegolith", "Regolith");
		default:                           return LOCTEXT("BankDiffEngine",   "engine");
		}
	};
	auto SpecName = [](ECustomSpecMode M) -> FText
	{
		switch (M)
		{
		case ECustomSpecMode::MultiLobe:       return LOCTEXT("BankSpecMultiLobe", "Multi-lobe");
		case ECustomSpecMode::CallistoDualGGX: return LOCTEXT("BankSpecCallisto",  "Callisto dual");
		case ECustomSpecMode::RegolithSheen:   return LOCTEXT("BankSpecRegolith",  "Regolith");
		default:                               return LOCTEXT("BankSpecEngine",    "engine");
		}
	};
	return FText::Format(LOCTEXT("LiveBankOwners", "Live banks: {0} diffuse \u00b7 {1} spec"),
		DiffName(S->DiffuseMode), SpecName(S->SpecularMode));
}

void PushLiveSlots()
{
	UShaderShiftSettings* S = UShaderShiftSettings::Get();
	if (!S) { return; }

	// All slot codes start at 1023 = clean -> shader uses the baked default.
	// (Bits [29:0] set, [31:30] clear: always a finite float bit pattern.)
	uint32 Bits[GShaderShiftLiveSlotFloats];
	for (uint32& B : Bits) { B = 0x3FFFFFFFu; }

	// Control lane: which diffuse/spec mode owns the shared BRDF bank lanes.
	// The decode macros gate banked knobs on this, so a non-owner mode's knob
	// reads its baked default instead of a foreign knob's code.
	{
		const uint32 Ctrl = (uint32)S->DiffuseMode | ((uint32)S->SpecularMode << 4);
		const int32 F = GShaderShiftCtrlLane / 3;
		const int32 L = GShaderShiftCtrlLane % 3;
		Bits[F] = (Bits[F] & ~(0x3FFu << (L * 10))) | (Ctrl << (L * 10));
	}

	int32 DirtyCount = 0;
	for (const FShaderShiftLiveKnobDef& K : GShaderShiftLiveKnobs)
	{
		if (!IsKnobDirtyInternal(K, *S))
		{
			continue;
		}
		// Resolve the knob's lane: focus-lane-only knobs (Slot == -1) have no
		// lane; banked knobs get one only while their mode group OWNS the bank
		// (dual-axis Regolith knobs try the spec bank, then the diffuse bank).
		int32 Lane = -1;
		if (K.Group == 0)
		{
			Lane = K.Slot;
		}
		else if (IsBankGroupOwned(K.Group, *S))
		{
			Lane = K.Slot;
		}
		else if (K.AltGroup != 0 && IsBankGroupOwned(K.AltGroup, *S))
		{
			Lane = K.AltSlot;
		}
		if (Lane < 0)
		{
			continue; // focus-lane only (or non-owner mode): reads baked
		}
		const float V    = K.Get(*S);
		const float Norm = FMath::Clamp((V - K.Lo) / (K.Hi - K.Lo), 0.0f, 1.0f);
		const uint32 Code = (uint32)FMath::Clamp(FMath::RoundToInt(Norm * 1022.0f), 0, 1022);
		const int32 F = Lane / 3;
		const int32 L = Lane % 3;
		Bits[F] = (Bits[F] & ~(0x3FFu << (L * 10))) | (Code << (L * 10));
		++DirtyCount;
	}

	ShaderShiftLive::SetLiveSlots(Bits, DirtyCount > 0);
}

int32 GetLiveDirtyCount()
{
	UShaderShiftSettings* S = UShaderShiftSettings::Get();
	if (!S) { return 0; }
	int32 N = 0;
	for (const FShaderShiftLiveKnobDef& K : GShaderShiftLiveKnobs)
	{
		if (IsKnobDirtyInternal(K, *S)) { ++N; }
	}
	return N;
}

bool IsKnobDirty(int32 LiveParamId)
{
	UShaderShiftSettings* S = UShaderShiftSettings::Get();
	if (!S) { return false; }
	const FShaderShiftLiveKnobDef* K = FindKnob(LiveParamId);
	return K && IsKnobDirtyInternal(*K, *S);
}

void ReseedBakedSnapshot()
{
	if (UShaderShiftSettings* S = UShaderShiftSettings::Get())
	{
		for (const FShaderShiftLiveKnobDef& K : GShaderShiftLiveKnobs)
		{
			GBakedSnapshot.Add(K.FocusId, K.Get(*S));
		}
		// Preset dropdowns are part of the baked state too: without this,
		// Revert restores the knob floats but leaves the preset enums (and so
		// the combos, via RefreshFromSettings) pointing at the un-baked pick.
		GBakedPresets.CallistoDiffuse = S->CallistoDiffusePreset;
		GBakedPresets.CallistoSpec    = S->CallistoSpecPreset;
		GBakedPresets.MultiLobeSpec   = S->MultiLobeSpecPreset;
		GBakedPresets.bValid          = true;
	}
	ShaderShiftLive::Disarm();   // park the focus lane at the 1.0 sentinel
	PushLiveSlots();             // nothing dirty -> slot channel parks too
}

void RevertLiveKnobs()
{
	if (UShaderShiftSettings* S = UShaderShiftSettings::Get())
	{
		for (const FShaderShiftLiveKnobDef& K : GShaderShiftLiveKnobs)
		{
			if (const float* Baked = GBakedSnapshot.Find(K.FocusId))
			{
				K.Set(*S, *Baked);
			}
		}
		if (GBakedPresets.bValid)
		{
			S->CallistoDiffusePreset = GBakedPresets.CallistoDiffuse;
			S->CallistoSpecPreset    = GBakedPresets.CallistoSpec;
			S->MultiLobeSpecPreset   = GBakedPresets.MultiLobeSpec;
		}
	}
	ShaderShiftLive::Disarm();
	PushLiveSlots();             // Settings == snapshot again -> parks
}

void InitLiveDisarmed()
{
	ShaderShiftLive::InitDisarmed();
	// At startup Settings hold the last-baked values (unbaked previews are
	// deliberately not persisted), so seeding here yields zero dirty knobs.
	ReseedBakedSnapshot();
}

void PushLiveFloat(int32 LiveParamId, float Value)
{
	// Focus lane: this knob at full precision while it is being dragged.
	ShaderShiftLive::SetArmedFloat(LiveParamId, Value);
	// Slot lanes: everything else that's dirty keeps previewing alongside.
	PushLiveSlots();
}

bool IsAnyHookActive()
{
	FShaderShiftHookRegistry& Registry = FShaderShiftHookRegistry::Get();
	TArray<const FShaderShiftFileHook*> Hooks;
	Registry.GetAllHooks(Hooks);
	for (const FShaderShiftFileHook* Hook : Hooks)
	{
		if (Hook && Hook->IsApplied()) return true;
	}
	return false;
}

bool IsHookApplied(const TCHAR* VirtualPath)
{
	FShaderShiftHookRegistry& Registry = FShaderShiftHookRegistry::Get();
	const FShaderShiftFileHook* Hook = Registry.FindHook(VirtualPath);
	return Hook && Hook->IsApplied();
}

void ResetSettingsToEngineDefaults(UShaderShiftSettings* Settings)
{
	if (!Settings)
	{
		return;
	}

	// Reset every user-facing setting to its engine-default value.
	// All mode enums use the integer 0 to mean "engine default",
	// AgxLook 0 is its neutral preset, and bPreserveEmissiveColor
	// off matches stock UE behaviour.  Without this reset the
	// dropdowns kept showing whatever the user had selected, and
	// the next PostEditChangeProperty would re-apply the old values
	// onto the engine shaders we just reverted - which made the
	// Restore button look like it did nothing.
	Settings->TonemapMode            = ECustomTonemapMode::ACES_Filmic;
	Settings->AgxLook                = EAgxLook::Default;
	Settings->DiffuseMode            = ECustomDiffuseMode::EngineDefault;
	Settings->SpecularMode           = ECustomSpecMode::EngineDefault;
	Settings->CallistoDiffusePreset  = ECallistoDiffusePreset::Custom;
	Settings->ProximaAlphaScale      = 1.0f;
	Settings->CallistoSpecPreset     = ECallistoSpecPreset::Custom;
	Settings->MultiLobeSpecPreset    = EMultiLobeSpecPreset::Custom;

	// Per-knob BRDF tunables -> their baked ini defaults.
	Settings->CallistoDiffFresnel            = 1.0f;
	Settings->CallistoDiffFresnelFalloff     = 0.75f;
	Settings->CallistoDiffFresnelTanFalloff  = 0.75f;
	Settings->CallistoDiffFresnelTintR       = 1.0f;
	Settings->CallistoDiffFresnelTintG       = 1.0f;
	Settings->CallistoDiffFresnelTintB       = 1.0f;
	Settings->CallistoRetro                  = 1.0f;
	Settings->CallistoRetroFalloff           = 0.75f;
	Settings->CallistoRetroTanFalloff        = 0.75f;
	Settings->CallistoRetroTintR             = 1.0f;
	Settings->CallistoRetroTintG             = 1.0f;
	Settings->CallistoRetroTintB             = 1.0f;
	Settings->CallistoSmoothTerm             = 0.0f;
	Settings->CallistoSmoothTermLength       = 0.5f;
	Settings->CallistoSmoothTermTintR        = 1.0f;
	Settings->CallistoSmoothTermTintG        = 1.0f;
	Settings->CallistoSmoothTermTintB        = 1.0f;
	Settings->CallistoSpecFresnelFalloff     = 1.0f;
	Settings->CallistoDualSpecRoughnessScale = 2.0f;
	Settings->CallistoDualSpecOpacity        = 0.0f;
	Settings->SpecPrimaryWeight              = 0.7f;
	Settings->SpecPrimaryRoughnessScale      = 0.45f;
	Settings->SpecSecondaryRoughnessScale    = 2.2f;
	Settings->SpecSecondaryMaxRoughness      = 0.95f;
	Settings->RegolithCoverage               = 0.35f;
	Settings->RegolithGrainRoughness         = 0.6f;
	Settings->RegolithRetro                  = 0.8f;
	Settings->RegolithSheen                  = 0.6f;
	Settings->RegolithTintR                  = 1.0f;
	Settings->RegolithTintG                  = 1.0f;
	Settings->RegolithTintB                  = 1.0f;
	Settings->RegolithTintBlend              = 0.0f;
	Settings->BloomMode              = ECustomBloomMode::EngineDefault;
	Settings->bPreserveEmissiveColor = false;
	Settings->AOMode                 = ECustomAOMode::EngineDefault;
	Settings->SSGIMode               = ECustomSSGIMode::EngineDefault;
	Settings->SSGIQuality            = ECustomSSGIQuality::x1;
	Settings->SSILVBIntensity        = 2.0f;

	// Volumetric Fog -- mirror the in-class initialisers on UShaderShiftSettings.
	// Defaults match the engine baseline so the patched VolumetricFog.usf
	// compiles to byte-equivalent output of the stock 5.7 shader.
	Settings->VolPhaseMode           = ECustomVolPhaseMode::EngineDefault;
	Settings->VolSelfShadow          = ECustomVolSelfShadow::Off;
	Settings->VolMultiScatter        = ECustomVolMultiScatter::Off;
	Settings->VolSpectralStrength    = 0.0f;
	Settings->VolPhaseG2             = -0.3f;
	Settings->VolPhaseBlend          = 0.3f;
	Settings->VolMieDropletDiameter  = 15.0f;
	Settings->VolPowderBlend         = 0.0f;
	Settings->bVolBlueNoise          = false;

	// ShaderShift v1.1 toggles - mirror the in-class initialisers
	// on UShaderShiftSettings.
	Settings->bFilmHalation                     = false;
	Settings->HalationThreshold                 = 0.8f;
	Settings->HalationStrength                  = 0.25f;
	Settings->HalationSpread                    = 1.0f;
	Settings->bOrganicGrain                     = false;
	Settings->GrainStrength                     = 0.12f;
	Settings->GrainScale                        = 1.0f;
	Settings->bTaaGhostingTweak                 = false;
	Settings->TaaClampFactor                    = 1.0f;
	Settings->bTaaSharpen                       = false;
	Settings->TaaSharpenStrength                = 0.4f;
	Settings->bRestirGIProbes                   = false;
	Settings->RestirStrength                    = 0.5f;
	Settings->RestirSearchRadius                = 1.0f;
	Settings->bGradientReflectionFilter         = false;
	Settings->GradientReflectionStrength        = 0.6f;
	Settings->GradientReflectionRoughnessBias   = 1.0f;
	Settings->GradientReflectionSharpTrace      = 0.5f;
	Settings->bContactAwareShadows              = false;
	Settings->ContactShadowHardening            = 0.5f;
	Settings->ContactShadowSoftness             = 1.0f;
}

} // namespace ShaderShiftQuickPanel

// ----------------------------------------------------------------------------
// SShaderShiftPanelTab
// ----------------------------------------------------------------------------

FShaderShiftEnumOptionPtr SShaderShiftPanelTab::FindOption(
	const TArray<FShaderShiftEnumOptionPtr>& Options, uint8 Value)
{
	for (const FShaderShiftEnumOptionPtr& Opt : Options)
	{
		if (Opt->Value == Value) return Opt;
	}
	return Options.Num() > 0 ? Options[0] : nullptr;
}

TSharedRef<SWidget> SShaderShiftPanelTab::GenerateComboItem(FShaderShiftEnumOptionPtr Item)
{
	return SNew(STextBlock)
		.Text(Item->DisplayName)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		.Margin(FMargin(4, 2));
}

TSharedRef<SWidget> SShaderShiftPanelTab::MakeSectionHeader(
	const FText& Title,
	const FText& Subtitle,
	const TCHAR* HookVirtualPath)
{
	const FLinearColor AccentColor = FShaderShiftPanelStyle::AccentColor;
	const FLinearColor DimColor    = FShaderShiftPanelStyle::DimTextColor;

	return SNew(SHorizontalBox)

		// Status indicator circle
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0, 0, 8, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("\u2713")))  // checkmark
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			.ColorAndOpacity_Lambda(
				[HookVirtualPath, AccentColor, DimColor]()
			{
				return ShaderShiftQuickPanel::IsHookApplied(HookVirtualPath)
					? FSlateColor(AccentColor)
					: FSlateColor(DimColor);
			})
		]

		// Title + subtitle
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(Title)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(Subtitle)
				.Font(FCoreStyle::GetDefaultFontStyle("Italic", 7))
				.ColorAndOpacity(DimColor)
			]
		]

		// Applied/Default label
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([HookVirtualPath]()
			{
				return ShaderShiftQuickPanel::IsHookApplied(HookVirtualPath)
					? LOCTEXT("HookPatched", "Patched")
					: LOCTEXT("HookDefault", "Engine Default");
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
			.ColorAndOpacity_Lambda(
				[HookVirtualPath, AccentColor, DimColor]()
			{
				return ShaderShiftQuickPanel::IsHookApplied(HookVirtualPath)
					? FSlateColor(AccentColor)
					: FSlateColor(DimColor);
			})
		];
}

TSharedRef<SWidget> SShaderShiftPanelTab::MakeComboRow(
	const FText& Label,
	TArray<FShaderShiftEnumOptionPtr>* Options,
	FShaderShiftEnumOptionPtr* CurrentSelection,
	TFunction<void(FShaderShiftEnumOptionPtr, ESelectInfo::Type)> OnChanged)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(FShaderShiftPanelStyle::LabelWidth)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(FShaderShiftPanelStyle::DimTextColor)
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		[
			SNew(SComboBox<FShaderShiftEnumOptionPtr>)
			.OptionsSource(Options)
			.InitiallySelectedItem(*CurrentSelection)
			.OnSelectionChanged_Lambda(MoveTemp(OnChanged))
			.OnGenerateWidget_Static(&SShaderShiftPanelTab::GenerateComboItem)
			.Content()
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.Text_Lambda([CurrentSelection]()
				{
					return (*CurrentSelection).IsValid()
						? (*CurrentSelection)->DisplayName
						: FText::GetEmpty();
				})
			]
		];
}

#undef LOCTEXT_NAMESPACE
