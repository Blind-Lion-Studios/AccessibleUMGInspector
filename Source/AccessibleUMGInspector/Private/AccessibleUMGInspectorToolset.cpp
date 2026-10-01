// Copyright Blind Lion Studios. All Rights Reserved.

#include "AccessibleUMGInspectorToolset.h"
#include "SerializedWidgetInspector.h"
#include "RuntimeWidgetInspector.h"
#include "DifferentialComparator.h"
#include "AccessibilityAuditEngine.h"

FInspectorCapabilities UAccessibleUMGInspectorToolset::GetInspectorCapabilities()
{
	FInspectorCapabilities Caps;
	return Caps;
}

FSerializedWidgetBlueprintReport UAccessibleUMGInspectorToolset::InspectSerializedWidgetBlueprint(const FString& AssetPath, const FString& Format)
{
	return FSerializedWidgetInspector::InspectAsset(AssetPath, Format);
}

FRuntimeWidgetReport UAccessibleUMGInspectorToolset::InspectRuntimeUserWidget(
	const FString& WidgetReference,
	const FString& WorldContext,
	int32 MaxDepth,
	const FString& Format)
{
	return FRuntimeWidgetInspector::InspectRuntimeUserWidget(WidgetReference, WorldContext, MaxDepth, Format);
}

FSerializedRuntimeComparisonReport UAccessibleUMGInspectorToolset::CompareSerializedAndRuntime(
	const FString& AssetPath,
	const FString& RuntimeWidgetReference,
	const FString& WorldContext,
	int32 MaxDepth,
	const FString& Format)
{
	return FDifferentialComparator::Compare(AssetPath, RuntimeWidgetReference, WorldContext, MaxDepth, Format);
}

FAccessibilityAuditReport UAccessibleUMGInspectorToolset::AuditWidgetAccessibility(
	const FString& AssetPath,
	const FString& RuntimeWidgetReference,
	const FString& WorldContext,
	const TArray<FString>& RuleSet,
	int32 MaxDepth,
	const FString& Format)
{
	return FAccessibilityAuditEngine::Audit(AssetPath, RuntimeWidgetReference, WorldContext, RuleSet, MaxDepth, Format);
}

