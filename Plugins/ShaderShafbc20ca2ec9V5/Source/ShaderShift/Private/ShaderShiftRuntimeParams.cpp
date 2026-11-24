// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#include "ShaderShiftRuntimeParams.h"

#include "HAL/IConsoleManager.h"
#include "RenderingThread.h"
#include "SceneUniformBuffer.h"

DEFINE_LOG_CATEGORY_STATIC(LogShaderShiftLive, Log, All);

// ============================================================================
// Scene-UB channel (see header for the architecture + coverage notes).
//
// The default-value factory runs whenever a scene renderer builds its
// FSceneUniformBuffer and no explicit value was set -- i.e. every frame, on
// the render thread -- so reading our render-thread mirror here IS the
// per-frame update path.  No binding code, no view-extension dependency.
// ============================================================================

namespace
{
	// Written only from render commands enqueued by PushSceneState; read only
	// by the factory (render thread).
	struct FShaderShiftRenderThreadMirror
	{
		uint32    SwitchBits = 0;
		FVector4f Float0     = FVector4f(0, 0, 0, 0);
		FVector4f Float1     = FVector4f(0, 0, 0, 0);
	};
	FShaderShiftRenderThreadMirror GShaderShiftRTMirror;
}

static void GetShaderShiftSceneDefaults(FShaderShiftSceneParameters& Out, FRDGBuilder& GraphBuilder)
{
	Out.SwitchBits   = GShaderShiftRTMirror.SwitchBits;
	Out.Pad0         = 0;
	Out.Pad1         = 0;
	Out.Pad2         = 0;
	Out.FloatParams0 = GShaderShiftRTMirror.Float0;
	Out.FloatParams1 = GShaderShiftRTMirror.Float1;
}

// DECLARE introduces SceneUB::ShaderShift so the IMPLEMENT macro's
// qualified definition compiles (same pattern as the engine's GPUScene /
// Nanite members, which DECLARE in their headers).
DECLARE_SCENE_UB_STRUCT(FShaderShiftSceneParameters, ShaderShift, )
IMPLEMENT_SCENE_UB_STRUCT(FShaderShiftSceneParameters, ShaderShift, GetShaderShiftSceneDefaults);

namespace ShaderShiftRuntime
{

void PushSceneState(uint32 SwitchBits, const FVector4f& Float0, const FVector4f& Float1)
{
	check(IsInGameThread());

	FShaderShiftRenderThreadMirror NewMirror;
	NewMirror.SwitchBits = SwitchBits;
	NewMirror.Float0     = Float0;
	NewMirror.Float1     = Float1;

	ENQUEUE_RENDER_COMMAND(ShaderShiftUpdateSceneParams)(
		[NewMirror](FRHICommandListImmediate&)
		{
			GShaderShiftRTMirror = NewMirror;
		});
}

} // namespace ShaderShiftRuntime

// ============================================================================
// ShaderShiftLive -- dynamic-lane live float channel (see header).
// ============================================================================
namespace ShaderShiftLive
{
	namespace
	{
		// Cached so we don't do a name lookup every slider tick.  The cvars
		// are render-thread-safe; the engine mirrors them itself.  Both are
		// absent in Shipping/Test (compiled out) -- harmless no-ops there.
		IConsoleVariable* GetIdCVar()
		{
			static IConsoleVariable* CVar =
				IConsoleManager::Get().FindConsoleVariable(TEXT("r.GeneralPurposeTweak"));
			return CVar;
		}
		IConsoleVariable* GetValueCVar()
		{
			static IConsoleVariable* CVar =
				IConsoleManager::Get().FindConsoleVariable(TEXT("r.GeneralPurposeTweak2"));
			return CVar;
		}
	}

	// Disarmed sentinel for lane A.  1.0 == the engine default AND the
	// hard-coded Shipping fill (SceneRendering.cpp:1843), so a disarmed editor
	// is bit-identical to a stock engine and no leaked-gate build can ever see
	// a spuriously armed ID.  IDs 0 and 1 are permanently reserved/never armed.
	static constexpr float DisarmedSentinel = 1.0f;

	void InitDisarmed()
	{
		check(IsInGameThread());
		if (IConsoleVariable* IdCVar = GetIdCVar())
		{
			IdCVar->Set(DisarmedSentinel, ECVF_SetByConsole);
		}
		if (IConsoleVariable* ValueCVar = GetValueCVar())
		{
			ValueCVar->Set(1.0f, ECVF_SetByConsole);
		}
	}

	void SetArmedFloat(int32 ParamId, float Value)
	{
		check(IsInGameThread());
		IConsoleVariable* IdCVar    = GetIdCVar();
		IConsoleVariable* ValueCVar = GetValueCVar();
		if (!IdCVar || !ValueCVar)
		{
			// Shipping/Test: cvars compiled out.  Live preview is an editor
			// feature; cooked builds use the compile-time defines.
			return;
		}

		// Set value first, then ID, so the frame that first sees the new ID
		// already sees the matching value (avoids a 1-frame flash of the old
		// armed parameter's value bleeding into the new one).
		ValueCVar->Set(Value, ECVF_SetByConsole);
		IdCVar->Set(float(ParamId), ECVF_SetByConsole);

		UE_LOG(LogShaderShiftLive, VeryVerbose,
			TEXT("Armed live param %d = %.4f"), ParamId, Value);
	}

	void Disarm()
	{
		check(IsInGameThread());
		if (IConsoleVariable* IdCVar = GetIdCVar())
		{
			IdCVar->Set(DisarmedSentinel, ECVF_SetByConsole);
		}
	}

	// ------------------------------------------------------------------------
	// Live SLOT channel state (see header).  Written by the editor module's
	// pack (slider drags / bake / revert), read by FShaderShiftTweakViewExtension
	// in BeginRenderViewFamily.  Both are game-thread, so no synchronisation.
	// ------------------------------------------------------------------------
	namespace
	{
		struct FLiveSlotState
		{
			uint32 SlotBits[LiveSlotFloatCount] = {};
			bool   bArmed = false;
		};
		FLiveSlotState GLiveSlotState;
	}

	void SetLiveSlots(const uint32 SlotBits[LiveSlotFloatCount], bool bArmed)
	{
		check(IsInGameThread());
		if (SlotBits)
		{
			FMemory::Memcpy(GLiveSlotState.SlotBits, SlotBits,
				sizeof(uint32) * LiveSlotFloatCount);
		}
		GLiveSlotState.bArmed = bArmed && (SlotBits != nullptr);
	}

	bool GetLiveSlots(uint32 OutSlotBits[LiveSlotFloatCount])
	{
		check(IsInGameThread());
		if (OutSlotBits)
		{
			FMemory::Memcpy(OutSlotBits, GLiveSlotState.SlotBits,
				sizeof(uint32) * LiveSlotFloatCount);
		}
		return GLiveSlotState.bArmed;
	}
}
