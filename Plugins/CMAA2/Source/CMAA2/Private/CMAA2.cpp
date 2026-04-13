// Copyright 2025 Dmitry Karpukhin. All Rights Reserved.

#include "CMAA2.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "CMAA2"

void FCMAA2::StartupModule()
{
	// Set up the Shader Directories
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("CMAA2"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/CMAA2"), PluginShaderDir);
}

void FCMAA2::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCMAA2, CMAA2);