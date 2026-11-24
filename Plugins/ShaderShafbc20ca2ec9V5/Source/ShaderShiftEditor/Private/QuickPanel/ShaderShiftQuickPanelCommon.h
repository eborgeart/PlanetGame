// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.
//
// ============================================================================
// ShaderShift Quick Panel - shared building blocks
//
// Everything the per-tab section widgets have in common lives here:
//   - FShaderShiftPanelStyle : the panel's colors / metric constants, so every
//     tab renders with the exact same look as the original monolithic panel.
//   - FShaderShiftEnumOption : the combo-box option payload (enum value + text).
//   - SShaderShiftPanelTab   : base class for one tab's content widget.  Owns
//     the Settings pointer and provides the row builders (MakeSectionHeader /
//     MakeComboRow / MakeBoolRow / MakeFloatRow / MakeCheckboxRow) that were
//     previously private members of the monolithic SShaderShiftQuickPanel.
//
// Tabs derive from SShaderShiftPanelTab, build their sections in Construct(),
// and implement RefreshFromSettings() so the shell can re-point combo
// selections after "Restore Engine Defaults" resets the UObject.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"                    // UEnum
#include "UObject/ReflectedTypeAccessors.h"   // StaticEnum<T>
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

class UShaderShiftSettings;

// ----------------------------------------------------------------------------
// Style constants -- single source of truth for the Quick Panel look & feel.
// Values are byte-identical to the original monolithic panel's locals.
// ----------------------------------------------------------------------------
struct FShaderShiftPanelStyle
{
	static const FLinearColor AccentColor;      // light blue accent
	static const FLinearColor DimTextColor;
	static const FLinearColor BrightTextColor;
	static const FLinearColor PanelBgColor;
	static const FLinearColor SectionBgColor;
	static const FLinearColor ButtonBgColor;

	static constexpr float PanelWidth = 420.f;
	static constexpr float ComboWidth = 240.f;
	static constexpr float LabelWidth = 120.f;
};

// ----------------------------------------------------------------------------
// Combo-box option payload.
// ----------------------------------------------------------------------------
struct FShaderShiftEnumOption
{
	uint8  Value;
	FText  DisplayName;
};
using FShaderShiftEnumOptionPtr = TSharedPtr<FShaderShiftEnumOption>;

// ----------------------------------------------------------------------------
// Free helpers shared by the shell and the tabs.
// ----------------------------------------------------------------------------
namespace ShaderShiftQuickPanel
{
	// --- Live float channel bridge -------------------------------------------
	// Thin non-template wrappers so the header-inline row builders can drive
	// the ShaderShiftLive cvar channel without pulling its (uniform-buffer-
	// heavy) header into every translation unit.  Implemented in the .cpp.

	/** Startup: park the focus lane at the 1.0 stock sentinel, seed the baked
	    snapshot from current Settings (nothing dirty), and park the slot lanes. */
	void InitLiveDisarmed();

	/**
	 * Live drag: arm the focus lane with this knob at FULL precision (instant),
	 * and repack the slot lanes so every other dirty knob keeps previewing
	 * simultaneously at 10-bit.  No recompile for any of it.
	 */
	void PushLiveFloat(int32 LiveParamId, float Value);

	/** Repack + publish the slot lanes from UShaderShiftSettings vs the baked
	    snapshot (dirty knobs quantized, clean knobs = 1023 -> baked default).
	    Call after any programmatic Settings change (e.g. preset seeding). */
	void PushLiveSlots();

	/**
	 * Baked snapshot = the knob values currently compiled into the #defines.
	 * dirty knob == Settings value differs from the snapshot == it is being
	 * live-previewed and needs a Bake to persist.
	 */
	int32 GetLiveDirtyCount();

	/** "Live banks: <diffuse mode> diffuse \u00b7 <spec mode> spec" -- which
	    mode groups currently own the shared BRDF bank lanes (status strip). */
	FText GetLiveBankOwnersText();
	bool  IsKnobDirty(int32 LiveParamId);

	/** Re-seed the snapshot from current Settings (call after any bake/apply
	    path) -- everything becomes clean; focus + slot lanes park. */
	void ReseedBakedSnapshot();

	/** Discard the live preview: restore every knob's Settings value from the
	    baked snapshot and park the lanes.  Caller refreshes the tab widgets. */
	void RevertLiveKnobs();

	/** True if ANY registered hook is currently applied to the engine install. */
	bool IsAnyHookActive();

	/** True if the hook registered for VirtualPath is currently applied. */
	bool IsHookApplied(const TCHAR* VirtualPath);

	/**
	 * Reset every user-facing UShaderShiftSettings property to its
	 * engine-default value (the in-class initialisers).  Used by the shell's
	 * "Restore Engine Defaults" button; the shell then calls
	 * RefreshFromSettings() on every tab so combo selections re-point.
	 * Does NOT SaveConfig() -- the caller decides when to persist.
	 */
	void ResetSettingsToEngineDefaults(UShaderShiftSettings* Settings);
}

// ----------------------------------------------------------------------------
// SShaderShiftPanelTab -- base class for one tab's content.
// ----------------------------------------------------------------------------
class SShaderShiftPanelTab : public SCompoundWidget
{
public:
	/**
	 * Re-read all displayed state from the (possibly externally reset)
	 * settings object.  Combo rows render via Text_Lambda on the
	 * Current*Option pointers, so re-pointing them here is sufficient;
	 * bool/float rows read the settings object directly every frame.
	 */
	virtual void RefreshFromSettings() = 0;

protected:
	UShaderShiftSettings* Settings = nullptr;

	// --- Enum option helpers -------------------------------------------------

	template<typename TEnum>
	static void BuildEnumOptions(TArray<FShaderShiftEnumOptionPtr>& OutOptions)
	{
		const UEnum* EnumType = StaticEnum<TEnum>();
		check(EnumType);

		for (int32 i = 0; i < EnumType->NumEnums() - 1; ++i) // -1 to skip _MAX
		{
			FShaderShiftEnumOptionPtr Opt = MakeShared<FShaderShiftEnumOption>();
			Opt->Value       = static_cast<uint8>(i);
			Opt->DisplayName = EnumType->GetDisplayNameTextByIndex(i);
			OutOptions.Add(Opt);
		}
	}

	static FShaderShiftEnumOptionPtr FindOption(
		const TArray<FShaderShiftEnumOptionPtr>& Options, uint8 Value);

	static TSharedRef<SWidget> GenerateComboItem(FShaderShiftEnumOptionPtr Item);

	// --- Section header with hook status icon --------------------------------
	//
	// Identical layout to the original: checkmark glyph + bold title + italic
	// shader-file subtitle + "Patched"/"Engine Default" status on the right,
	// all tinted live by whether the hook is applied.

	static TSharedRef<SWidget> MakeSectionHeader(
		const FText& Title,
		const FText& Subtitle,
		const TCHAR* HookVirtualPath);

	// --- Combo row builder ----------------------------------------------------
	//
	// Same layout as the original MakeComboRow.  The original bound a member
	// callback via OnSelectionChanged_Raw(Widget, Callback); tabs now pass a
	// TFunction (usually a [this] lambda) which removes the template-widget
	// coupling without changing behaviour.

	static TSharedRef<SWidget> MakeComboRow(
		const FText& Label,
		TArray<FShaderShiftEnumOptionPtr>* Options,
		FShaderShiftEnumOptionPtr* CurrentSelection,
		TFunction<void(FShaderShiftEnumOptionPtr, ESelectInfo::Type)> OnChanged);

	// --- Bool checkbox row builder --------------------------------------------

	template<typename TBoolGetter, typename TBoolSetter>
	static TSharedRef<SWidget> MakeBoolRow(
		const FText& Label,
		const FText& ToolTip,
		TBoolGetter Getter,
		TBoolSetter Setter)
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
					.ToolTipText(ToolTip)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity(FShaderShiftPanelStyle::DimTextColor)
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([Getter]()
				{
					return Getter() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([Setter](ECheckBoxState NewState)
				{
					Setter(NewState == ECheckBoxState::Checked);
				})
			];
	}

	// --- Float spin-box row builder --------------------------------------------
	//
	// LiveParamId (optional): when >= 0, dragging this slider previews LIVE with
	// no recompile -- full precision on the focus lane while dragging, and the
	// value keeps previewing (10-bit slot lane) alongside every other dirty knob
	// after release, until Bake persists it into the compile-time #define.
	// The label lights up in the accent colour while the knob is dirty
	// (Settings value != baked snapshot).

	template<typename TFloatGetter, typename TFloatSetter>
	static TSharedRef<SWidget> MakeFloatRow(
		const FText& Label,
		const FText& ToolTip,
		float MinValue,
		float MaxValue,
		TFloatGetter Getter,
		TFloatSetter Setter,
		int32 LiveParamId = INDEX_NONE)
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
					.ToolTipText(ToolTip)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					.ColorAndOpacity_Lambda([LiveParamId]()
					{
						// dirty == live-previewing an unbaked value
						return (LiveParamId >= 0 && ShaderShiftQuickPanel::IsKnobDirty(LiveParamId))
							? FSlateColor(FShaderShiftPanelStyle::AccentColor)
							: FSlateColor(FShaderShiftPanelStyle::DimTextColor);
					})
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(SSpinBox<float>)
				.MinValue(MinValue)
				.MaxValue(MaxValue)
				.MinSliderValue(MinValue)
				.MaxSliderValue(MaxValue)
				.Value_Lambda(Getter)
				.OnValueChanged_Lambda([Setter, LiveParamId](float V)
				{
					Setter(V);
					if (LiveParamId >= 0)
					{
						ShaderShiftQuickPanel::PushLiveFloat(LiveParamId, V);
					}
				})
				.OnValueCommitted_Lambda([Setter, LiveParamId](float V, ETextCommit::Type)
				{
					Setter(V);
					if (LiveParamId >= 0)
					{
						ShaderShiftQuickPanel::PushLiveFloat(LiveParamId, V);
					}
				})
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
			];
	}
};
