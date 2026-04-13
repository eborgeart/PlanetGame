// Copyright 2025 Dmitry Karpukhin. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "CMAA2Subsystem.generated.h"

UCLASS()
class UCMAA2Subsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TSharedPtr<class FCMAA2SceneViewExtension, ESPMode::ThreadSafe> CMAA2SceneViewExtension;
};