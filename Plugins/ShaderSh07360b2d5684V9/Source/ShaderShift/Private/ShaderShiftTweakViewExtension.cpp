// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "ShaderShiftTweakViewExtension.h"

#include "ShaderShiftTweakSubsystem.h"
#include "ShaderShiftRuntimeParams.h"   // ShaderShiftLive slot + focus channels
#include "SceneView.h"

namespace
{
	// Bit-exact uint32 -> float (the slot lanes are raw bit patterns, always
	// finite by construction: bits [31:30] are 0 in every packed lane and the
	// marker constant's exponent field is < 0xFF).
	FORCEINLINE float ShaderShift_AsFloat(uint32 U)
	{
		float F;
		FMemory::Memcpy(&F, &U, sizeof(float));
		return F;
	}

	// Armed marker (v3, unified across all engine versions): sign bit set on
	// BOTH SpecularOverrideParameter.x and NormalOverrideParameter.x -- plain-
	// copied to the UB (clamp-safe), never negative from engine viewmode
	// writers, bit 30 stays 0 so the pattern is always finite.  This frees
	// AmbientOcclusionOverrideParameter.y (former 'SSLV' home) as a data float.
	// MUST match SSL_ARMED / SSL_NEUTRAL in the generated shader blocks.
	constexpr uint32 ShaderShift_LiveMarkerSignBit = 0x80000000u;

	// A view is hijackable only if its override members hold the engine's Lit
	// identity values.  Debug viewmodes (Lighting Only, Reflection Override,
	// VisualizeBuffer, ...) legitimately write these members BEFORE the view
	// extension runs -- leaving such views untouched means those viewmodes
	// keep working and the live preview silently pauses there instead.
	bool HasIdentityOverrides(const FSceneView& View)
	{
		return View.DiffuseOverrideParameter          == FVector4f(0.f, 0.f, 0.f, 1.f)
			&& View.SpecularOverrideParameter         == FVector4f(0.f, 0.f, 0.f, 1.f)
			&& View.NormalOverrideParameter           == FVector4f(0.f, 0.f, 0.f, 1.f)
			&& View.RoughnessOverrideParameter        == FVector2f(0.f, 1.f)
			&& View.AmbientOcclusionOverrideParameter == FVector2f(0.f, 1.f);
	}
}

FShaderShiftTweakViewExtension::FShaderShiftTweakViewExtension(
	const FAutoRegister& AutoRegister,
	UShaderShiftTweakSubsystem* InOwner)
	: FSceneViewExtensionBase(AutoRegister)
	, Owner(InOwner)
{
}

void FShaderShiftTweakViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	// ------------------------------------------------------------------------
	// Live SLOT channel: write the packed 10-bit knob lanes into the five
	// editor-only View override members.  Timing verified against 5.7 source:
	// BeginRenderViewFamily runs on the game thread AFTER SetupView and after
	// GameViewportClient / FEditorViewportClient::SetupViewForRendering have
	// applied their viewmode parameters (we are the LAST writer), and BEFORE
	// the renderer copies FSceneView into FViewInfo -- so these values land in
	// this frame's View uniform buffer (SceneRenderBuilder.cpp:508 ->
	// SceneRendering.cpp:2680).
	//
	// The engine only reads these members in USE_DEVELOPMENT_SHADERS viewmode
	// transforms, every one of which carries an SSL_NEUTRAL guard in the
	// patched shaders, so writing arbitrary bit patterns here is inert outside
	// ShaderShift's own SSL_KNOB decode.
	// ------------------------------------------------------------------------
	uint32 SlotBits[ShaderShiftLive::LiveSlotFloatCount];
	if (!ShaderShiftLive::GetLiveSlots(SlotBits))
	{
		return; // disarmed: every view keeps its engine-set (identity) values
	}

	for (int32 ViewIndex = 0; ViewIndex < InViewFamily.Views.Num(); ++ViewIndex)
	{
		const FSceneView* ConstView = InViewFamily.Views[ViewIndex];
		if (!ConstView)
		{
			continue;
		}

		// Captures bake into persistent assets / probes -- they must always
		// see identity override values, never live-preview data.
		if (ConstView->bIsSceneCapture || ConstView->bIsReflectionCapture || ConstView->bIsPlanarReflection)
		{
			continue;
		}

		// Respect legitimately-set debug viewmodes (see HasIdentityOverrides).
		if (!HasIdentityOverrides(*ConstView))
		{
			continue;
		}

		// FSceneViewFamily::Views stores const pointers, but at this point the
		// views are this frame's mutable game-thread objects (the engine's own
		// SetupViewForRendering mutates them the same way moments earlier).
		FSceneView* View = const_cast<FSceneView*>(ConstView);

		// CLAMP-SAFE CARRIERS ONLY (v3 banked).  FSceneView::SetupCommonView-
		// UniformBufferParameters (SceneView.cpp, editor builds) rewrites
		// Diffuse (X=Y=Z=max(X,r.DiffuseColor.Min), W=min(X+W,r.DiffuseColor
		// .Max)-newX) and Roughness (X=max(X,r.Roughness.Min), Y=min(X+Y,
		// r.Roughness.Max)-newX) BEFORE filling the View UB -- bit patterns in
		// Diffuse.yzw or Roughness.y are destroyed.  Safe floats, bit-exact
		// through SetupCommon: Specular.xyzw + Normal.xyzw + AO.xy (untouched)
		// and Diffuse.x + Roughness.x (max(X,0) identity for sign-0 patterns).
		// Float order mirrors the SSL_KNOB_* macros and gen.inl lanes:
		//   floats 0-3 = Specular.xyzw   (lanes 0-11:  diffuse + spec banks)
		//   floats 4-7 = Normal.xyzw     (lanes 12-23: banks, CTRL 21, fog)
		//   float  8   = Diffuse.x       (lanes 24-26: fog, grain)
		//   float  9   = Roughness.x     (lanes 27-29: fog, halation pair)
		//   float 10   = AO.x            (lanes 30-32: SSGI, ReSTIR, grad)
		//   float 11   = AO.y            (lanes 33-35: grad-sharp, TAA pair)
		// Mode-exclusive BRDF knobs SHARE bank lanes; the CTRL lane says which
		// diffuse/spec mode owns them (see gen.inl).  The armed marker rides
		// the SIGN BITS of floats 0 and 4 on top of the lane data.  Diffuse
		// .yzw / Roughness.y are engine-identity constants: SetupCommon
		// mangles whatever sits there, and every consuming site is guarded
		// while armed, so identity keeps the disarmed math exact.
		View->SpecularOverrideParameter = FVector4f(
			ShaderShift_AsFloat(SlotBits[0] | ShaderShift_LiveMarkerSignBit),
			ShaderShift_AsFloat(SlotBits[1]),
			ShaderShift_AsFloat(SlotBits[2]),  ShaderShift_AsFloat(SlotBits[3]));
		View->NormalOverrideParameter = FVector4f(
			ShaderShift_AsFloat(SlotBits[4] | ShaderShift_LiveMarkerSignBit),
			ShaderShift_AsFloat(SlotBits[5]),
			ShaderShift_AsFloat(SlotBits[6]),  ShaderShift_AsFloat(SlotBits[7]));
		View->DiffuseOverrideParameter = FVector4f(
			ShaderShift_AsFloat(SlotBits[8]), 0.0f, 0.0f, 1.0f);
		View->RoughnessOverrideParameter = FVector2f(
			ShaderShift_AsFloat(SlotBits[9]), 1.0f);
		View->AmbientOcclusionOverrideParameter = FVector2f(
			ShaderShift_AsFloat(SlotBits[10]),
			ShaderShift_AsFloat(SlotBits[11]));
	}
}
