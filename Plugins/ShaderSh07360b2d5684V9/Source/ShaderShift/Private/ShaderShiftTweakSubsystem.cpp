// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "ShaderShiftTweakSubsystem.h"

#include "ShaderShiftTweakViewExtension.h"
#include "ShaderShiftRuntimeParams.h"   // ShaderShiftLive channel
#include "SceneViewExtension.h"          // FSceneViewExtensions::NewExtension
#include "Misc/App.h"                    // FApp::CanEverRender

void UShaderShiftTweakSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// The View tweak lanes default to 1.0, which would spuriously match live
	// param ID 1 (Halation) every frame; force the channel disarmed until a
	// slider / Blueprint grabs it.  Harmless no-op where the cvars are absent.
	ShaderShiftLive::InitDisarmed();

	// Only spin up the render-side view extension where we actually render -
	// skip cook / server commandlets that never build a scene renderer.
	if (FApp::CanEverRender())
	{
		ViewExtension =
			FSceneViewExtensions::NewExtension<FShaderShiftTweakViewExtension>(this);
	}
}

void UShaderShiftTweakSubsystem::Deinitialize()
{
	// Drop the extension first (releases the render-thread reference) then
	// disarm so nothing keeps feeding the View lane after teardown.
	ViewExtension.Reset();
	ShaderShiftLive::Disarm();

	Super::Deinitialize();
}

