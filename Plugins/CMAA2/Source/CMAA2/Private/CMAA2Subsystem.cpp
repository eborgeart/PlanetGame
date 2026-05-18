// Copyright 2025 Dmitry Karpukhin. All Rights Reserved.

#include "CMAA2Subsystem.h"
#include "CMAA2SceneViewExtension.h"
#include "SceneViewExtension.h"

void UCMAA2Subsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	CMAA2SceneViewExtension = FSceneViewExtensions::NewExtension<FCMAA2SceneViewExtension>();
	UE_LOG(LogTemp, Log, TEXT("CMAA2Subsystem: Subsystem initialized & SceneViewExtension created"));
}

void UCMAA2Subsystem::Deinitialize()
{
	{
		CMAA2SceneViewExtension->IsActiveThisFrameFunctions.Empty();

		FSceneViewExtensionIsActiveFunctor IsActiveFunctor;

		IsActiveFunctor.IsActiveFunction = [](const ISceneViewExtension* SceneViewExtension, const FSceneViewExtensionContext& Context)
		{
			return TOptional<bool>(false);
		};

		CMAA2SceneViewExtension->IsActiveThisFrameFunctions.Add(IsActiveFunctor);
	}

	CMAA2SceneViewExtension.Reset();
	CMAA2SceneViewExtension = nullptr;
}
