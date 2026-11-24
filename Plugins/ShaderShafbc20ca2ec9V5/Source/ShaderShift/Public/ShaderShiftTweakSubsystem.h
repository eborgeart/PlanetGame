// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.
//
// ============================================================================
// UShaderShiftTweakSubsystem
//
// The game/Blueprint-facing home for ShaderShift's real-time-editable tweak
// values -- the EngineSubsystem from the Scene-UB design note, adapted to this
// codebase's existing two transport channels:
//
//   * View tweak lane  (ShaderShiftLive)  -> reaches the deferred BRDF, post
//     process, TAA/TSR, Lumen.  Used for the live diffuse-intensity test knob.
//     Editor look-dev only (the fill cvars are compiled out of Shipping/Test).
//   * Scene UB channel (FShaderShiftSceneParameters) -> reaches base pass /
//     mesh passes / Lumen tracing.  Fed every frame by the companion
//     FShaderShiftTweakViewExtension.
//
// Drive it from Blueprints / C++ game code; the Quick Panel sliders drive the
// same live param IDs directly.  Last writer wins on the single View tweak lane.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "ShaderShiftTweakSubsystem.generated.h"

class FShaderShiftTweakViewExtension;

UCLASS()
class SHADERSHIFT_API UShaderShiftTweakSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface

	// (The Live Diffuse Intensity BRDF-test accessors that lived here were
	// retired 2026-07-10 -- the full-liveness slot transport superseded the
	// single-knob test path; see ShaderShiftRuntimeParams.h.)

private:
	/** Registers the per-view live-lane writer; created in Initialize, released in Deinitialize. */
	TSharedPtr<FShaderShiftTweakViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
