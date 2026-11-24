// Copyright Hyperdyne Systems LLC 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Containers/Ticker.h"

class SHADERSHIFTEDITOR_API FShaderShiftEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void RegisterIconStyle();

	/**
	 * Called by UShaderShiftSettings::PostEditChangeProperty to write patched
	 * shaders to disk and schedule a deferred recompile on the next tick.
	 *
	 * The recompile is deferred because PostEditChangeProperty runs inside a
	 * Slate callback - issuing "recompileshaders changed" + FlushRenderingCommands
	 * from that context can deadlock or produce partial flushes.  By deferring
	 * to the next game-thread tick we match the same timing as typing the
	 * console command manually, which is known to work.
	 *
	 * Returns the number of hooks that wrote new content to disk.
	 *
	 * bIsManualApply: true for the Quick Panel "Apply" button - an explicit user
	 * action that ALWAYS recompiles, bypassing the bAutoRecompile setting (which
	 * only governs the per-edit Project Settings path).  Global vs. "changed" is
	 * decided by each hook's RequiresGlobalRecompile config flag regardless.
	 * Defaults to false.
	 */
	int32 ApplyAndRecompileIfNeeded(bool bIsManualApply = false);

	/**
	 * Force-apply all hooks and recompile immediately on the next tick,
	 * regardless of whether on-disk content changed.  Used by the manual
	 * "Recompile Shaders Now" button.
	 */
	void ForceRecompile();

	/**
	 * If persist mode is active and source control is enabled, ensure the
	 * registered engine shader files are checked out into the user's default
	 * changelist (source-engine builds track engine shaders in P4/Plastic).
	 * No-ops when persist is off, SCC is disabled, or nothing needs checkout.
	 * Exposed for the Quick Panel's System tab persist toggle.
	 */
	static void CheckOutPatchedShadersForPersist();

private:
	/** Read the plugin config and populate UShaderShiftSettings::ShaderHooks. */
	void LoadShaderHookConfigs();

	/**
	 * Push UShaderShiftSettings data into FShaderShiftHookRegistry hook PendingDefines,
	 * then call ApplyAllEnabledHooks().
	 */
	void SyncSettingsWithHooks();

	/**
	 * Schedules the actual recompile + viewport redraw on the next game-thread
	 * tick via FTSTicker.  If a recompile is already pending, the new request
	 * coalesces (bNeedsGlobalRecompile is OR'd).
	 */
	void ScheduleDeferredRecompile(bool bNeedsGlobalRecompile);

	/**
	 * The actual recompile logic that runs on a deferred tick.
	 * Flushes the shader file cache, issues the appropriate recompileshaders
	 * command(s), waits for completion, flushes rendering commands, then
	 * redraws all viewports.
	 */
	void ExecuteRecompileAndRedraw(bool bNeedsGlobalRecompile);

	TSharedPtr<FSlateStyleSet> StyleSet;

	FString PluginDirectory;
	FString ShadersDirectory;

	/** Handle for the deferred recompile ticker. */
	FTSTicker::FDelegateHandle DeferredRecompileHandle;

	/** Accumulated flag - if any pending hook needs global, we do global. */
	bool bPendingNeedsGlobalRecompile = false;
};
