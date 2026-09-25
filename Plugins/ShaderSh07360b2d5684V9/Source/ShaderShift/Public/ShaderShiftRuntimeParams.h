// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.
//
// ============================================================================
// ShaderShift Scene-UB parameter channel (DORMANT infrastructure)
//
// FShaderShiftSceneParameters is registered as a modular member of the
// engine's Scene uniform buffer (IMPLEMENT_SCENE_UB_STRUCT) -- the same
// mechanism GPUScene/Nanite use.  Shaders in Scene-bound passes (base pass,
// mesh passes, Lumen tracing) can read:
//
//     Scene.ShaderShift.SwitchBits
//     Scene.ShaderShift.FloatParams0.x ...
//
// The member's default-value factory reads a render-thread mirror each time
// a scene renderer builds its FSceneUniformBuffer (per frame), so values
// pushed from the game thread via ShaderShiftRuntime::PushSceneState are
// visible to shaders the next frame -- no recompile, no binding code.
//
// NO SHADER CONSUMES THIS YET.  It exists as the ready-made transport for
// future runtime-tweakable parameters whose consumers live in mesh/material
// passes (e.g. SSDM tunables in BasePassPixelShader/MaterialTemplate, the
// Callisto per-material override).  IMPORTANT COVERAGE LIMIT, verified
// against 5.7 engine source: the Scene UB is NOT bound by the deferred-light
// pass, the post-process chain, TAA/TSR, VSM projection, the Lumen
// integrate/denoiser passes, or the volumetric-fog scattering passes --
// parameters consumed there cannot use this channel (RDG rebinds static
// uniform buffers per pass strictly from each pass's own parameter struct;
// a plugin cannot extend engine pass parameter structs).
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "ShaderParameterMacros.h"

// Registered as Scene.ShaderShift.  Members must not be bool (HLSL bool
// layout != C++); pack flags into SwitchBits.
BEGIN_SHADER_PARAMETER_STRUCT(FShaderShiftSceneParameters, SHADERSHIFT_API)
	SHADER_PARAMETER(uint32,    SwitchBits)     // up to 32 packed bools / bitfields
	SHADER_PARAMETER(uint32,    Pad0)
	SHADER_PARAMETER(uint32,    Pad1)
	SHADER_PARAMETER(uint32,    Pad2)
	SHADER_PARAMETER(FVector4f, FloatParams0)   // 4 float lanes
	SHADER_PARAMETER(FVector4f, FloatParams1)   // 4 more; grow as needed
END_SHADER_PARAMETER_STRUCT()

namespace ShaderShiftRuntime
{
	/**
	 * Push new Scene-UB lane values.  Game thread only; enqueues a render
	 * command that updates the mirror the per-frame factory reads.
	 */
	SHADERSHIFT_API void PushSceneState(
		uint32 SwitchBits, const FVector4f& Float0, const FVector4f& Float1);
}

// ============================================================================
// ShaderShiftLive -- the live tweak channels (EDITOR look-dev).
//
// TWO cooperating transports give silent full-liveness (every slider live,
// simultaneously, no recompile):
//
//   FOCUS lane (the two tweak cvars):
//     View.GeneralPurposeTweak  (lane A) = dragged parameter's integer ID
//     View.GeneralPurposeTweak2 (lane B) = its FULL-PRECISION live value
//   SLOT lanes (View *OverrideParameter members, per-view, see below):
//     every OTHER dirty knob at 10-bit precision, all at once.
//
// Every patched float site compiled with SHADERSHIFT_LIVE_TWEAKS resolves
//   focus lane (id match)  ->  slot lane (armed + code != 1023)  ->  baked
// via the generated SSL_KNOB_<id> macros.  Modes/features stay compile-time.
//
// View is bound in every pass ShaderShift patches, so this reaches the
// post-process chain, the deferred BRDF, TAA/TSR, VSM, and Lumen alike --
// the GeneralPurposeTweak member is an UNCONDITIONAL View UB field (only the
// cvar that fills it is shipping-gated), so the shader read never causes a
// compile error in any build.
//
// EDITOR ONLY: the fill cvars are compiled out of Shipping/Test by the
// engine, and SHADERSHIFT_LIVE_TWEAKS is set to 0 for packaged builds, so
// cooked content falls back to the compile-time defines.  All calls are
// game-thread; the engine mirrors the render-thread-safe cvars itself.
// ============================================================================
namespace ShaderShiftLive
{
	// Parameter IDs.  MUST stay in sync with the literal IDs passed to
	// ShaderShift_LiveFloat() in the patched shaders.  ID 0 is reserved as
	// the DISARMED sentinel (matches no parameter -> everything uses its
	// compile-time default).  Grow this list as more float sites are wired.
	enum ELiveParamId : int32
	{
		Live_None              = 0,   // never armed (reserved)
		// ID 1 is RETIRED and must never be reassigned: the engine default AND
		// the hard-coded Shipping value of r.GeneralPurposeTweak are both 1.0
		// (SceneRendering.cpp:1843), so an ID of 1 would spuriously read as
		// armed before InitDisarmed runs / in a build where the live gate
		// leaked.  The disarmed sentinel written by InitDisarmed()/Disarm()
		// is therefore 1.0 (stock-identical), and real IDs start at 2.
		// Halation Strength (formerly 1) is now Live_HalationStrength = 43.
		Live_Retired_1         = 1,
		Live_GrainStrength     = 2,
		// id 3 fully retired 2026-07-10 (Live Diffuse Intensity test knob removed).

		// --- Real BRDF tunables (resolved via BRDF.ush global indirection) -----
		// Multi-Lobe Cinematic specular (CUSTOM_SPEC_MODE == 1)
		Live_SpecPrimaryWeight         = 4,   // CUSTOM_SPEC_PRIMARY_WEIGHT
		Live_SpecPrimaryRoughScale     = 5,   // CUSTOM_SPEC_PRIMARY_ROUGHNESS_SCALE
		Live_SpecSecondaryRoughScale   = 6,   // CUSTOM_SPEC_SECONDARY_ROUGHNESS_SCALE
		Live_SpecSecondaryMaxRough     = 7,   // CUSTOM_SPEC_SECONDARY_MAX_ROUGHNESS
		// Callisto Modified Dual GGX specular (CUSTOM_SPEC_MODE == 2)
		Live_CallistoSpecFresnelFalloff = 8,  // CALLISTO_SPEC_FRESNEL_FALLOFF
		Live_CallistoDualSpecRoughScale = 9,  // CALLISTO_DUAL_SPEC_ROUGHNESS_SCALE
		Live_CallistoDualSpecOpacity    = 10, // CALLISTO_DUAL_SPEC_OPACITY
		// Callisto diffuse (CUSTOM_DIFFUSE_MODE == 5)
		Live_CallistoDiffFresnel        = 11, // CALLISTO_DIFF_FRESNEL
		Live_CallistoDiffFresnelFalloff = 12, // CALLISTO_DIFF_FRESNEL_FALLOFF
		Live_CallistoDiffFresnelTanFall = 13, // CALLISTO_DIFF_FRESNEL_TAN_FALLOFF
		Live_CallistoDiffFresnelTintR   = 14, // CALLISTO_DIFF_FRESNEL_TINT_R
		Live_CallistoDiffFresnelTintG   = 15, // CALLISTO_DIFF_FRESNEL_TINT_G
		Live_CallistoDiffFresnelTintB   = 16, // CALLISTO_DIFF_FRESNEL_TINT_B
		Live_CallistoRetro              = 17, // CALLISTO_RETRO
		Live_CallistoRetroFalloff       = 18, // CALLISTO_RETRO_FALLOFF
		Live_CallistoRetroTanFalloff    = 19, // CALLISTO_RETRO_TAN_FALLOFF
		Live_CallistoRetroTintR         = 20, // CALLISTO_RETRO_TINT_R
		Live_CallistoRetroTintG         = 21, // CALLISTO_RETRO_TINT_G
		Live_CallistoRetroTintB         = 22, // CALLISTO_RETRO_TINT_B
		Live_CallistoSmoothTerm         = 23, // CALLISTO_SMOOTH_TERM
		Live_CallistoSmoothTermLength   = 24, // CALLISTO_SMOOTH_TERM_LENGTH
		Live_CallistoSmoothTermTintR    = 25, // CALLISTO_SMOOTH_TERM_TINT_R
		Live_CallistoSmoothTermTintG    = 26, // CALLISTO_SMOOTH_TERM_TINT_G
		Live_CallistoSmoothTermTintB    = 27, // CALLISTO_SMOOTH_TERM_TINT_B
		// Proxima diffuse (CUSTOM_DIFFUSE_MODE == 6)
		Live_ProximaAlphaScale          = 28, // CALLISTO_PROXIMA_ALPHA_SCALE

		// === Other-tab tunables ===
		// Focus-lane ids for the non-BRDF knobs; each also owns a 10-bit slot
		// lane (see the Live SLOT channel below), so all of them can preview
		// simultaneously.  The legacy NEGATIVE-lane packed scheme is retired.
		Live_HalationThreshold          = 29, // CUSTOM_HALATION_THRESHOLD       (Color/PostProcessTonemap.usf)
		Live_SSILVBIntensity            = 30, // CUSTOM_SSGI_INTENSITY           (GI/SSRTDiffuseIndirect.usf)
		Live_RestirStrength             = 31, // CUSTOM_RESTIR_STRENGTH          (GI/LumenScreenProbeGather.usf)
		Live_GradReflStrength           = 32, // CUSTOM_GRAD_REFL_STRENGTH       (GI/LumenReflectionDenoiserSpatial.usf)
		Live_GradReflSharpTrace         = 33, // CUSTOM_GRAD_REFL_SHARP_TRACE    (GI/LumenReflectionCommon.ush)
		Live_VolSpectralStrength        = 34, // CUSTOM_VOL_SPECTRAL_STRENGTH    (Fog, LightScatteringCS scope)
		Live_VolPhaseG2                 = 35, // CUSTOM_VOL_PHASE_G2             (Fog, shared PhaseFunction)
		Live_VolPhaseBlend              = 36, // CUSTOM_VOL_PHASE_BLEND          (Fog, shared PhaseFunction)
		Live_VolMieDroplet              = 37, // CUSTOM_VOL_MIE_DROPLET_UM       (Fog, shared PhaseFunction)
		Live_VolPowderBlend             = 38, // CUSTOM_VOL_POWDER_BLEND         (Fog, FinalIntegrationCS -> ViewUniformBuffer)
		Live_TaaClampFactor             = 39, // CUSTOM_TAA_CLAMP_FACTOR         (AA/TemporalAA.usf + TSRUpdateHistory.usf)
		Live_TaaSharpenStrength         = 40, // CUSTOM_TAA_SHARPEN_STRENGTH     (AA/TemporalAA.usf)
		Live_ContactShadowHardening     = 41, // CUSTOM_SHADOW_HARDENING_STRENGTH    (AA/VSMProjectionFilter.ush)
		Live_ContactShadowSoftness      = 42, // CUSTOM_SHADOW_SOFTNESS_MULTIPLIER   (AA/VSMProjectionFilter.ush)
		Live_HalationStrength           = 43, // CUSTOM_HALATION_STRENGTH        (Color/PostProcessTonemap.usf; moved off retired ID 1)

		// Regolith micrograin dust layer (BRDF group; gSSL_ globals via the
		// View-safe setters).  Slots 41-44 -- the live-slot budget is now FULL
		// (45/45); the next knob needs the 10:10:10 packing upgrade.
		Live_RegolithCoverage           = 44, // REGOLITH_COVERAGE
		Live_RegolithGrainRoughness     = 45, // REGOLITH_GRAIN_ROUGHNESS
		Live_RegolithRetro              = 46, // REGOLITH_RETRO
		Live_RegolithSheen              = 47, // REGOLITH_SHEEN
	};

	// =========================================================================
	// Live SLOT channel -- the full-liveness transport (2026-07 rework).
	//
	// The focus lane above carries ONE full-precision value (the knob being
	// dragged right now).  Every OTHER dirty knob rides the editor-only View
	// override members.  CLAMP-SAFE NOTE (v2, 2026-07):
	// FSceneView::SetupCommonViewUniformBufferParameters (editor builds)
	// rewrites Diffuse.yzw and Roughness.y against r.DiffuseColor.Min/Max and
	// r.Roughness.Min/Max BEFORE the View UB is filled -- bit patterns stored
	// there are destroyed.  The transport therefore uses ONLY the 11 floats
	// that survive bit-exact: Specular (float4) + Normal (float4) + AO.x
	// (untouched) and Diffuse.x + Roughness.x (max(X,0) is identity for
	// sign-0 patterns) -- 33 lanes of 3 x 10-bit slot codes (bits [29:0];
	// bits [31:30] stay 0 so the pattern is always a finite float).  Slot
	// code 1023 = clean -> shader uses its baked #define; 0..1022 = quantized
	// over the knob's [lo,hi] from the generated table, as 3 x 10-bit lane codes (bits [29:0]; slot code 1023
	// = clean -> shader uses its baked #define; 0..1022 = quantized over the
	// knob's [lo,hi]).  v3 BANKED LAYOUT: mode-exclusive BRDF knobs SHARE
	// lanes -- diffuse bank (lanes 0-16: Callisto 17 / Proxima 1 / Regolith 3)
	// and spec bank (lanes 17-20: multi-lobe 4 / Callisto dual 3 / Regolith 3);
	// the CTRL lane (21) carries DiffuseMode | SpecularMode<<4 so the decode
	// macros gate banked knobs on ownership (non-owner knobs read baked, never
	// a foreign code).  Dual-axis Regolith coverage/grain-roughness try the
	// spec bank then the diffuse bank.  Lanes 22-29 = fog quintet, grain,
	// halation pair; lanes 30-35 (AO.xy) = SSGI, ReSTIR, gradient pair,
	// TAA pair.  Armed marker = SIGN BIT of BOTH Specular.x and Normal.x
	// (v3: unified with 5.4/5.5, freeing the former AO.y 'SSLV' float for
	// data).  Only the shadow pair (ids 41/42) is focus-lane-only here.
	//
	// The editor module packs these from UShaderShiftSettings (see
	// ShaderShiftLiveKnobTable.gen.inl + Build/gen_live_slots.py, which also
	// generates the matching SSL_KNOB_* shader macros -- regenerate together).
	// FShaderShiftTweakViewExtension writes them per-view in
	// BeginRenderViewFamily (last game-thread writer, verified vs 5.7), and
	// only into identity views (skips scene/reflection/planar captures and any
	// view a debug viewmode legitimately configured).  Engine consumption of
	// these members is dev-only viewmode transforms, all patched with
	// SSL_NEUTRAL guards, so arming is engine-inert outside our own decode.
	// =========================================================================

	/** Number of raw 32-bit slot lanes -- ONLY the clamp-safe View floats
	    (Spec4 + Normal4 + Diffuse.x + Roughness.x + AO.xy; the former AO.y
	    'SSLV' marker float carries data since the v3 sign-bit marker). */
	inline constexpr int32 LiveSlotFloatCount = 12;

	/**
	 * Publish the packed slot lanes (game thread).  bArmed=false parks the
	 * channel: the view extension stops writing and every view keeps its
	 * engine-set override values (identity in Lit), so all knobs read baked.
	 */
	SHADERSHIFT_API void SetLiveSlots(const uint32 SlotBits[LiveSlotFloatCount], bool bArmed);

	/** Read the current slot lanes (game thread).  Returns the armed flag. */
	SHADERSHIFT_API bool GetLiveSlots(uint32 OutSlotBits[LiveSlotFloatCount]);

	/**
	 * Park the focus lane at the DISARMED SENTINEL 1.0 -- the engine default
	 * (and the hard-coded Shipping constant), so a disarmed editor is
	 * stock-identical and no ID can spuriously match (IDs 0/1 never armed).
	 * Call at editor startup and whenever the focus knob is released.
	 */
	SHADERSHIFT_API void InitDisarmed();

	/** Arm parameter ParamId with Value (lane A = ID, lane B = Value). */
	SHADERSHIFT_API void SetArmedFloat(int32 ParamId, float Value);

	/** Disarm the focus lane (lane A = 1.0 sentinel); slot lanes are unaffected. */
	SHADERSHIFT_API void Disarm();
}
