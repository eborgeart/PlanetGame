// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ShaderShift.h"
#include "ShaderShiftSettings.generated.h"

// ---------------------------------------------------------------------------
// Enums for Project Settings dropdowns
// ---------------------------------------------------------------------------

/** Tone mapping operator. */
UENUM()
enum class ECustomTonemapMode : uint8
{
	ACES_Filmic        UMETA(DisplayName = "ACES Filmic"),
	AgX_Punchy         UMETA(DisplayName = "AgX"),
	Reinhard           UMETA(DisplayName = "Reinhard"),
	Linear             UMETA(DisplayName = "Linear (No Tonemapper)"),
	Custom             UMETA(DisplayName = "Custom (Uncharted 2)"),
	GT_Uchimura        UMETA(DisplayName = "Gran Turismo (GT Tonemap, Uchimura 2017)"),
	GT7_Perceptual     UMETA(DisplayName = "Gran Turismo 7 (Perceptual, 2025)"),
	Apex               UMETA(DisplayName = "Apex (AgX look + HDR-native GT curve)"),
	Prism              UMETA(DisplayName = "Prism (ShaderShift, hue-preserving HDR)"),
};

/** Creative look applied on top of the AgX base curve. */
UENUM()
enum class EAgxLook : uint8
{
	Default  UMETA(DisplayName = "Default (Neutral)"),
	Golden   UMETA(DisplayName = "Golden"),
	Punchy   UMETA(DisplayName = "Punchy")
};

/** Diffuse BRDF model. */
UENUM()
enum class ECustomDiffuseMode : uint8
{
	EngineDefault  UMETA(DisplayName = "Engine Default (Lambert / Chan / EON)"),
	OrenNayar      UMETA(DisplayName = "Energy-conserving Oren-Nayar"),
	DisneyBurley   UMETA(DisplayName = "Disney Burley"),
	LambertSphere  UMETA(DisplayName = "Lambert-Sphere (d'Eon 2021)"),
	Gotanda        UMETA(DisplayName = "Gotanda (tri-Ace)"),
	Callisto       UMETA(DisplayName = "Callisto (Diffuse Fresnel + Retro + Smooth Terminator)"),
	Proxima        UMETA(DisplayName = "Proxima (roughness-aware diffuse / diffuse AA)"),
	Regolith       UMETA(DisplayName = "Regolith (Micrograin Dust Layer)")
};

/** Specular BRDF model. */
UENUM()
enum class ECustomSpecMode : uint8
{
	EngineDefault   UMETA(DisplayName = "Engine Default (Single-Lobe GGX)"),
	MultiLobe       UMETA(DisplayName = "Multi-Lobe Cinematic"),
	CallistoDualGGX UMETA(DisplayName = "Callisto Modified Dual GGX (Spec Fresnel Falloff + Dual Lobe)"),
	RegolithSheen   UMETA(DisplayName = "Regolith Sheen (Dust Layer)")
};

/**
 * Callisto diffuse "look" preset.  Selecting anything other than Custom writes a
 * whole group of CALLISTO_DIFF_* / CALLISTO_RETRO_* / CALLISTO_SMOOTH_TERM_*
 * values in SyncEnumsToDefines, so users get an instant look without editing the
 * ~8 individual knobs.  Custom leaves the Project-Settings values untouched.
 * Only meaningful when DiffuseMode == Callisto.
 */
UENUM()
enum class ECallistoDiffusePreset : uint8
{
	Custom        UMETA(DisplayName = "Custom (use Project Settings values)"),
	Neutral       UMETA(DisplayName = "Neutral (engine Lambert)"),
	PaperDefault  UMETA(DisplayName = "Paper Default (Callisto reference look)"),
	SoftSkin      UMETA(DisplayName = "Soft Skin (peach-fuzz + soft terminator)"),
	VelvetSheen   UMETA(DisplayName = "Velvet / Sheen (fabric rim)"),
	LunarMatte    UMETA(DisplayName = "Lunar Matte (heavy backscatter)"),
	WaxyPlush     UMETA(DisplayName = "Waxy / Plush (soft waxy fill)")
};

/**
 * Callisto Modified Dual GGX specular "look" preset (drives CALLISTO_DUAL_SPEC_*
 * and CALLISTO_SPEC_FRESNEL_FALLOFF).  Only meaningful when SpecularMode ==
 * CallistoDualGGX.  Custom leaves Project-Settings values untouched.
 */
UENUM()
enum class ECallistoSpecPreset : uint8
{
	Custom       UMETA(DisplayName = "Custom (use Project Settings values)"),
	SingleLobe   UMETA(DisplayName = "Single Lobe (engine-like)"),
	SoftCoat     UMETA(DisplayName = "Soft Coat (broad Fresnel - paper look)"),
	GlossyDual   UMETA(DisplayName = "Glossy Dual-Lobe"),
	SatinSheen   UMETA(DisplayName = "Satin Sheen (broad soft secondary)")
};

/**
 * Original Multi-Lobe Cinematic specular "look" preset (drives the CUSTOM_SPEC_*
 * primary/secondary lobe knobs).  Only meaningful when SpecularMode == MultiLobe.
 * Custom leaves Project-Settings values untouched.
 */
UENUM()
enum class EMultiLobeSpecPreset : uint8
{
	Custom        UMETA(DisplayName = "Custom (use Project Settings values)"),
	SingleLobe    UMETA(DisplayName = "Single Lobe (primary only)"),
	Cinematic     UMETA(DisplayName = "Cinematic (soft dual highlight)"),
	CarPaint      UMETA(DisplayName = "Car Paint (tight core + flake haze)"),
	SoftSkinSpec  UMETA(DisplayName = "Soft Skin (broad secondary)")
};

/** Bloom kernel / look. */
UENUM()
enum class ECustomBloomMode : uint8
{
	EngineDefault       UMETA(DisplayName = "Engine Default"),
	DualKawase          UMETA(DisplayName = "Dual Kawase Blur"),
	CODScatterAsGather  UMETA(DisplayName = "COD Scatter-as-Gather (Karis 13/Tent 9)"),
	FFTDiffraction      UMETA(DisplayName = "FFT Diffraction Starburst"),
	AnamorphicStreak    UMETA(DisplayName = "Anamorphic Lens Streak"),
	SpencerOcular       UMETA(DisplayName = "Spencer Ocular PSF"),
	AnimeHardGlow       UMETA(DisplayName = "Anime / Toon Hard Glow"),
	RetroCRT            UMETA(DisplayName = "Retro / CRT-style Glow")
};

/**
 * Ambient Occlusion algorithm selector.
 *
 * Drives CUSTOM_AO_MODE inside the patched PostProcessAmbientOcclusion.usf
 * (both the legacy SSAO MainPSandCS entry and the GTAOCombinedPSandCS entry).
 *
 *   EngineDefault -- engine SSAO / GTAO unchanged (no-op patch).
 *   SSILVB        -- Screen Space Indirect Lighting with Visibility Bitmask
 *                    (Therrien et al. 2023).  32-bit per-slice occupancy mask
 *                    replaces horizon-cosine accumulation; fixes thin-object
 *                    over-darkening and runs in the same pass as engine SSAO.
 *
 * More AO methods can be appended without changing the wiring in
 * SyncEnumsToDefines or EarlyApplySavedSettings -- just add the enum entry
 * and a new #if CUSTOM_AO_MODE == N branch in the shader template.
 */
UENUM()
enum class ECustomAOMode : uint8
{
	EngineDefault  UMETA(DisplayName = "Engine Default (SSAO / GTAO)"),
	SSILVB         UMETA(DisplayName = "SSILVB (Visibility Bitmask)")
};

/**
 * Screen-Space Global Illumination algorithm selector.
 *
 * Drives CUSTOM_SSGI_MODE inside the patched SSRTDiffuseIndirect.usf.  The
 * shader runs only when r.SSGI.Enable=1 AND r.DynamicGlobalIlluminationMethod=2
 * (Screen Space).  Method=0 means no GI at all; Method=1 routes through Lumen
 * which bypasses this shader entirely.
 *
 *   EngineDefault    -- engine stochastic SSRT ray cast, unchanged (no-op patch).
 *   SSILVB           -- SSILVB indirect diffuse with visibility-bitmask gather
 *                       (Therrien et al. 2023, paper Algorithm 1).  Reads the
 *                       reduced previous-frame scene color plus per-tap normals
 *                       for emitter-side back-face culling and per-sector
 *                       contribution weighting.
 *   SSILVB_Diagnostic -- writes bright magenta into the GI buffer instead of
 *                       any actual computation.  Use to verify the shader hook
 *                       is applying and the substitution worked: if the scene
 *                       goes magenta when this mode is selected, the SSILVB
 *                       path is reaching the GPU; if it looks unchanged the
 *                       patch isn't taking effect (recompile didn't happen,
 *                       the hook is disabled, or persist mode is stale).
 *   StableSSGI        -- engine's stochastic ray cast unchanged, BUT with
 *                       frame-stable sample directions and step offsets.  The
 *                       engine path uses ComputeRandomSeed (which mixes in
 *                       View.StateFrameIndex) and InterleavedGradientNoise of
 *                       View.StateFrameIndexMod8, so ray directions and step
 *                       starts shuffle every frame and produce the visible
 *                       "boiling" temporal noise that TSR/TAA can't denoise.
 *                       This mode replaces both with pixel-only hashes so the
 *                       same rays cast at the same offsets every frame --
 *                       TSR's motion-compensated history then accumulates
 *                       cleanly instead of rejecting the per-frame shuffle.
 *
 * Like ECustomAOMode the enum is extensible -- add entries plus matching
 * #if CUSTOM_SSGI_MODE == N branches in the shader template.
 */
UENUM()
enum class ECustomSSGIMode : uint8
{
	EngineDefault       UMETA(DisplayName = "Engine Default (Stochastic Ray Cast)"),
	SSILVB              UMETA(DisplayName = "SSILVB (Visibility Bitmask)"),
	SSILVB_Diagnostic   UMETA(DisplayName = "Diagnostic (Magenta tint - verify hook is active)"),
	StableSSGI          UMETA(DisplayName = "Stable SSGI (Engine ray cast, TSR-friendly)")
};

/**
 * Per-lane Hammersley ray multiplier for Stable SSGI (Mode 3).
 *
 * Each compute lane casts N rays in a loop instead of one and writes the
 * mean to LDS.  The tile-local bilateral denoiser then averages across
 * CONFIG_RAY_COUNT lanes per pixel, so the effective sample count scales
 * linearly: x4 gathers 4x more rays per pixel per frame.  Noise floor
 * drops by sqrt(N).  Cost scales linearly with the multiplier; x8 means
 * roughly 8x the SSGI compute time.
 *
 * Only takes effect when SSGIMode == StableSSGI.  Other modes ignore it.
 */
UENUM()
enum class ECustomSSGIQuality : uint8
{
	x1   UMETA(DisplayName = "x1 (baseline - same as engine ray count)"),
	x2   UMETA(DisplayName = "x2 (2 rays/lane - ~sqrt(2) less noise)"),
	x4   UMETA(DisplayName = "x4 (4 rays/lane - half the noise)"),
	x8   UMETA(DisplayName = "x8 (8 rays/lane - smoothest, 8x cost)")
};

/**
 * Volumetric fog phase function.
 *
 * Selects the in-scattering distribution used by the directional light, sky
 * SH zonal harmonic, and all local lights inside the volumetric fog froxel
 * grid.  All five modes route through the same PhaseFunction() wrapper inside
 * the patched VolumetricFog.usf.
 */
UENUM()
enum class ECustomVolPhaseMode : uint8
{
	EngineDefault     UMETA(DisplayName = "Engine Default (Henyey-Greenstein)"),
	Schlick           UMETA(DisplayName = "Schlick (cheaper HG approximation)"),
	CornetteShanks    UMETA(DisplayName = "Cornette-Shanks (Mie improvement)"),
	DualLobeHG        UMETA(DisplayName = "Dual-Lobe HG (cloud-style Mie)"),
	MieApprox         UMETA(DisplayName = "Jendersie-d'Eon 2023 Approximate Mie")
};

/**
 * Number of secondary ray-march taps used to compute fog-on-fog self-shadowing
 * (Phase A).  At each step the shader samples the density volume along the
 * direction toward the sun and accumulates Beer-Lambert transmittance.  Off
 * matches engine behaviour; the higher tiers give darker fog interiors and
 * volumetric god-ray falloff at proportional GPU cost.
 */
UENUM()
enum class ECustomVolSelfShadow : uint8
{
	Off       UMETA(DisplayName = "Off (engine default)"),
	Steps4    UMETA(DisplayName = "4 taps (fast)"),
	Steps8    UMETA(DisplayName = "8 taps (balanced)"),
	Steps16   UMETA(DisplayName = "16 taps (cinematic)")
};

/**
 * Phase B multi-octave scattering approximation (Wrenninge/Frostbite).
 * Each successive octave represents another bounce of light through the
 * medium with attenuated phase anisotropy, contribution, and shadow
 * influence.  This is what gives dense fog the 'lit from within' look
 * instead of looking inky-black -- it compensates for single-scattering
 * Beer-Lambert killing the directional contribution inside dense volumes.
 */
UENUM()
enum class ECustomVolMultiScatter : uint8
{
	Off        UMETA(DisplayName = "Off (single scattering)"),
	Octaves2   UMETA(DisplayName = "2 octaves (subtle)"),
	Octaves3   UMETA(DisplayName = "3 octaves (recommended)"),
	Octaves4   UMETA(DisplayName = "4 octaves (cinematic)")
};

// ---------------------------------------------------------------------------
// UShaderShiftSettings - Project Settings > Plugins > ShaderShift
// ---------------------------------------------------------------------------

UCLASS(config = ShaderShift, defaultconfig, meta = (DisplayName = "ShaderShift"))
class SHADERSHIFTEDITOR_API UShaderShiftSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UShaderShiftSettings();

	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetCategoryName() const override  { return TEXT("Plugins"); }
	virtual FName GetSectionName()  const override  { return TEXT("ShaderShift"); }

	virtual FText GetSectionText() const override
	{
		return NSLOCTEXT("ShaderShift", "SettingsSection", "ShaderShift");
	}

	virtual FText GetSectionDescription() const override
	{
		return NSLOCTEXT("ShaderShift", "SettingsDesc",
			"Configure engine shader overrides, BRDF, and tone mapping settings.");
	}

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	static UShaderShiftSettings* Get();

	// =================================================================
	// Tone Mapping
	// =================================================================

	/**
	 * Tone mapping operator.
	 *
	 * NOTE: All UENUM defaults below intentionally point at the engine-default
	 * value (= 0).  These are the CDO defaults the Quick Settings panel reads
	 * on a fresh install (no Saved/Config/.../ShaderShift.ini present).  They
	 * MUST stay in sync with the DefaultIntValue fallbacks in
	 * FShaderShiftModule::EarlyApplySavedSettings - the runtime module can't
	 * read the CDO at PostConfigInit, so it has its own copy of these values.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Tone Mapping",
		meta = (DisplayName = "Tonemapper"))
	ECustomTonemapMode TonemapMode = ECustomTonemapMode::ACES_Filmic;

	/** AgX creative look (only relevant when an AgX tonemapper is selected) */
	UPROPERTY(config, EditAnywhere, Category = "Tone Mapping",
		meta = (DisplayName = "AgX Look"))
	EAgxLook AgxLook = EAgxLook::Default;

	// =================================================================
	// Shading Model
	// =================================================================

	/** Diffuse BRDF */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Diffuse BRDF"))
	ECustomDiffuseMode DiffuseMode = ECustomDiffuseMode::EngineDefault;

	/** Specular BRDF */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Specular BRDF"))
	ECustomSpecMode SpecularMode = ECustomSpecMode::EngineDefault;

	/** One-click Callisto diffuse look. Custom = keep the individual CALLISTO_* knobs below. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Callisto Diffuse Preset",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	ECallistoDiffusePreset CallistoDiffusePreset = ECallistoDiffusePreset::Custom;

	/**
	 * Proxima diffuse roughness scale (CALLISTO_PROXIMA_ALPHA_SCALE).
	 * 0 = Lambert, 1 = default backscatter, higher = rougher / more backscatter.
	 * Only used when DiffuseMode == Proxima.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Proxima Diffuse Roughness",
				ClampMin = "0.0", ClampMax = "4.0",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Proxima"))
	float ProximaAlphaScale = 1.0f;

	// ------------------------------------------------------------------
	// Regolith micrograin dust layer (REGOLITH_*).  Shared by the diffuse
	// mode (Regolith) and the specular mode (Regolith Sheen); shown when
	// either is active.  Coverage 0 = engine-exact.
	// ------------------------------------------------------------------

	/** Dust coverage CEILING tau [0,1] (REGOLITH_COVERAGE). Each pixel's roughness gates how much it holds -- polished surfaces shed light dust, heavy dust buries everything. 0 = engine-exact. Live. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Regolith Dust Coverage",
				ClampMin = "0.0", ClampMax = "1.0",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Regolith || SpecularMode == ECustomSpecMode::RegolithSheen"))
	float RegolithCoverage = 0.35f;

	/** Grain sheen lobe roughness (REGOLITH_GRAIN_ROUGHNESS). Live. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Regolith Grain Roughness",
				ClampMin = "0.05", ClampMax = "1.0",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Regolith || SpecularMode == ECustomSpecMode::RegolithSheen"))
	float RegolithGrainRoughness = 0.6f;

	/** Opposition retro-surge strength (REGOLITH_RETRO): dust glow when the light sits behind the camera. Live. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Regolith Opposition Glow",
				ClampMin = "0.0", ClampMax = "2.0",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Regolith || SpecularMode == ECustomSpecMode::RegolithSheen"))
	float RegolithRetro = 0.8f;

	/** Grain specular lobe gain (REGOLITH_SHEEN). 0 = chalk (dust kills gloss), ~1 = silky sheen. Live. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Regolith Dust Sheen",
				ClampMin = "0.0", ClampMax = "2.0",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Regolith || SpecularMode == ECustomSpecMode::RegolithSheen"))
	float RegolithSheen = 0.6f;

	/** Roughness affinity of the dust (REGOLITH_ROUGHNESS_AFFINITY): 1 = dust clings to rough surfaces only (previous behavior), 0 = coverage applies flat. Live. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Dust Roughness Affinity",
				ClampMin = "0.0", ClampMax = "1.0",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Regolith || SpecularMode == ECustomSpecMode::RegolithSheen"))
	float RegolithRoughnessAffinity = 1.0f;

	/** Dust albedo tint target, red (REGOLITH_TINT_R). Applies on Bake. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Regolith Tint R",
				ClampMin = "0.0", ClampMax = "2.0",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Regolith || SpecularMode == ECustomSpecMode::RegolithSheen"))
	float RegolithTintR = 1.0f;

	/** Dust albedo tint target, green (REGOLITH_TINT_G). Applies on Bake. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Regolith Tint G",
				ClampMin = "0.0", ClampMax = "2.0",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Regolith || SpecularMode == ECustomSpecMode::RegolithSheen"))
	float RegolithTintG = 1.0f;

	/** Dust albedo tint target, blue (REGOLITH_TINT_B). Applies on Bake. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Regolith Tint B",
				ClampMin = "0.0", ClampMax = "2.0",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Regolith || SpecularMode == ECustomSpecMode::RegolithSheen"))
	float RegolithTintB = 1.0f;

	/** Dust albedo source blend (REGOLITH_TINT_BLEND): 0 = dust keeps the base color, 1 = pure tint. Applies on Bake. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Regolith Tint Blend",
				ClampMin = "0.0", ClampMax = "1.0",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Regolith || SpecularMode == ECustomSpecMode::RegolithSheen"))
	float RegolithTintBlend = 0.0f;

	/** One-click Callisto Modified Dual GGX specular look. Custom = keep the individual knobs. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Callisto Specular Preset",
				EditCondition = "SpecularMode == ECustomSpecMode::CallistoDualGGX"))
	ECallistoSpecPreset CallistoSpecPreset = ECallistoSpecPreset::Custom;

	/** One-click Multi-Lobe Cinematic specular look. Custom = keep the individual knobs. */
	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Multi-Lobe Specular Preset",
				EditCondition = "SpecularMode == ECustomSpecMode::MultiLobe"))
	EMultiLobeSpecPreset MultiLobeSpecPreset = EMultiLobeSpecPreset::Custom;

	// =================================================================
	// Per-knob BRDF tunables (live-tweakable, below the preset dropdowns).
	// Source of truth for the baked CALLISTO_*/CUSTOM_SPEC_* defines.  A non-
	// Custom preset fills these in SyncEnumsToDefines; dragging a slider sets the
	// matching preset back to Custom so the edit is not re-clobbered.  Each has a
	// live View-lane id so the viewport previews it with no recompile.
	// =================================================================


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Diffuse Fresnel",
				ClampMin = "0", ClampMax = "8",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoDiffFresnel = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Diffuse Fresnel Falloff",
				ClampMin = "0", ClampMax = "1",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoDiffFresnelFalloff = 0.75f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Diffuse Fresnel Tangent Falloff",
				ClampMin = "0", ClampMax = "1",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoDiffFresnelTanFalloff = 0.75f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Diffuse Fresnel Tint R",
				ClampMin = "0", ClampMax = "4",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoDiffFresnelTintR = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Diffuse Fresnel Tint G",
				ClampMin = "0", ClampMax = "4",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoDiffFresnelTintG = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Diffuse Fresnel Tint B",
				ClampMin = "0", ClampMax = "4",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoDiffFresnelTintB = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Retroreflection",
				ClampMin = "0", ClampMax = "8",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoRetro = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Retroreflection Falloff",
				ClampMin = "0", ClampMax = "1",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoRetroFalloff = 0.75f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Retroreflection Tangent Falloff",
				ClampMin = "0", ClampMax = "1",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoRetroTanFalloff = 0.75f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Retroreflection Tint R",
				ClampMin = "0", ClampMax = "4",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoRetroTintR = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Retroreflection Tint G",
				ClampMin = "0", ClampMax = "4",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoRetroTintG = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Retroreflection Tint B",
				ClampMin = "0", ClampMax = "4",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoRetroTintB = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Smooth Terminator",
				ClampMin = "-1", ClampMax = "1",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoSmoothTerm = 0.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Smooth Terminator Length",
				ClampMin = "0", ClampMax = "1",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoSmoothTermLength = 0.5f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Smooth Terminator Tint R",
				ClampMin = "0", ClampMax = "4",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoSmoothTermTintR = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Smooth Terminator Tint G",
				ClampMin = "0", ClampMax = "4",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoSmoothTermTintG = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Smooth Terminator Tint B",
				ClampMin = "0", ClampMax = "4",
				EditCondition = "DiffuseMode == ECustomDiffuseMode::Callisto"))
	float CallistoSmoothTermTintB = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Specular Fresnel Falloff",
				ClampMin = "0", ClampMax = "1",
				EditCondition = "SpecularMode == ECustomSpecMode::CallistoDualGGX"))
	float CallistoSpecFresnelFalloff = 1.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Dual Specular Roughness Scale",
				ClampMin = "1", ClampMax = "14",
				EditCondition = "SpecularMode == ECustomSpecMode::CallistoDualGGX"))
	float CallistoDualSpecRoughnessScale = 2.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Dual Specular Opacity",
				ClampMin = "0", ClampMax = "1",
				EditCondition = "SpecularMode == ECustomSpecMode::CallistoDualGGX"))
	float CallistoDualSpecOpacity = 0.0f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Specular Primary Weight",
				ClampMin = "0", ClampMax = "1",
				EditCondition = "SpecularMode == ECustomSpecMode::MultiLobe"))
	float SpecPrimaryWeight = 0.7f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Primary Roughness Scale",
				ClampMin = "0", ClampMax = "2",
				EditCondition = "SpecularMode == ECustomSpecMode::MultiLobe"))
	float SpecPrimaryRoughnessScale = 0.45f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Secondary Roughness Scale",
				ClampMin = "0", ClampMax = "4",
				EditCondition = "SpecularMode == ECustomSpecMode::MultiLobe"))
	float SpecSecondaryRoughnessScale = 2.2f;


	UPROPERTY(config, EditAnywhere, Category = "Shading Model",
		meta = (DisplayName = "Secondary Max Roughness",
				ClampMin = "0", ClampMax = "1",
				EditCondition = "SpecularMode == ECustomSpecMode::MultiLobe"))
	float SpecSecondaryMaxRoughness = 0.95f;

	// =================================================================
	// Bloom
	// =================================================================

	/** Bloom kernel / look */
	UPROPERTY(config, EditAnywhere, Category = "Bloom",
		meta = (DisplayName = "Bloom Mode"))
	ECustomBloomMode BloomMode = ECustomBloomMode::EngineDefault;

	/**
	 * When enabled, the bloom setup pass applies a hue-preserving saturation
	 * boost to bright pixels so high-emissive object cores retain their colour
	 * through the tonemapper instead of trending to white - the pre-4.17
	 * Unreal bloom behaviour.
	 *
	 * Only relevant for the non-convolution bloom paths (engine default and
	 * setup-stage modes), since FFT Diffraction / Spencer Ocular are kernel
	 * replacements that go through a different normalisation pipeline.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Bloom",
		meta = (DisplayName = "Preserve Emissive Color (pre-4.17 style)",
				EditCondition = "BloomMode != ECustomBloomMode::FFTDiffraction && BloomMode != ECustomBloomMode::SpencerOcular"))
	bool bPreserveEmissiveColor = false;

	// =================================================================
	// Ambient Occlusion
	//
	// AOMode drives CUSTOM_AO_MODE inside the patched
	// PostProcessAmbientOcclusion.usf.  EngineDefault keeps the engine's
	// SSAO / GTAO paths untouched; SSILVB swaps them for a 32-bit
	// per-slice visibility-bitmask trace that fixes the classic horizon-AO
	// "dark halo behind thin geometry" failure mode.
	// =================================================================

	/** Ambient occlusion algorithm. */
	UPROPERTY(config, EditAnywhere, Category = "Ambient Occlusion",
		meta = (DisplayName = "AO Method"))
	ECustomAOMode AOMode = ECustomAOMode::EngineDefault;

	// =================================================================
	// Screen-Space Global Illumination
	//
	// SSGIMode drives CUSTOM_SSGI_MODE inside the patched
	// SSRTDiffuseIndirect.usf.  Only takes effect when the engine's SSGI
	// pass is active (r.SSGI.Enable=1) AND Lumen GI is disabled
	// (r.DynamicGlobalIlluminationMethod = 0 or 2).
	// =================================================================

	/** Screen-space GI algorithm. */
	UPROPERTY(config, EditAnywhere, Category = "Screen-Space GI",
		meta = (DisplayName = "SSGI Method",
				ToolTip = "Requires r.DynamicGlobalIlluminationMethod=2 (Screen Space) AND r.SSGI.Enable=1. With Method=0 the engine runs no GI pass; with Method=1 (Lumen) the engine takes a different code path. Use Diagnostic to verify the patch is reaching the GPU -- the scene should go magenta."))
	ECustomSSGIMode SSGIMode = ECustomSSGIMode::EngineDefault;

	/** Per-lane sample multiplier for SSILVB (extra hemisphere slices) and Stable SSGI (extra rays). */
	UPROPERTY(config, EditAnywhere, Category = "Screen-Space GI",
		meta = (DisplayName = "SSGI Sample Multiplier",
				ToolTip = "Per-lane supersampling multiplier. SSILVB traces N hemisphere slices per lane; Stable SSGI casts N Hammersley rays per lane. Higher = sqrt(N) less noise at linear cost. Takes effect when SSGI Method == SSILVB or Stable SSGI.",
				EditCondition = "SSGIMode == ECustomSSGIMode::StableSSGI || SSGIMode == ECustomSSGIMode::SSILVB"))
	ECustomSSGIQuality SSGIQuality = ECustomSSGIQuality::x1;

	/**
	 * SSILVB GI brightness multiplier.  Scales the final per-slice DiffuseGI
	 * before it is encoded into the SSGI output buffer.  Drives
	 * CUSTOM_SSGI_INTENSITY inside the patched SSRTDiffuseIndirect.usf.
	 *
	 * The slice estimator is normalised in-shader (x pi/2) so that 1.0 means
	 * "engine-parity brightness" (a uniform-radiance surround reports the
	 * same value as the stock cosine-sampled SSGI).  SSILVB still reads a
	 * touch dimmer on real scenes because it weights emitters by their
	 * foreshortening cosine, which stock SSGI ignores -- the 2.0 default
	 * compensates.  Tune ~1-4.
	 *
	 * Only takes effect when SSGI Method == SSILVB.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Screen-Space GI",
		meta = (DisplayName = "SSILVB Intensity",
				ToolTip = "SSILVB GI brightness multiplier. 1.0 = engine-parity brightness (estimator is pi/2-normalised in-shader); 2.0 default compensates for the emitter-cosine weighting stock SSGI lacks. Tune ~1-4. Only takes effect when SSGI Method == SSILVB.",
				ClampMin = "0.0", ClampMax = "20.0",
				EditCondition = "SSGIMode == ECustomSSGIMode::SSILVB"))
	float SSILVBIntensity = 2.0f;

	// =================================================================
	// Volumetric Fog
	//
	// All defaults match the engine baseline so a fresh install renders
	// byte-equivalent to stock UE 5.7 volumetric fog.  See the patched
	// VolumetricFog.usf for full per-mode documentation.
	// =================================================================

	/** Volumetric fog phase function. */
	UPROPERTY(config, EditAnywhere, Category = "Volumetric Fog",
		meta = (DisplayName = "Phase Function"))
	ECustomVolPhaseMode VolPhaseMode = ECustomVolPhaseMode::EngineDefault;

	/**
	 * Self-shadowing through the fog volume.  Adds a short secondary ray-march
	 * from each froxel toward the directional light, sampling the density
	 * volume that the material pass populated earlier this frame.  This is
	 * what gives dense fog its dark interiors and visible god-ray falloff;
	 * the default engine fog only has solid-geometry shadows in this path.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Volumetric Fog",
		meta = (DisplayName = "Self-Shadow Steps"))
	ECustomVolSelfShadow VolSelfShadow = ECustomVolSelfShadow::Off;

	/**
	 * Multi-octave scattering -- adds N 'bounces' of light per directional
	 * or local light contribution.  Pairs particularly well with Self-Shadow:
	 * self-shadow darkens fog correctly, multi-scatter brings light back into
	 * the dense interior for the cinematic glow-from-within look.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Volumetric Fog",
		meta = (DisplayName = "Multi-Scattering Octaves"))
	ECustomVolMultiScatter VolMultiScatter = ECustomVolMultiScatter::Off;

	/**
	 * Phase C spectral fog tint -- approximates Rayleigh atmospheric extinction
	 * as a function of sun elevation, producing the blue-hour / golden-hour
	 * wavelength shift on directional light scattered through fog.  0 = off,
	 * 1 = physical-ish, 2 = exaggerated cinematic.  Only affects directional
	 * light contribution; sky and local lights are untouched.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Volumetric Fog",
		meta = (DisplayName = "Spectral Strength",
				ClampMin = "0.0", ClampMax = "2.0"))
	float VolSpectralStrength = 0.0f;

	/**
	 * Secondary anisotropy for Dual-Lobe HG (only used when VolPhaseMode = DualLobeHG).
	 * Range -1..1.  -0.3 produces a soft backscatter halo, +0.95 a sharp forward peak.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Volumetric Fog",
		meta = (DisplayName = "Dual-Lobe Secondary G",
				ClampMin = "-1.0", ClampMax = "1.0",
				EditCondition = "VolPhaseMode == ECustomVolPhaseMode::DualLobeHG"))
	float VolPhaseG2 = -0.3f;

	/**
	 * Blend between primary and secondary lobes for Dual-Lobe HG.
	 * 0 = primary lobe only, 1 = secondary lobe only.  Typical: 0.3.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Volumetric Fog",
		meta = (DisplayName = "Dual-Lobe Blend",
				ClampMin = "0.0", ClampMax = "1.0",
				EditCondition = "VolPhaseMode == ECustomVolPhaseMode::DualLobeHG"))
	float VolPhaseBlend = 0.3f;

	/**
	 * Droplet diameter in micrometers for Jendersie-d'Eon Mie-Approx phase.
	 * 5 = fine mist, 15 = fog, 50 = light cloud, 100+ = dense cloud.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Volumetric Fog",
		meta = (DisplayName = "Mie Droplet Diameter (μm)",
				ClampMin = "1.0", ClampMax = "200.0",
				EditCondition = "VolPhaseMode == ECustomVolPhaseMode::MieApprox"))
	float VolMieDropletDiameter = 15.0f;

	/**
	 * Schneider 2015 powder term applied in the final integration pass.
	 * 0 = engine Beer-Lambert only.  ~0.5 produces the characteristic
	 * silver-lining glow at fog/cloud edges.  Free perf.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Volumetric Fog",
		meta = (DisplayName = "Powder Edge Glow",
				ClampMin = "0.0", ClampMax = "1.0"))
	float VolPowderBlend = 0.0f;

	/**
	 * When enabled, replaces the engine's per-supersample PCG hash with a
	 * Martin Roberts R2 quasirandom sequence.  R2 has a blue-noise-like
	 * power spectrum, so TAA absorbs the frame-to-frame jitter as smooth
	 * motion rather than residual sparkle.  Zero perf cost.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Volumetric Fog",
		meta = (DisplayName = "R2 Quasirandom Jitter"))
	bool bVolBlueNoise = false;

	// =================================================================
	// Film Halation + Organic Grain (PostProcessTonemap.usf)
	//
	// Two independent post-tonemap effects synthesised into integer
	// SHADERSHIFT_* toggles by SyncEnumsToDefines.  Tunables match the
	// CUSTOM_HALATION_* / CUSTOM_GRAIN_* defines in the patched shader.
	// =================================================================

	/** Enable the post-tonemap red-highlight bleed (film halation). */
	UPROPERTY(config, EditAnywhere, Category = "Film Halation",
		meta = (DisplayName = "Enable Film Halation"))
	bool bFilmHalation = false;

	UPROPERTY(config, EditAnywhere, Category = "Film Halation",
		meta = (DisplayName = "Halation Threshold",
				ClampMin = "0.0", ClampMax = "2.0",
				EditCondition = "bFilmHalation"))
	float HalationThreshold = 0.8f;

	UPROPERTY(config, EditAnywhere, Category = "Film Halation",
		meta = (DisplayName = "Halation Strength",
				ClampMin = "0.0", ClampMax = "1.0",
				EditCondition = "bFilmHalation"))
	float HalationStrength = 0.25f;

	UPROPERTY(config, EditAnywhere, Category = "Film Halation",
		meta = (DisplayName = "Halation Spread",
				ClampMin = "0.0", ClampMax = "4.0",
				EditCondition = "bFilmHalation"))
	float HalationSpread = 1.0f;

	/** Enable midtone-weighted chromatic film grain (post-tonemap). */
	UPROPERTY(config, EditAnywhere, Category = "Organic Grain",
		meta = (DisplayName = "Enable Organic Grain"))
	bool bOrganicGrain = false;

	UPROPERTY(config, EditAnywhere, Category = "Organic Grain",
		meta = (DisplayName = "Grain Strength",
				ClampMin = "0.0", ClampMax = "1.0",
				EditCondition = "bOrganicGrain"))
	float GrainStrength = 0.12f;

	UPROPERTY(config, EditAnywhere, Category = "Organic Grain",
		meta = (DisplayName = "Grain Scale",
				ClampMin = "0.1", ClampMax = "8.0",
				EditCondition = "bOrganicGrain"))
	float GrainScale = 1.0f;

	// =================================================================
	// TAA Tweaks (TemporalAA.usf)
	// =================================================================

	/** Rescale the TAA history-clamp neighbour AABB to trade ghosting vs flicker. */
	UPROPERTY(config, EditAnywhere, Category = "Temporal AA",
		meta = (DisplayName = "Enable Ghosting Tweak"))
	bool bTaaGhostingTweak = false;

	UPROPERTY(config, EditAnywhere, Category = "Temporal AA",
		meta = (DisplayName = "Clamp Factor",
				ClampMin = "0.1", ClampMax = "4.0",
				EditCondition = "bTaaGhostingTweak",
				ToolTip = "1.0 = engine. <1 = stronger ghost rejection (tighter history clamp; on TSR also faster velocity-driven history invalidation and a weaker contrast-stability floor) at the cost of more flicker under motion. >1 = smoother motion, more ghosting. Typical anti-ghosting value 0.4-0.7."))
	float TaaClampFactor = 1.0f;

	/** Apply a cheap CAS-style sharpen after the TAA clamp. */
	UPROPERTY(config, EditAnywhere, Category = "Temporal AA",
		meta = (DisplayName = "Enable TAA Sharpen"))
	bool bTaaSharpen = false;

	UPROPERTY(config, EditAnywhere, Category = "Temporal AA",
		meta = (DisplayName = "Sharpen Strength",
				ClampMin = "0.0", ClampMax = "2.0",
				EditCondition = "bTaaSharpen"))
	float TaaSharpenStrength = 0.4f;

	// =================================================================
	// Lumen ReSTIR GI Probes (LumenScreenProbeGather.usf)
	// =================================================================

	/** Enable single-iteration spatial reservoir resample on screen-probe gather. */
	UPROPERTY(config, EditAnywhere, Category = "Lumen GI",
		meta = (DisplayName = "Enable ReSTIR Probes"))
	bool bRestirGIProbes = false;

	UPROPERTY(config, EditAnywhere, Category = "Lumen GI",
		meta = (DisplayName = "ReSTIR Strength",
				ClampMin = "0.0", ClampMax = "1.0",
				EditCondition = "bRestirGIProbes"))
	float RestirStrength = 0.5f;

	UPROPERTY(config, EditAnywhere, Category = "Lumen GI",
		meta = (DisplayName = "ReSTIR Search Radius",
				ClampMin = "1.0", ClampMax = "4.0",
				EditCondition = "bRestirGIProbes",
				ToolTip = "Spatial-reuse search radius in probe-grid tiles (~16px each). The resample adds a 4-tap cross of grid probes this many tiles out, with plane-distance rejection so energy never crosses depth edges."))
	float RestirSearchRadius = 1.0f;

	// =================================================================
	// Gradient Reflection Filter (LumenReflectionCommon.ush + Denoiser)
	// =================================================================

	UPROPERTY(config, EditAnywhere, Category = "Lumen Reflections",
		meta = (DisplayName = "Enable Gradient Reflection Filter"))
	bool bGradientReflectionFilter = false;

	UPROPERTY(config, EditAnywhere, Category = "Lumen Reflections",
		meta = (DisplayName = "Gradient Filter Strength",
				ClampMin = "0.0", ClampMax = "1.0",
				EditCondition = "bGradientReflectionFilter"))
	float GradientReflectionStrength = 0.6f;

	UPROPERTY(config, EditAnywhere, Category = "Lumen Reflections",
		meta = (DisplayName = "Roughness Bias",
				ClampMin = "0.1", ClampMax = "4.0",
				EditCondition = "bGradientReflectionFilter"))
	float GradientReflectionRoughnessBias = 1.0f;

	UPROPERTY(config, EditAnywhere, Category = "Lumen Reflections",
		meta = (DisplayName = "Sharp Trace Amount",
				ClampMin = "0.0", ClampMax = "1.0",
				EditCondition = "bGradientReflectionFilter",
				ToolTip = "How far the tracing roughness is pulled toward mirror (sharp 1-spp trace). The spatial denoiser then re-blurs to match the true material roughness via the gradient bilateral. 0 = engine trace, 1 = full mirror trace."))
	float GradientReflectionSharpTrace = 0.5f;

	// =================================================================
	// Contact-Aware VSM Shadows (VirtualShadowMapProjectionFilter.ush)
	//
	// Hooks the canonical VSM filter and uses the SMRT trace's real
	// OccluderDistance to drive PCSS-style penumbra hardening.  Only
	// affects Virtual Shadow Maps; legacy CSM / forward shadow paths
	// are untouched.
	// =================================================================

	UPROPERTY(config, EditAnywhere, Category = "Shadows",
		meta = (DisplayName = "Enable Contact-Aware VSM Shadows"))
	bool bContactAwareShadows = false;

	UPROPERTY(config, EditAnywhere, Category = "Shadows",
		meta = (DisplayName = "Hardening Strength",
				ClampMin = "0.0", ClampMax = "1.0",
				EditCondition = "bContactAwareShadows",
				ToolTip = "Lerp between engine-filtered VSM result and the contact-hardened curve. 0=engine, 1=full hardening."))
	float ContactShadowHardening = 0.5f;

	UPROPERTY(config, EditAnywhere, Category = "Shadows",
		meta = (DisplayName = "Softness Distance Scale",
				ClampMin = "0.1", ClampMax = "4.0",
				EditCondition = "bContactAwareShadows",
				ToolTip = "World-space distance scale before normalisation. 1.0 = 100cm to full softness. <1 = harder shadows, >1 = softer transitions."))
	float ContactShadowSoftness = 1.0f;

	// =================================================================
	// Shader Hooks (generic config-driven list)
	// =================================================================

	UPROPERTY(config, EditAnywhere, Category = "Shader Hooks",
		meta = (DisplayName = "Shader Overrides",
				TitleProperty = "DisplayName",
				ShowOnlyInnerProperties))
	TArray<FShaderShiftHookConfig> ShaderHooks;

	// =================================================================
	// Recompilation
	// =================================================================

	/**
	 * When enabled, changing any setting above will immediately write the
	 * patched shaders to disk and issue "recompileshaders changed".
	 * Disable this to batch multiple changes before recompiling.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Recompilation",
		meta = (DisplayName = "Auto-Recompile Shaders on Change"))
	bool bAutoRecompile = true;

	// =================================================================
	// Packaging
	// =================================================================

	/**
	 * When enabled, ShaderShift does NOT revert the patched engine shaders on
	 * editor shutdown, crash, or pre-exit, and the next editor/cook startup
	 * skips its stale-backup recovery sweep.  This is intended for project
	 * packaging - the cook commandlet (which is auto-detected and persists
	 * regardless of this checkbox) writes the patched shader output into the
	 * cooked content; keeping the patches on disk between cook runs makes
	 * iterative packaging more predictable.
	 *
	 * NOTE: While this is enabled, the engine shader files in your install
	 * remain modified.  If you uninstall the plugin without first turning
	 * this off and clicking "Restore Engine Defaults", the patched shaders
	 * will stay in place.  The .backup files written alongside the patched
	 * shaders allow ShaderShift to restore them when this option is later
	 * disabled.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Packaging",
		meta = (DisplayName = "Persist Shader Changes in Engine",
				ToolTip = "Keep patched engine shaders on disk across editor shutdown/crash and skip stale-backup recovery on next startup. Use when packaging the game. Cook commandlets automatically persist regardless of this setting."))
	bool bPersistShaderChangesInEngine = false;

	/**
	 * Manually recompile shaders.  Called by the "Recompile Shaders Now"
	 * button added via IDetailCustomization (CallInEditor does not work
	 * on UDeveloperSettings in the Project Settings panel).
	 */
	void RecompileShadersNow();

	// =================================================================
	// Helpers
	// =================================================================

	/**
	 * Sync the UENUM properties (TonemapMode, AgxLook, etc.) into the
	 * matching FShaderShiftDefineEntry::Value strings inside ShaderHooks.
	 * Called before applying hooks.
	 */
	void SyncEnumsToDefines();

	/**
	 * Seed the per-knob Callisto/Multi-Lobe tunables from the matching preset
	 * enum (no-op when the preset is Custom).  Called by SyncEnumsToDefines and
	 * by the Quick Panel preset combos so the sliders update the instant a
	 * preset is chosen.  Dragging a slider sets its preset back to Custom, so a
	 * selected preset only seeds the knobs once.
	 */
	void ApplyCallistoDiffusePresetToKnobs();
	void ApplyCallistoSpecPresetToKnobs();
	void ApplyMultiLobePresetToKnobs();
};
