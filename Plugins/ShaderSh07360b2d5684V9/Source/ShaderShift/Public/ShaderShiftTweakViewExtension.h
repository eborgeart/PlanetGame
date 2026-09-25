// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.
//
// ============================================================================
// FShaderShiftTweakViewExtension
//
// The "push values into the renderer every frame" half of the runtime tweak
// module described in the Scene-UB design note.  This codebase's Scene-UB
// channel (FShaderShiftSceneParameters) already updates per frame via a
// render-thread mirror + a per-frame default-value factory, so the view
// extension does NOT need the doc's GetSceneUniforms().Set() binding code.
// Instead it simply refreshes the mirror once per rendered view family from
// the subsystem's authoritative game-thread values.
//
// BeginRenderViewFamily runs on the GAME thread, so calling the game-thread
// PushSceneState (which enqueues the render command itself) is safe here.
//
// NOTE on the live BRDF knob: the deferred-light pass does NOT bind the Scene
// UB in 5.7, so the diffuse-intensity test knob rides the View tweak lane
// (armed by UShaderShiftTweakSubsystem / the Quick Panel slider), not this
// channel.  This extension keeps the Scene-UB channel fed for the consumers
// that CAN read it (base pass / mesh passes / Lumen tracing).
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "SceneViewExtension.h"

class UShaderShiftTweakSubsystem;

class FShaderShiftTweakViewExtension : public FSceneViewExtensionBase
{
public:
	FShaderShiftTweakViewExtension(
		const FAutoRegister& AutoRegister,
		UShaderShiftTweakSubsystem* InOwner);

	//~ Begin ISceneViewExtension interface
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PreRenderViewFamily_RenderThread(
		FRDGBuilder& GraphBuilder, FSceneViewFamily& InViewFamily) override {}
	virtual void PreRenderView_RenderThread(
		FRDGBuilder& GraphBuilder, FSceneView& InView) override {}
	//~ End ISceneViewExtension interface

private:
	/** The engine subsystem that owns the authoritative tweak values. */
	TWeakObjectPtr<UShaderShiftTweakSubsystem> Owner;
};
