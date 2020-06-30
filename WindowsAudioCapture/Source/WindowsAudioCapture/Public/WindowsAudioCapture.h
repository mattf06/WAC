//Windows Audio Capture (WAC) by KwstasG (Kostas Giannakakis)
#pragma once

#include <sdkddkver.h>

#ifndef NTDDI_THRESHOLD
#define NTDDI_THRESHOLD NTDDI_VERSION
#endif

#include "CoreMinimal.h"
//#include "Engine.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(WindowsAudioCaptureLog, Log, All);

class FWindowsAudioCaptureModule : public IModuleInterface {
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
