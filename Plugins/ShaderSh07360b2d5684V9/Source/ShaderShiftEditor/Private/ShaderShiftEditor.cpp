// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "ShaderShiftEditor.h"
#include "ShaderShiftSettings.h"
#include "ShaderShift.h"

#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

// Viewport redraw after recompileshaders
#include "Containers/Ticker.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "EditorSupportDelegates.h"
#include "ShaderCompiler.h"
#include "RenderingThread.h"
#include "GlobalShader.h"
#include "RendererInterface.h"
#include "EngineModule.h"

// Detail customization for the Recompile button
#include "IDetailCustomization.h"
#include "ISettingsModule.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "PropertyEditorModule.h"

// Toolbar extension
#include "ToolMenus.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"

// Source control - used by the persist-mode workflow to check out engine
// shader files into the default changelist when the user is on a source
// engine build (Perforce / Plastic).  Degrades to a no-op when SCC is
// disabled in the project.
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "ISourceControlState.h"
#include "SourceControlOperations.h"

#define LOCTEXT_NAMESPACE "FShaderShiftEditorModule"

DEFINE_LOG_CATEGORY_STATIC(LogShaderShiftEditor, Log, All);

namespace
{
	/**
	 * If persist mode is active and source control is enabled, ensure the
	 * registered engine shader files are checked out into the user's default
	 * changelist.  Required for source-engine builds where engine shaders are
	 * tracked in P4/Plastic - without this, our writes hit read-only files or
	 * leave the user with modified-but-not-checked-out files that source
	 * control can't reconcile.
	 *
	 * No-ops when:
	 *   - persist is OFF and we're not in a cook commandlet
	 *   - SCC is disabled in the project
	 *   - none of our registered hooks have files on disk yet
	 *   - all relevant files are already checked out / added / under our own user
	 *
	 * Files that are simply not under SCC at all are silently skipped (the
	 * common case for users on a launcher engine install).
	 */
	void CheckOutPatchedShadersIfPersist()
	{
		if (!FShaderShiftModule::ShouldPersistShaderChanges())
		{
			return;
		}

		ISourceControlModule& SCCModule = ISourceControlModule::Get();
		if (!SCCModule.IsEnabled())
		{
			UE_LOG(LogShaderShiftEditor, Verbose,
				TEXT("Persist + SCC: source control not enabled - skipping checkout"));
			return;
		}

		ISourceControlProvider& Provider = SCCModule.GetProvider();
		if (!Provider.IsAvailable())
		{
			UE_LOG(LogShaderShiftEditor, Warning,
				TEXT("Persist + SCC: provider not available (login state, network?) - "
					 "skipping checkout"));
			return;
		}

		// Gather every registered hook's physical engine shader path.
		FShaderShiftHookRegistry& Registry = FShaderShiftHookRegistry::Get();
		TArray<const FShaderShiftFileHook*> AllHooks;
		Registry.GetAllHooks(AllHooks);

		TArray<FString> CandidatePaths;
		CandidatePaths.Reserve(AllHooks.Num());
		for (const FShaderShiftFileHook* Hook : AllHooks)
		{
			if (!Hook)
			{
				continue;
			}
			const FString PhysicalPath = Hook->GetPhysicalPath();
			if (FPaths::FileExists(PhysicalPath))
			{
				CandidatePaths.Add(FPaths::ConvertRelativePathToFull(PhysicalPath));
			}
		}

		if (CandidatePaths.Num() == 0)
		{
			return;
		}

		// Force-update SCC state for these specific files so we don't act on
		// stale cached state from a previous session.
		TArray<FSourceControlStateRef> States;
		const ECommandResult::Type StateResult = Provider.GetState(
			CandidatePaths, States, EStateCacheUsage::ForceUpdate);

		if (StateResult != ECommandResult::Succeeded)
		{
			UE_LOG(LogShaderShiftEditor, Warning,
				TEXT("Persist + SCC: failed to query state for %d engine shader file(s)"),
				CandidatePaths.Num());
			return;
		}

		TArray<FString> FilesToCheckOut;
		FilesToCheckOut.Reserve(States.Num());

		int32 NotUnderSCC      = 0;
		int32 AlreadyCheckedOut = 0;
		int32 OtherUserOpened  = 0;

		for (int32 i = 0; i < States.Num(); ++i)
		{
			const FSourceControlStateRef& State = States[i];

			if (!State->IsSourceControlled())
			{
				++NotUnderSCC;
				continue;
			}
			if (State->IsCheckedOut() || State->IsAdded())
			{
				++AlreadyCheckedOut;
				continue;
			}
			if (State->IsCheckedOutOther())
			{
				++OtherUserOpened;
				UE_LOG(LogShaderShiftEditor, Warning,
					TEXT("Persist + SCC: %s is checked out by another user - "
						 "patches will not be writable"),
					*CandidatePaths[i]);
				continue;
			}

			FilesToCheckOut.Add(CandidatePaths[i]);
		}

		if (FilesToCheckOut.Num() > 0)
		{
			const ECommandResult::Type CheckoutResult = Provider.Execute(
				ISourceControlOperation::Create<FCheckOut>(),
				FilesToCheckOut);

			if (CheckoutResult == ECommandResult::Succeeded)
			{
				UE_LOG(LogShaderShiftEditor, Log,
					TEXT("Persist + SCC: checked out %d engine shader file(s) into default changelist"),
					FilesToCheckOut.Num());
			}
			else
			{
				UE_LOG(LogShaderShiftEditor, Warning,
					TEXT("Persist + SCC: checkout failed for %d engine shader file(s) "
						 "- writes may hit read-only files"),
					FilesToCheckOut.Num());
			}
		}
		else
		{
			UE_LOG(LogShaderShiftEditor, Verbose,
				TEXT("Persist + SCC: nothing to do (not-tracked: %d, already-open: %d, other-user: %d)"),
				NotUnderSCC, AlreadyCheckedOut, OtherUserOpened);
		}
	}
}

// ============================================================================
// FShaderShiftSettingsDetails - IDetailCustomization
//
// Adds a "Recompile Shaders Now" button to the Project Settings panel.
// UFUNCTION(CallInEditor) does not work on UDeveloperSettings because the
// Project Settings details panel does not use the Actor/Component details
// path that invokes CallInEditor functions.
// ============================================================================

class FShaderShiftSettingsDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FShaderShiftSettingsDetails);
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(
			TEXT("Recompilation"),
			LOCTEXT("RecompilationCategory", "Recompilation"));

		Category.AddCustomRow(LOCTEXT("RecompileRow", "Recompile Shaders"))
			.NameContent()
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(LOCTEXT("RecompileLabel", "Manual Recompile"))
			]
			.ValueContent()
			.MaxDesiredWidth(200.f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(6, 2))
				.Text(LOCTEXT("RecompileButton", "Recompile Shaders Now"))
				.OnClicked_Lambda([]() -> FReply
				{
					if (UShaderShiftSettings* Settings = UShaderShiftSettings::Get())
					{
						Settings->RecompileShadersNow();
					}
					return FReply::Handled();
				})
			];
	}
};

// ============================================================================
// SShaderShiftQuickPanel
//
// The Quick Panel itself now lives in self-contained per-category components
// under Private/QuickPanel/ (shell + Color / Shading / GI / Fog / AA / System
// tabs).  See QuickPanel/SShaderShiftQuickPanel.h.
// ============================================================================

#include "QuickPanel/SShaderShiftQuickPanel.h"

// Bridge for the Quick Panel's System tab: expose the file-local persist
// checkout helper through the module's public static interface.
void FShaderShiftEditorModule::CheckOutPatchedShadersForPersist()
{
	CheckOutPatchedShadersIfPersist();
}

// ============================================================================
// Toolbar registration helpers
// ============================================================================

static const FName ShaderShiftTabName("ShaderShiftQuickPanel");
static TWeakPtr<SDockTab> GShaderShiftQuickTab;

static void SpawnQuickPanel()
{
	if (!FGlobalTabmanager::Get()->HasTabSpawner(ShaderShiftTabName))
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ShaderShiftTabName, FOnSpawnTab::CreateLambda([](const FSpawnTabArgs& Args)
		{
			TSharedRef<SDockTab> Tab = SNew(SDockTab)
				.TabRole(ETabRole::NomadTab)[
					SNew(SShaderShiftQuickPanel)
				];

			GShaderShiftQuickTab = Tab;
			return Tab;
		}))
		.SetDisplayName(LOCTEXT("QuickWindowTitle", "ShaderShift"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	}

	FGlobalTabmanager::Get()->TryInvokeTab(ShaderShiftTabName);
}

static void RegisterToolbarButton()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (!ToolMenus)
	{
		return;
	}

	UToolMenu* Menu = ToolMenus->ExtendMenu(
		"LevelEditor.LevelEditorToolBar.PlayToolBar");
	if (!Menu)
	{
		return;
	}

	FToolMenuSection& Section = Menu->FindOrAddSection("ShaderShift");

	Section.AddEntry(FToolMenuEntry::InitToolBarButton(
		"ShaderShiftQuickOptions",
		FUIAction(FExecuteAction::CreateStatic(&SpawnQuickPanel)),
		LOCTEXT("ToolbarLabel", "ShaderShift"),
		LOCTEXT("ToolbarTooltip",
			"Open ShaderShift Quick Options - switch tonemapper and BRDF settings"),
		FSlateIcon(FName("ShaderShiftStyle"),
			"ShaderShift.ToolbarIcon")//"LevelEditor.GameSettings")
	));
}

static void UnregisterToolbarButton()
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (ToolMenus)
	{
		ToolMenus->RemoveSection(
			"LevelEditor.LevelEditorToolBar.PlayToolBar",
			"ShaderShift");
	}
}

// ============================================================================
// UShaderShiftSettings
// ============================================================================

UShaderShiftSettings::UShaderShiftSettings()
{
}

UShaderShiftSettings* UShaderShiftSettings::Get()
{
	return GetMutableDefault<UShaderShiftSettings>();
}

void UShaderShiftSettings::ApplyCallistoDiffusePresetToKnobs()
{
	if (CallistoDiffusePreset == ECallistoDiffusePreset::Custom) { return; }
	struct FCallistoDiffuse { float F, Ff, Ft, R, Rf, Rt, S, Sl; };
	static const FCallistoDiffuse Diff[] =
	{                  //  rho_f  n_f    m_f    rho_r  n_r    m_r    o      p
		/* Custom       */ { 1.0f, 0.75f, 0.75f, 1.0f, 0.75f, 0.75f, 0.0f,  0.5f },
		/* Neutral      */ { 1.0f, 0.75f, 0.75f, 1.0f, 0.75f, 0.75f, 0.0f,  0.5f },
		/* PaperDefault */ { 2.0f, 0.75f, 0.75f, 2.0f, 0.75f, 0.75f, 0.4f,  0.5f },
		/* SoftSkin     */ { 1.3f, 0.80f, 0.80f, 1.7f, 0.80f, 0.80f, 0.6f,  0.6f },
		/* VelvetSheen  */ { 3.0f, 0.85f, 0.85f, 4.0f, 0.85f, 0.85f, 0.25f, 0.5f },
		/* LunarMatte   */ { 1.0f, 0.75f, 0.75f, 6.0f, 0.70f, 0.70f, 0.0f,  0.5f },
		/* WaxyPlush    */ { 4.0f, 0.80f, 0.80f, 2.0f, 0.80f, 0.80f, 0.8f,  0.7f },
	};
	const FCallistoDiffuse& P = Diff[FMath::Clamp((int32)CallistoDiffusePreset, 0, (int32)UE_ARRAY_COUNT(Diff) - 1)];
	CallistoDiffFresnel           = P.F;
	CallistoDiffFresnelFalloff    = P.Ff;
	CallistoDiffFresnelTanFalloff = P.Ft;
	CallistoRetro                 = P.R;
	CallistoRetroFalloff          = P.Rf;
	CallistoRetroTanFalloff       = P.Rt;
	CallistoSmoothTerm            = P.S;
	CallistoSmoothTermLength      = P.Sl;
}

void UShaderShiftSettings::ApplyCallistoSpecPresetToKnobs()
{
	if (CallistoSpecPreset == ECallistoSpecPreset::Custom) { return; }
	struct FCallistoSpec { float Opacity, RoughScale, FresFalloff; };
	static const FCallistoSpec Spec[] =
	{                //  opacity rough_x  n_s
		/* Custom     */ { 0.0f,  2.0f,   1.0f },
		/* SingleLobe */ { 0.0f,  2.0f,   1.0f },
		/* SoftCoat   */ { 0.20f, 2.5f,   0.5f },
		/* GlossyDual */ { 0.35f, 2.0f,   0.6f },
		/* SatinSheen */ { 0.15f, 4.0f,   0.4f },
	};
	const FCallistoSpec& P = Spec[FMath::Clamp((int32)CallistoSpecPreset, 0, (int32)UE_ARRAY_COUNT(Spec) - 1)];
	CallistoDualSpecOpacity        = P.Opacity;
	CallistoDualSpecRoughnessScale = P.RoughScale;
	CallistoSpecFresnelFalloff     = P.FresFalloff;
}

void UShaderShiftSettings::ApplyMultiLobePresetToKnobs()
{
	if (MultiLobeSpecPreset == EMultiLobeSpecPreset::Custom) { return; }
	struct FMultiLobe { float Weight, PrimRough, SecRough, SecMax; };
	static const FMultiLobe Lobe[] =
	{                  //  weight prim_x sec_x  sec_max
		/* Custom       */ { 0.7f, 0.45f, 2.2f, 0.95f },
		/* SingleLobe   */ { 1.0f, 1.0f,  2.0f, 0.95f },
		/* Cinematic    */ { 0.8f, 0.85f, 2.5f, 0.95f },
		/* CarPaint     */ { 0.6f, 0.6f,  3.0f, 0.90f },
		/* SoftSkinSpec */ { 0.7f, 1.0f,  2.0f, 0.80f },
	};
	const FMultiLobe& P = Lobe[FMath::Clamp((int32)MultiLobeSpecPreset, 0, (int32)UE_ARRAY_COUNT(Lobe) - 1)];
	SpecPrimaryWeight           = P.Weight;
	SpecPrimaryRoughnessScale   = P.PrimRough;
	SpecSecondaryRoughnessScale = P.SecRough;
	SpecSecondaryMaxRoughness   = P.SecMax;
}


void UShaderShiftSettings::SyncEnumsToDefines()
{
	TMap<FString, FString> EnumValues;
	EnumValues.Add(TEXT("CUSTOM_TONEMAP_MODE"),         FString::FromInt(static_cast<int32>(TonemapMode)));
	EnumValues.Add(TEXT("AGX_LOOK"),                    FString::FromInt(static_cast<int32>(AgxLook)));
	EnumValues.Add(TEXT("CUSTOM_DIFFUSE_MODE"),         FString::FromInt(static_cast<int32>(DiffuseMode)));
	EnumValues.Add(TEXT("CUSTOM_SPEC_MODE"),            FString::FromInt(static_cast<int32>(SpecularMode)));

	// Live BRDF test knob (ShadingModels.ush) -- real-time diffuse-intensity
	// multiplier.  Live preview rides the View tweak lane; this baked value is
	// the fallback when disarmed / cooked.  SanitizeFloat keeps it locale-safe.
	EnumValues.Add(TEXT("CUSTOM_BLOOM_MODE"),           FString::FromInt(static_cast<int32>(BloomMode)));
	EnumValues.Add(TEXT("CUSTOM_BLOOM_PRESERVE_COLOR"), bPreserveEmissiveColor ? TEXT("1") : TEXT("0"));

	// Ambient Occlusion -- drives CUSTOM_AO_MODE in PostProcessAmbientOcclusion.usf.
	// New AO methods extend ECustomAOMode and add #if CUSTOM_AO_MODE == N branches
	// in the shader template; no other wiring needs to change.
	EnumValues.Add(TEXT("CUSTOM_AO_MODE"),              FString::FromInt(static_cast<int32>(AOMode)));

	// Screen-Space GI -- drives CUSTOM_SSGI_MODE in SSRTDiffuseIndirect.usf.
	// Same extensibility pattern as AOMode.
	EnumValues.Add(TEXT("CUSTOM_SSGI_MODE"),            FString::FromInt(static_cast<int32>(SSGIMode)));

	// Stable SSGI quality multiplier -- the enum's index is the multiplier value
	// itself (x1=0->1, x2=1->2, x4=2->4, x8=3->8).  Map via a small table so the
	// enum-as-int doesn't accidentally become the multiplier.
	{
		static const int32 SSGIQualityToMult[] = { 1, 2, 4, 8 };
		const int32 Idx = FMath::Clamp(static_cast<int32>(SSGIQuality), 0, 3);
		EnumValues.Add(TEXT("CUSTOM_SSGI_RAY_MULTIPLIER"), FString::FromInt(SSGIQualityToMult[Idx]));
	}

	// SSILVB GI intensity scalar -- drives CUSTOM_SSGI_INTENSITY in
	// SSRTDiffuseIndirect.usf.  SanitizeFloat keeps the literal form locale-safe
	// (always uses '.' as decimal separator) so the macro substitution drops a
	// valid HLSL float into the patched shader.
	EnumValues.Add(TEXT("CUSTOM_SSGI_INTENSITY"),       FString::SanitizeFloat(SSILVBIntensity));

	// Volumetric Fog — enum + numeric tunables.
	// SanitizeFloat keeps a stable text form that the shader macro substitution
	// can drop directly into the patched VolumetricFog.usf without further
	// formatting (e.g., "0.3" not "0.300000").
	EnumValues.Add(TEXT("CUSTOM_VOL_PHASE_MODE"),       FString::FromInt(static_cast<int32>(VolPhaseMode)));

	// Self-shadow tap count -- map the enum's discrete tiers to the actual
	// integer step count the shader uses inside its [unroll]ed loop.
	static const int32 SelfShadowStepCounts[] = { 0, 4, 8, 16 };
	const int32 SelfShadowIdx = FMath::Clamp(static_cast<int32>(VolSelfShadow), 0, 3);
	EnumValues.Add(TEXT("CUSTOM_VOL_SELF_SHADOW_STEPS"), FString::FromInt(SelfShadowStepCounts[SelfShadowIdx]));

	// Multi-scattering octave count -- map enum tier to actual N.
	static const int32 MultiScatterOctaveCounts[] = { 0, 2, 3, 4 };
	const int32 MultiScatterIdx = FMath::Clamp(static_cast<int32>(VolMultiScatter), 0, 3);
	EnumValues.Add(TEXT("CUSTOM_VOL_MS_OCTAVES"), FString::FromInt(MultiScatterOctaveCounts[MultiScatterIdx]));

	// Phase C spectral strength: float passes through directly, plus a
	// derived integer toggle for the #if gate in the shader (HLSL can't
	// compare floats in preprocessor expressions).
	EnumValues.Add(TEXT("CUSTOM_VOL_SPECTRAL_STRENGTH"), FString::SanitizeFloat(VolSpectralStrength));
	EnumValues.Add(TEXT("CUSTOM_USE_SPECTRAL"),          VolSpectralStrength > 0.0f ? TEXT("1") : TEXT("0"));
	EnumValues.Add(TEXT("CUSTOM_VOL_PHASE_G2"),         FString::SanitizeFloat(VolPhaseG2));
	EnumValues.Add(TEXT("CUSTOM_VOL_PHASE_BLEND"),      FString::SanitizeFloat(VolPhaseBlend));
	EnumValues.Add(TEXT("CUSTOM_VOL_MIE_DROPLET_UM"),   FString::SanitizeFloat(VolMieDropletDiameter));
	EnumValues.Add(TEXT("CUSTOM_VOL_POWDER_BLEND"),     FString::SanitizeFloat(VolPowderBlend));
	// Integer toggle derived from the float blend.  HLSL's preprocessor can't
	// compare floats in #if, so we synthesise a real int for the powder gate.
	EnumValues.Add(TEXT("CUSTOM_USE_POWDER"),            VolPowderBlend > 0.0f ? TEXT("1") : TEXT("0"));
	EnumValues.Add(TEXT("CUSTOM_VOL_BLUE_NOISE"),       bVolBlueNoise ? TEXT("1") : TEXT("0"));

	// ============================================================
	// Film Halation + Organic Grain (PostProcessTonemap.usf)
	// ============================================================
	EnumValues.Add(TEXT("SHADERSHIFT_FILM_HALATION"),  bFilmHalation ? TEXT("1") : TEXT("0"));
	EnumValues.Add(TEXT("CUSTOM_HALATION_THRESHOLD"),  FString::SanitizeFloat(HalationThreshold));
	EnumValues.Add(TEXT("CUSTOM_HALATION_STRENGTH"),   FString::SanitizeFloat(HalationStrength));
	EnumValues.Add(TEXT("CUSTOM_HALATION_SPREAD"),     FString::SanitizeFloat(HalationSpread));

	EnumValues.Add(TEXT("SHADERSHIFT_ORGANIC_GRAIN"),  bOrganicGrain ? TEXT("1") : TEXT("0"));
	EnumValues.Add(TEXT("CUSTOM_GRAIN_STRENGTH"),      FString::SanitizeFloat(GrainStrength));
	EnumValues.Add(TEXT("CUSTOM_GRAIN_SCALE"),         FString::SanitizeFloat(GrainScale));

	// ============================================================
	// TAA tweaks (TemporalAA.usf)
	// ============================================================
	EnumValues.Add(TEXT("SHADERSHIFT_TAA_GHOSTING_TWEAK"), bTaaGhostingTweak ? TEXT("1") : TEXT("0"));
	EnumValues.Add(TEXT("CUSTOM_TAA_CLAMP_FACTOR"),        FString::SanitizeFloat(TaaClampFactor));
	EnumValues.Add(TEXT("SHADERSHIFT_TAA_SHARPEN"),        bTaaSharpen ? TEXT("1") : TEXT("0"));
	EnumValues.Add(TEXT("CUSTOM_TAA_SHARPEN_STRENGTH"),    FString::SanitizeFloat(TaaSharpenStrength));

	// ============================================================
	// Lumen ReSTIR GI Probes (LumenScreenProbeGather.usf)
	//
	// CUSTOM_RESTIR_SEARCH_RADIUS is a true probe-grid spatial search radius
	// (v1.2): the shader adds a 4-tap cross of grid probes this many tiles
	// out and derives the compile-time tap radius directly from the define.
	// (The v1.1 SHADERSHIFT_RESTIR_LINEAR_WEIGHT pow fast-path gate is gone.)
	// ============================================================
	EnumValues.Add(TEXT("SHADERSHIFT_RESTIR_GI_PROBES"),     bRestirGIProbes ? TEXT("1") : TEXT("0"));
	EnumValues.Add(TEXT("CUSTOM_RESTIR_STRENGTH"),           FString::SanitizeFloat(RestirStrength));
	EnumValues.Add(TEXT("CUSTOM_RESTIR_SEARCH_RADIUS"),      FString::SanitizeFloat(RestirSearchRadius));

	// ============================================================
	// Gradient Reflection Filter (LumenReflectionCommon.ush + Denoiser)
	// ============================================================
	EnumValues.Add(TEXT("SHADERSHIFT_GRADIENT_REFLECTION_FILTER"), bGradientReflectionFilter ? TEXT("1") : TEXT("0"));
	EnumValues.Add(TEXT("CUSTOM_GRAD_REFL_STRENGTH"),              FString::SanitizeFloat(GradientReflectionStrength));
	EnumValues.Add(TEXT("CUSTOM_GRAD_REFL_ROUGHNESS_BIAS"),        FString::SanitizeFloat(GradientReflectionRoughnessBias));
	EnumValues.Add(TEXT("CUSTOM_GRAD_REFL_SHARP_TRACE"),           FString::SanitizeFloat(GradientReflectionSharpTrace));

	// ============================================================
	// Contact-Aware VSM Shadows (VirtualShadowMapProjectionFilter.ush)
	// ============================================================
	EnumValues.Add(TEXT("SHADERSHIFT_CONTACT_AWARE_SHADOWS"),  bContactAwareShadows ? TEXT("1") : TEXT("0"));
	EnumValues.Add(TEXT("CUSTOM_SHADOW_HARDENING_STRENGTH"),  FString::SanitizeFloat(ContactShadowHardening));
	EnumValues.Add(TEXT("CUSTOM_SHADOW_SOFTNESS_MULTIPLIER"), FString::SanitizeFloat(ContactShadowSoftness));

	// ============================================================
	// Callisto / Proxima BRDF presets (BRDF.ush)
	//
	// Each preset enum writes a whole group of CALLISTO_* / CUSTOM_SPEC_* values
	// so users get an instant "look" without touching the individual knobs.  The
	// "Custom" entry (index 0) is deliberately NOT handled here: leaving those
	// defines out of EnumValues means the substitution loop below skips them and
	// the user's hand-tuned Project-Settings values flow through unchanged.
	// The targeted defines all exist in Config/ShaderOverride.ini's [BRDF.ush]
	// section, and only take visible effect when their mode is active
	// (#if CUSTOM_DIFFUSE_MODE == 5/6, #if CUSTOM_SPEC_MODE == 1/2).
	// ============================================================

	// -- Seed per-knob tunables from any active preset (no-op when Custom), then
	//    write every knob into its BRDF.ush #define.  The per-knob settings are the
	//    single source of truth; presets just fill them and a slider edit sets its
	//    preset back to Custom so hand-tuning persists.
	ApplyCallistoDiffusePresetToKnobs();
	ApplyCallistoSpecPresetToKnobs();
	ApplyMultiLobePresetToKnobs();

	EnumValues.Add(TEXT("CALLISTO_DIFF_FRESNEL"), FString::SanitizeFloat(CallistoDiffFresnel));
	EnumValues.Add(TEXT("CALLISTO_DIFF_FRESNEL_FALLOFF"), FString::SanitizeFloat(CallistoDiffFresnelFalloff));
	EnumValues.Add(TEXT("CALLISTO_DIFF_FRESNEL_TAN_FALLOFF"), FString::SanitizeFloat(CallistoDiffFresnelTanFalloff));
	EnumValues.Add(TEXT("CALLISTO_DIFF_FRESNEL_TINT_R"), FString::SanitizeFloat(CallistoDiffFresnelTintR));
	EnumValues.Add(TEXT("CALLISTO_DIFF_FRESNEL_TINT_G"), FString::SanitizeFloat(CallistoDiffFresnelTintG));
	EnumValues.Add(TEXT("CALLISTO_DIFF_FRESNEL_TINT_B"), FString::SanitizeFloat(CallistoDiffFresnelTintB));
	EnumValues.Add(TEXT("CALLISTO_RETRO"), FString::SanitizeFloat(CallistoRetro));
	EnumValues.Add(TEXT("CALLISTO_RETRO_FALLOFF"), FString::SanitizeFloat(CallistoRetroFalloff));
	EnumValues.Add(TEXT("CALLISTO_RETRO_TAN_FALLOFF"), FString::SanitizeFloat(CallistoRetroTanFalloff));
	EnumValues.Add(TEXT("CALLISTO_RETRO_TINT_R"), FString::SanitizeFloat(CallistoRetroTintR));
	EnumValues.Add(TEXT("CALLISTO_RETRO_TINT_G"), FString::SanitizeFloat(CallistoRetroTintG));
	EnumValues.Add(TEXT("CALLISTO_RETRO_TINT_B"), FString::SanitizeFloat(CallistoRetroTintB));
	EnumValues.Add(TEXT("CALLISTO_SMOOTH_TERM"), FString::SanitizeFloat(CallistoSmoothTerm));
	EnumValues.Add(TEXT("CALLISTO_SMOOTH_TERM_LENGTH"), FString::SanitizeFloat(CallistoSmoothTermLength));
	EnumValues.Add(TEXT("CALLISTO_SMOOTH_TERM_TINT_R"), FString::SanitizeFloat(CallistoSmoothTermTintR));
	EnumValues.Add(TEXT("CALLISTO_SMOOTH_TERM_TINT_G"), FString::SanitizeFloat(CallistoSmoothTermTintG));
	EnumValues.Add(TEXT("CALLISTO_SMOOTH_TERM_TINT_B"), FString::SanitizeFloat(CallistoSmoothTermTintB));
	EnumValues.Add(TEXT("CALLISTO_SPEC_FRESNEL_FALLOFF"), FString::SanitizeFloat(CallistoSpecFresnelFalloff));
	EnumValues.Add(TEXT("CALLISTO_DUAL_SPEC_ROUGHNESS_SCALE"), FString::SanitizeFloat(CallistoDualSpecRoughnessScale));
	EnumValues.Add(TEXT("CALLISTO_DUAL_SPEC_OPACITY"), FString::SanitizeFloat(CallistoDualSpecOpacity));
	EnumValues.Add(TEXT("CALLISTO_PROXIMA_ALPHA_SCALE"), FString::SanitizeFloat(ProximaAlphaScale));
	EnumValues.Add(TEXT("REGOLITH_COVERAGE"), FString::SanitizeFloat(RegolithCoverage));
	EnumValues.Add(TEXT("REGOLITH_GRAIN_ROUGHNESS"), FString::SanitizeFloat(RegolithGrainRoughness));
	EnumValues.Add(TEXT("REGOLITH_RETRO"), FString::SanitizeFloat(RegolithRetro));
	EnumValues.Add(TEXT("REGOLITH_SHEEN"), FString::SanitizeFloat(RegolithSheen));
	EnumValues.Add(TEXT("REGOLITH_ROUGHNESS_AFFINITY"), FString::SanitizeFloat(RegolithRoughnessAffinity));
	EnumValues.Add(TEXT("REGOLITH_TINT_R"), FString::SanitizeFloat(RegolithTintR));
	EnumValues.Add(TEXT("REGOLITH_TINT_G"), FString::SanitizeFloat(RegolithTintG));
	EnumValues.Add(TEXT("REGOLITH_TINT_B"), FString::SanitizeFloat(RegolithTintB));
	EnumValues.Add(TEXT("REGOLITH_TINT_BLEND"), FString::SanitizeFloat(RegolithTintBlend));
	EnumValues.Add(TEXT("CUSTOM_SPEC_PRIMARY_WEIGHT"), FString::SanitizeFloat(SpecPrimaryWeight));
	EnumValues.Add(TEXT("CUSTOM_SPEC_PRIMARY_ROUGHNESS_SCALE"), FString::SanitizeFloat(SpecPrimaryRoughnessScale));
	EnumValues.Add(TEXT("CUSTOM_SPEC_SECONDARY_ROUGHNESS_SCALE"), FString::SanitizeFloat(SpecSecondaryRoughnessScale));
	EnumValues.Add(TEXT("CUSTOM_SPEC_SECONDARY_MAX_ROUGHNESS"), FString::SanitizeFloat(SpecSecondaryMaxRoughness));

	for (FShaderShiftHookConfig& Hook : ShaderHooks)
	{
		for (FShaderShiftDefineEntry& Def : Hook.Defines)
		{
			if (const FString* EnumVal = EnumValues.Find(Def.DefineName))
			{
				Def.Value = *EnumVal;
			}
		}
	}
}

void UShaderShiftSettings::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SyncEnumsToDefines();

	// Push the persist flag into the runtime module so its shutdown / crash
	// handlers see the up-to-date value.  The runtime module reads this flag
	// from its own cached static - UDeveloperSettings is not safe to access
	// from the OnShutdownAfterError lambda context.
	FShaderShiftModule::SetPersistShaderChanges(bPersistShaderChangesInEngine);

	// If persist just became active, check out the engine shader files for
	// source-engine builds (Perforce / Plastic).  Helper is a no-op when
	// persist is OFF or SCC is disabled.  Ordered before ApplyAndRecompileIfNeeded
	// so the checkout completes before any write attempts.
	CheckOutPatchedShadersIfPersist();

	UE_LOG(LogShaderShiftEditor, Log, TEXT("Settings changed - syncing hooks"));

	FShaderShiftEditorModule& EditorModule =
		FModuleManager::GetModuleChecked<FShaderShiftEditorModule>(TEXT("ShaderShiftEditor"));

	const int32 WrittenCount = EditorModule.ApplyAndRecompileIfNeeded();

	if (WrittenCount == 0)
	{
		UE_LOG(LogShaderShiftEditor, Log,
			TEXT("  No shader files changed - skipping recompile"));
	}

	SaveConfig();

	// Project-Settings edits are an immediate bake (defines just re-synced from
	// the current values), so the Quick Panel's live baked-snapshot must follow
	// or its dirty count would show stale "unbaked" entries.
	ShaderShiftQuickPanel::ReseedBakedSnapshot();
}

void UShaderShiftSettings::RecompileShadersNow()
{
	SyncEnumsToDefines();

	FShaderShiftEditorModule& EditorModule =
		FModuleManager::GetModuleChecked<FShaderShiftEditorModule>(TEXT("ShaderShiftEditor"));

	EditorModule.ForceRecompile();

	SaveConfig();
}

// ============================================================================
// FShaderShiftEditorModule
// ============================================================================

void FShaderShiftEditorModule::StartupModule()
{
	UE_LOG(LogShaderShiftEditor, Log,
		TEXT("ShaderShift Editor module starting up..."));

	TSharedPtr<IPlugin> Plugin =
		IPluginManager::Get().FindPlugin(TEXT("ShaderShift"));
	PluginDirectory = Plugin.IsValid()
		? Plugin->GetBaseDir()
		: FPaths::ProjectPluginsDir() / TEXT("ShaderShift");
	ShadersDirectory = FPaths::Combine(PluginDirectory, TEXT("Shaders"));

	LoadShaderHookConfigs();

	if (UShaderShiftSettings* Settings = UShaderShiftSettings::Get())
	{
		Settings->SyncEnumsToDefines();

		// Sync the runtime module's cached persist flag with the resolved
		// UDeveloperSettings value.  The runtime read its own copy directly
		// from the saved ini at PostConfigInit; this keeps the two sources
		// in agreement once UE's full config hierarchy is available.
		FShaderShiftModule::SetPersistShaderChanges(
			Settings->bPersistShaderChangesInEngine);
	}

	SyncSettingsWithHooks();

	// Register IDetailCustomization for the Recompile button in Project Settings.
	FPropertyEditorModule& PropertyModule =
		FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		UShaderShiftSettings::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(
			&FShaderShiftSettingsDetails::MakeInstance));

	// Register Slate style for the toolbar button icon.
	RegisterIconStyle();

	// Register the toolbar button.
	RegisterToolbarButton();

	// Park the live channels: focus lane to the 1.0 stock sentinel (ID 1 is
	// retired, so the engine-default cvar value can never spuriously arm a
	// knob), slot lanes clean, and the baked snapshot seeded from the current
	// Settings (== last-baked; unbaked previews are never persisted).  Must
	// run after the engine cvars exist.
	ShaderShiftQuickPanel::InitLiveDisarmed();

	UE_LOG(LogShaderShiftEditor, Log,
		TEXT("Editor module startup complete"));
}

void FShaderShiftEditorModule::ShutdownModule()
{
	UE_LOG(LogShaderShiftEditor, Log,
		TEXT("ShaderShift Editor module shutting down"));

	// Cancel any pending deferred recompile.
	if (DeferredRecompileHandle.IsValid())
	{
		FTSTicker::RemoveTicker(DeferredRecompileHandle);
		DeferredRecompileHandle.Reset();
	}

	// Unregister detail customization.
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(
			UShaderShiftSettings::StaticClass()->GetFName());
	}

	// Unregister toolbar button.
	UnregisterToolbarButton();

	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);

	// Close the quick panel window if open.
	if (TSharedPtr<SDockTab> Tab = GShaderShiftQuickTab.Pin())
	{
		Tab->RequestCloseTab();
	}

	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ShaderShiftTabName);
	}
}

void FShaderShiftEditorModule::RegisterIconStyle()
{
	StyleSet = MakeShareable(new FSlateStyleSet("ShaderShiftStyle"));

	// Get the plugin directory dynamically
	FString PluginDir = IPluginManager::Get().FindPlugin("ShaderShift")->GetBaseDir();
	FString IconPath = FPaths::Combine(PluginDir, TEXT("Resources/Icon_ShaderShift.png"));

	StyleSet->Set("ShaderShift.ToolbarIcon",
		new FSlateImageBrush(IconPath, FVector2D(50.0f, 50.0f))
	);

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

// ---------------------------------------------------------------------------
// ScheduleDeferredRecompile
//
// Instead of issuing "recompileshaders" from inside PostEditChangeProperty
// (which runs in a Slate callback context where FlushRenderingCommands can
// deadlock or produce partial flushes), we defer the work to the next
// game-thread tick.  This matches the timing of typing the console command
// manually, which is known to work and update the viewport correctly.
//
// If multiple property changes fire before the ticker gets a chance to run,
// we coalesce them - bPendingNeedsGlobalRecompile is OR'd so the most
// expensive required recompile mode wins.
// ---------------------------------------------------------------------------

void FShaderShiftEditorModule::ScheduleDeferredRecompile(bool bNeedsGlobalRecompile)
{
	bPendingNeedsGlobalRecompile |= bNeedsGlobalRecompile;

	// If a ticker is already pending, it will pick up the OR'd flag - no need
	// to register another one.
	if (DeferredRecompileHandle.IsValid())
	{
		return;
	}

	DeferredRecompileHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float /*DeltaTime*/) -> bool
		{
			const bool bNeedsGlobal = bPendingNeedsGlobalRecompile;
			bPendingNeedsGlobalRecompile = false;
			DeferredRecompileHandle.Reset();

			ExecuteRecompileAndRedraw(bNeedsGlobal);

			return false; // one-shot
		}),
		0.0f // fire on the very next tick
	);

	UE_LOG(LogShaderShiftEditor, Log,
		TEXT("Deferred shader recompile scheduled for next tick%s"),
		bPendingNeedsGlobalRecompile ? TEXT(" (global)") : TEXT(""));
}

// ---------------------------------------------------------------------------
// ExecuteRecompileAndRedraw
//
// This runs on a normal game-thread tick - the same context as a console
// command - so the full recompile → flush → redraw pipeline works correctly.
// ---------------------------------------------------------------------------

void FShaderShiftEditorModule::ExecuteRecompileAndRedraw(bool bNeedsGlobalRecompile)
{
	// 1. Flush the virtual shader file cache so UE re-reads our patched files.
	FlushShaderFileCache();

	// 2. Issue the appropriate recompileshaders command(s).
	if (GEngine)
	{
		if (bNeedsGlobalRecompile)
		{
			UE_LOG(LogShaderShiftEditor, Log,
				TEXT("Issuing recompileshaders global + changed (shading model hooks changed)..."));
			GEngine->Exec(nullptr, TEXT("recompileshaders global"));
			GEngine->Exec(nullptr, TEXT("recompileshaders changed"));
		}
		else
		{
			UE_LOG(LogShaderShiftEditor, Log,
				TEXT("Issuing recompileshaders changed..."));
			GEngine->Exec(nullptr, TEXT("recompileshaders changed"));
		}
	}

	// 3. Wait for any async shader compilation to finish.
	if (GShaderCompilingManager)
	{
		UE_LOG(LogShaderShiftEditor, Log, TEXT("Waiting for shader compilation to complete..."));
		GShaderCompilingManager->FinishAllCompilation();
		UE_LOG(LogShaderShiftEditor, Log, TEXT("Shader compilation finished."));
	}

	// 4. Block until the render thread has processed the new shader maps.
	//    This is safe here because we are on a normal game-thread tick,
	//    not inside a Slate callback.
	FlushRenderingCommands();

	// 5. Destroy and reallocate the ViewState on every viewport client.
	//
	//    FEditorViewportClient::ViewState holds per-viewport persistent render
	//    state including the cached color grading LUT generated by
	//    PostProcessCombineLUTs.  After the tonemapper shader is recompiled the
	//    old LUT is stale but the viewport keeps reusing it - this is why
	//    opening a *new* viewport shows the correct result (fresh ViewState)
	//    while existing viewports don't.
	//
	//    Destroying the ViewState and immediately reallocating it forces the
	//    renderer to regenerate the LUT (and all other view-dependent caches)
	//    on the next frame.
	if (GEditor)
	{
		for (FEditorViewportClient* Client : GEditor->GetAllViewportClients())
		{
			if (Client)
			{
				Client->ViewState.Destroy();
				Client->ViewState.Allocate(Client->GetWorld()
					? Client->GetWorld()->GetFeatureLevel()
					: GMaxRHIFeatureLevel);
			}
		}

		UE_LOG(LogShaderShiftEditor, Log,
			TEXT("Reset ViewState on %d viewport client(s) to force LUT regeneration"),
			GEditor->GetAllViewportClients().Num());
	}

	// 6. Redraw all viewports.
	if (GEditor)
	{
		// Invalidate every viewport client - marks surfaces dirty and forces
		// the next draw to rebuild its full pipeline state from the current
		// global shader map.
		for (FEditorViewportClient* Client : GEditor->GetAllViewportClients())
		{
			if (Client)
			{
				Client->Invalidate(/*bInvalidateChildViews=*/true,
				                   /*bInvalidateHitProxies=*/true);
			}
		}

		// Redraw all viewports - OS surface + Slate active timer + bNeedsRedraw.
		GEditor->RedrawAllViewports(/*bInvalidateHitProxies=*/true);
		FEditorSupportDelegates::RedrawAllViewports.Broadcast();

		for (FEditorViewportClient* Client : GEditor->GetAllViewportClients())
		{
			if (Client)
			{
				Client->RedrawRequested(Client->Viewport);
			}
		}

		UE_LOG(LogShaderShiftEditor, Log,
			TEXT("Viewport redraw requested on all %d viewport client(s)"),
			GEditor->GetAllViewportClients().Num());
	}
}

// ---------------------------------------------------------------------------
// ApplyAndRecompileIfNeeded
// ---------------------------------------------------------------------------

int32 FShaderShiftEditorModule::ApplyAndRecompileIfNeeded(bool bIsManualApply)
{
	UShaderShiftSettings* Settings = UShaderShiftSettings::Get();
	if (!Settings)
	{
		return 0;
	}

	FShaderShiftHookRegistry& Registry = FShaderShiftHookRegistry::Get();

	// Stamp PendingDefines, bEnabled, and bRequiresGlobalRecompile onto every hook
	for (const FShaderShiftHookConfig& HookConfig : Settings->ShaderHooks)
	{
		FShaderShiftFileHook* Hook = Registry.FindHookMutable(HookConfig.VirtualPath);
		if (Hook)
		{
			Hook->bEnabled                = HookConfig.bEnabled;
			Hook->PendingDefines          = HookConfig.Defines;
			Hook->bRequiresGlobalRecompile = HookConfig.bRequiresGlobalRecompile;
		}
	}

	// In persist mode on a source-engine build, ensure the engine shader files
	// are checked out before we attempt to write to them.  Otherwise P4 would
	// leave them read-only and our SaveStringToFile would fail.
	CheckOutPatchedShadersIfPersist();

	// Apply hooks - only hooks whose on-disk content differs will return WroteDisk
	int32 DiskWriteCount = 0;
	bool bNeedsGlobalRecompile = false;
	const int32 SuccessCount = Registry.ApplyAllEnabledHooks(DiskWriteCount, bNeedsGlobalRecompile);

	UE_LOG(LogShaderShiftEditor, Log,
		TEXT("ApplyAndRecompileIfNeeded - %d hook(s) succeeded, %d wrote new content to disk%s"),
		SuccessCount, DiskWriteCount,
		bNeedsGlobalRecompile ? TEXT(" (global recompile required)") : TEXT(""));

	// Global vs. "changed" recompile is decided purely by each hook's config flag
	// (RequiresGlobalRecompile): when a global-flagged hook (BRDF / ShadingModels /
	// Substrate / etc., all #included by every material) writes, ApplyAllEnabledHooks
	// sets bNeedsGlobalRecompile - so BRDF changes get 'recompileshaders global +
	// changed' while tonemapper / bloom / AO (flag=false) get just 'changed'.
	if (DiskWriteCount > 0)
	{
		// The Apply button (bIsManualApply) is an explicit "make my changes live"
		// action and must ALWAYS recompile.  bAutoRecompile only governs the
		// per-edit Project Settings path - a leftover from the abandoned live-update
		// feature that lets the user batch edits and recompile manually - so it must
		// never block the Apply button.
		if (bIsManualApply || Settings->bAutoRecompile)
		{
			UE_LOG(LogShaderShiftEditor, Log,
				TEXT("  Scheduling deferred recompile (disk writes: %d, global: %s)"),
				DiskWriteCount, bNeedsGlobalRecompile ? TEXT("yes") : TEXT("no"));

			ScheduleDeferredRecompile(bNeedsGlobalRecompile);
		}
		else
		{
			UE_LOG(LogShaderShiftEditor, Log,
				TEXT("  %d hook(s) changed but Auto-Recompile is OFF - use 'Recompile Shaders Now' when ready"),
				DiskWriteCount);
		}
	}
	else
	{
		UE_LOG(LogShaderShiftEditor, Verbose,
			TEXT("  All hooks already current - no recompile needed"));
	}

	return DiskWriteCount;
}

// ---------------------------------------------------------------------------
// ForceRecompile
// ---------------------------------------------------------------------------

void FShaderShiftEditorModule::ForceRecompile()
{
	UShaderShiftSettings* Settings = UShaderShiftSettings::Get();
	if (!Settings)
	{
		return;
	}

	Settings->SyncEnumsToDefines();

	FShaderShiftHookRegistry& Registry = FShaderShiftHookRegistry::Get();

	// Stamp PendingDefines, bEnabled, and bRequiresGlobalRecompile onto every hook
	for (const FShaderShiftHookConfig& HookConfig : Settings->ShaderHooks)
	{
		FShaderShiftFileHook* Hook = Registry.FindHookMutable(HookConfig.VirtualPath);
		if (Hook)
		{
			Hook->bEnabled                = HookConfig.bEnabled;
			Hook->PendingDefines          = HookConfig.Defines;
			Hook->bRequiresGlobalRecompile = HookConfig.bRequiresGlobalRecompile;
		}
	}

	// Same SCC check-out pre-pass as ApplyAndRecompileIfNeeded - see comment there.
	CheckOutPatchedShadersIfPersist();

	int32 DiskWriteCount = 0;
	bool bNeedsGlobalRecompile = false;
	Registry.ApplyAllEnabledHooks(DiskWriteCount, bNeedsGlobalRecompile);

	UE_LOG(LogShaderShiftEditor, Log,
		TEXT("ForceRecompile - scheduling recompile (global=%s)"),
		bNeedsGlobalRecompile ? TEXT("true") : TEXT("false"));

	// Always schedule a recompile even if DiskWriteCount == 0 - the user
	// pressed the button explicitly.
	ScheduleDeferredRecompile(bNeedsGlobalRecompile);
}

// ---------------------------------------------------------------------------
// LoadShaderHookConfigs
// ---------------------------------------------------------------------------

void FShaderShiftEditorModule::LoadShaderHookConfigs()
{
	const FString ConfigPath =
		FPaths::Combine(PluginDirectory, TEXT("Config/ShaderOverride.ini"));

	if (!FPaths::FileExists(ConfigPath))
	{
		UE_LOG(LogShaderShiftEditor, Warning,
			TEXT("Plugin config not found: %s"), *ConfigPath);
		return;
	}

	FConfigFile PluginConfig;
	PluginConfig.Read(ConfigPath);

	const FConfigSection* ShadersSection =
		PluginConfig.FindSection(TEXT("Shaders"));
	if (!ShadersSection)
	{
		UE_LOG(LogShaderShiftEditor, Warning,
			TEXT("No [Shaders] section in %s"), *ConfigPath);
		return;
	}

	TArray<FString> ShaderEntries;
	for (auto It = ShadersSection->CreateConstIterator(); It; ++It)
	{
		if (It->Key == TEXT("+Shader"))
		{
			ShaderEntries.Add(It->Value.GetValue());
		}
	}

	UShaderShiftSettings* Settings = UShaderShiftSettings::Get();
	if (!Settings)
	{
		UE_LOG(LogShaderShiftEditor, Warning,
			TEXT("UShaderShiftSettings CDO not available"));
		return;
	}

	for (const FString& Entry : ShaderEntries)
	{
		FString VirtualPath, TemplateFilename, HookDisplayName;
		FString RequiresGlobalStr;

		FParse::Value(*Entry, TEXT("VirtualPath="),              VirtualPath);
		FParse::Value(*Entry, TEXT("TemplateFilename="),         TemplateFilename);
		FParse::Value(*Entry, TEXT("DisplayName="),              HookDisplayName);
		FParse::Value(*Entry, TEXT("RequiresGlobalRecompile="),  RequiresGlobalStr);

		VirtualPath      = VirtualPath.TrimQuotes();
		TemplateFilename = TemplateFilename.TrimQuotes();
		HookDisplayName  = HookDisplayName.TrimQuotes();
		RequiresGlobalStr = RequiresGlobalStr.TrimQuotes();

		const bool bRequiresGlobal =
			RequiresGlobalStr.Equals(TEXT("true"), ESearchCase::IgnoreCase)
			|| RequiresGlobalStr == TEXT("1");

		if (VirtualPath.IsEmpty() || TemplateFilename.IsEmpty())
		{
			UE_LOG(LogShaderShiftEditor, Warning,
				TEXT("Incomplete shader entry: %s"), *Entry);
			continue;
		}

		// Section header is keyed by the bare filename (no path prefix), to
		// match what the runtime module looks up via GetCleanFilename in
		// EarlyApplySavedSettings.  Without this strip, hooks whose
		// TemplateFilename includes a subdirectory (e.g. "Bloom/Foo.usf")
		// get an empty Defines array and SyncEnumsToDefines silently
		// skips them - the patched on-disk file then keeps its default
		// #define values regardless of the dropdown selection.
		const FString SectionKey = FPaths::GetCleanFilename(TemplateFilename);

		const FConfigSection* DefineSection =
			PluginConfig.FindSection(SectionKey);

		TMap<FString, FShaderShiftDefineEntry> ConfigDefines;

		if (DefineSection)
		{
			for (auto It = DefineSection->CreateConstIterator(); It; ++It)
			{
				if (It->Key != TEXT("+Define"))
				{
					continue;
				}

				const FString& DefEntry = It->Value.GetValue();

				FShaderShiftDefineEntry Def;
				FString DefName, DefDisplay, DefDesc, DefDefault;

				FParse::Value(*DefEntry, TEXT("DefineName="),   DefName);
				FParse::Value(*DefEntry, TEXT("DisplayName="),  DefDisplay);
				FParse::Value(*DefEntry, TEXT("Description="),  DefDesc);
				FParse::Value(*DefEntry, TEXT("DefaultValue="), DefDefault);

				Def.DefineName   = DefName.TrimQuotes();
				Def.DisplayName  = DefDisplay.TrimQuotes();
				Def.Description  = DefDesc.TrimQuotes();
				Def.DefaultValue = DefDefault.TrimQuotes();
				Def.Value        = Def.DefaultValue;

				if (!Def.DefineName.IsEmpty())
				{
					ConfigDefines.Add(Def.DefineName, Def);
				}
			}
		}
		else
		{
			UE_LOG(LogShaderShiftEditor, Log,
				TEXT("No defines section [%s] in config"), *SectionKey);
		}

		FShaderShiftHookConfig* HookConfig = nullptr;
		for (FShaderShiftHookConfig& HC : Settings->ShaderHooks)
		{
			if (HC.VirtualPath == VirtualPath)
			{
				HookConfig = &HC;
				break;
			}
		}

		if (!HookConfig)
		{
			FShaderShiftHookConfig NewConfig;
			NewConfig.DisplayName              = HookDisplayName;
			NewConfig.VirtualPath              = VirtualPath;
			NewConfig.TemplateFilename         = TemplateFilename;
			NewConfig.bEnabled                 = true;
			NewConfig.bRequiresGlobalRecompile = bRequiresGlobal;
			Settings->ShaderHooks.Add(NewConfig);
			HookConfig = &Settings->ShaderHooks.Last();
		}
		else
		{
			if (HookConfig->DisplayName.IsEmpty())
			{
				HookConfig->DisplayName = HookDisplayName;
			}
			HookConfig->TemplateFilename         = TemplateFilename;
			HookConfig->bRequiresGlobalRecompile  = bRequiresGlobal;
		}

		for (auto& Pair : ConfigDefines)
		{
			const FString& DefName                   = Pair.Key;
			const FShaderShiftDefineEntry& ConfigDef = Pair.Value;

			FShaderShiftDefineEntry* Existing = nullptr;
			for (FShaderShiftDefineEntry& D : HookConfig->Defines)
			{
				if (D.DefineName == DefName)
				{
					Existing = &D;
					break;
				}
			}

			if (Existing)
			{
				Existing->DisplayName  = ConfigDef.DisplayName;
				Existing->Description  = ConfigDef.Description;
				Existing->DefaultValue = ConfigDef.DefaultValue;
			}
			else
			{
				HookConfig->Defines.Add(ConfigDef);
			}
		}
	}

	UE_LOG(LogShaderShiftEditor, Log,
		TEXT("Loaded %d shader hook configs from plugin config"),
		ShaderEntries.Num());
}

// ---------------------------------------------------------------------------
// SyncSettingsWithHooks
// ---------------------------------------------------------------------------

void FShaderShiftEditorModule::SyncSettingsWithHooks()
{
	UShaderShiftSettings* Settings = UShaderShiftSettings::Get();
	if (!Settings)
	{
		return;
	}

	FShaderShiftHookRegistry& Registry = FShaderShiftHookRegistry::Get();

	// Ensure every registry hook has a settings entry (covers programmatic hooks)
	TArray<const FShaderShiftFileHook*> AllHooks;
	Registry.GetAllHooks(AllHooks);

	for (const FShaderShiftFileHook* Hook : AllHooks)
	{
		bool bFound = false;
		for (const FShaderShiftHookConfig& HC : Settings->ShaderHooks)
		{
			if (HC.VirtualPath == Hook->VirtualPath)
			{
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			FShaderShiftHookConfig NewConfig;
			NewConfig.DisplayName      = Hook->DisplayName;
			NewConfig.VirtualPath      = Hook->VirtualPath;
			NewConfig.TemplateFilename = Hook->Filename;
			NewConfig.bEnabled         = true;
			Settings->ShaderHooks.Add(NewConfig);
		}
	}

	// Stamp PendingDefines, bEnabled, and bRequiresGlobalRecompile onto every hook
	for (const FShaderShiftHookConfig& HookConfig : Settings->ShaderHooks)
	{
		FShaderShiftFileHook* Hook = Registry.FindHookMutable(HookConfig.VirtualPath);
		if (Hook)
		{
			Hook->bEnabled                = HookConfig.bEnabled;
			Hook->PendingDefines          = HookConfig.Defines;
			Hook->bRequiresGlobalRecompile = HookConfig.bRequiresGlobalRecompile;
		}
	}

	// Apply all hooks to disk.  The runtime module (PostConfigInit) already
	// patched files using saved settings before the engine compiled shaders.
	// This call will return AlreadyCurrent for hooks whose on-disk content
	// matches - meaning no recompile is needed (the engine already compiled
	// the patched versions).
	//
	// DiskWriteCount > 0 only if the editor module's UObject-based settings
	// differ from what the runtime module wrote (e.g., the user's saved config
	// was somehow inconsistent, or the runtime module couldn't read GConfig).
	// In that case we schedule a deferred recompile as a fallback.
	int32 DiskWriteCount = 0;
	bool bNeedsGlobalRecompile = false;
	const int32 AppliedCount = Registry.ApplyAllEnabledHooks(DiskWriteCount, bNeedsGlobalRecompile);

	UE_LOG(LogShaderShiftEditor, Log,
		TEXT("SyncSettingsWithHooks - %d / %d hooks applied (%d wrote new disk content)"),
		AppliedCount, Registry.GetHookCount(), DiskWriteCount);

	if (DiskWriteCount > 0)
	{
		UE_LOG(LogShaderShiftEditor, Log,
			TEXT("  Startup: %d hook(s) wrote to disk (settings differed from early apply) - scheduling deferred recompile"),
			DiskWriteCount);

		ScheduleDeferredRecompile(bNeedsGlobalRecompile);
	}
	else
	{
		UE_LOG(LogShaderShiftEditor, Log,
			TEXT("  All hooks already current from early apply - no startup recompile needed"));
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FShaderShiftEditorModule, ShaderShiftEditor)