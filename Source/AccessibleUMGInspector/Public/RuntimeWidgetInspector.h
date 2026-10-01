// Copyright Blind Lion Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AccessibleUMGInspectorTypes.h"

class UUserWidget;
class UWidget;
class SWidget;
class UWorld;

/**
 * Core engine for strict, read-only inspection of live runtime Viewports and UserWidgets.
 *
 * Traverses active UMG and Slate widget trees, extracting passive visibility,
 * Slate geometry, focus states, and accessibility metadata without triggering mutations,
 * synthetic inputs, or Slate construction.
 */
class ACCESSIBLEUMGINSPECTOR_API FRuntimeWidgetInspector
{
public:

	/**
	 * Inspects a live runtime UUserWidget by reference or automatically from the active world.
	 *
	 * @param WidgetReference Target widget name, object path, or class. If empty, resolves single top-level user widget.
	 * @param WorldContext    Target world name or path. If empty, resolves active PIE/Game world.
	 * @param MaxDepth        Maximum hierarchy depth to inspect (clamped to [1, 100], default 32).
	 * @param Format          Output format: "NvdaText", "Json", or "Both" (default).
	 * @return High-level inspection report with structured widget data and formatted output.
	 */
	static FRuntimeWidgetReport InspectRuntimeUserWidget(
		const FString& WidgetReference = TEXT(""),
		const FString& WorldContext = TEXT(""),
		int32 MaxDepth = 32,
		const FString& Format = TEXT("Both"));

	/**
	 * Directly inspects a known, live UUserWidget instance.
	 *
	 * @param UserWidget  Target live user widget instance. Must be valid.
	 * @param MaxDepth    Maximum hierarchy depth to inspect (clamped to [1, 100], default 32).
	 * @param Format      Output format: "NvdaText", "Json", or "Both" (default).
	 * @return High-level inspection report with structured widget data and formatted output.
	 */
	static FRuntimeWidgetReport InspectUserWidgetInstance(
		UUserWidget* UserWidget,
		int32 MaxDepth = 32,
		const FString& Format = TEXT("Both"));

	/** Resolves the target UWorld based on WorldContext parameter */
	static UWorld* ResolveTargetWorld(
		const FString& WorldContext,
		EInspectionAvailability& OutAvailability,
		FString& OutError,
		TArray<FString>& OutCandidates);

	/** Resolves the target UUserWidget based on WidgetReference parameter */
	static UUserWidget* ResolveTargetWidget(
		UWorld* World,
		const FString& WidgetReference,
		EInspectionAvailability& OutAvailability,
		FString& OutError,
		TArray<FString>& OutCandidates);

	/** Converts ESlateVisibility enum to descriptive string */
	static FString GetVisibilityString(int32 VisibilityEnumVal);

	/**
	 * Computes effective visibility, ancestor suppression, effective interaction,
	 * and effective focusability across the complete runtime widget hierarchy.
	 */
	static void ComputeEffectiveVisibility(FRuntimeWidgetReport& Report);

private:

	/** Recursively inspects a runtime UWidget node and its children */
	static int32 InspectWidgetRecursively(
		UWidget* Widget,
		const FString& ParentName,
		int32 ParentNodeIndex,
		int32 ChildIndexInParent,
		int32 CurrentDepth,
		int32 MaxDepth,
		TSet<UWidget*>& VisitedWidgets,
		const TSharedPtr<SWidget>& KeyboardFocusedSlateWidget,
		const TSharedPtr<SWidget>& UserFocusedSlateWidget,
		FRuntimeWidgetReport& OutReport);

	/** Safely extracts runtime slot properties via reflection */
	static void ExtractSlotProperties(UWidget* Widget, FAccessibleWidgetSlotRecord& OutSlot);

	/** Safely extracts runtime navigation rules */
	static void ExtractNavigationRules(UWidget* Widget, TArray<FAccessibleNavigationRecord>& OutRules);
};
