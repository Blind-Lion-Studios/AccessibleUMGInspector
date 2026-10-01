// Copyright Blind Lion Studios. All Rights Reserved.

#include "Modules/ModuleManager.h"

class FAccessibleUMGInspectorTestsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FAccessibleUMGInspectorTestsModule, AccessibleUMGInspectorTests)
