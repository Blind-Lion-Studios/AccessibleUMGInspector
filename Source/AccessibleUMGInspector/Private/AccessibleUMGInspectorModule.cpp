// Copyright Blind Lion Studios. All Rights Reserved.

#include "AccessibleUMGInspectorModule.h"
#include "AccessibleUMGInspectorToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

#define LOCTEXT_NAMESPACE "FAccessibleUMGInspectorModule"

void FAccessibleUMGInspectorModule::StartupModule()
{
	UToolsetRegistry::RegisterToolsetClass(UAccessibleUMGInspectorToolset::StaticClass());
}

void FAccessibleUMGInspectorModule::ShutdownModule()
{
	UToolsetRegistry::UnregisterToolsetClass(UAccessibleUMGInspectorToolset::StaticClass());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAccessibleUMGInspectorModule, AccessibleUMGInspector)
