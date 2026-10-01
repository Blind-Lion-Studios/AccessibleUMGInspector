// Copyright Blind Lion Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AccessibleUMGInspectorTypes.h"

/**
 * Authoritative differential comparator reconciling serialized Widget Blueprint asset
 * hierarchies against active runtime Viewport / UserWidget state.
 *
 * Implements deterministic evidence-based matching, ambiguity resolution, and descriptive
 * discrepancy tracking without package dirtying, Slate construction, or input simulation.
 */
class ACCESSIBLEUMGINSPECTOR_API FDifferentialComparator
{
public:

	/**
	 * Reconciles serialized Widget Blueprint hierarchy against active runtime Viewport / UserWidget state.
	 *
	 * @param AssetPath               Full asset path to Widget Blueprint (e.g. "/Game/UI/WBP_Menu"). Must not be empty.
	 * @param RuntimeWidgetReference  Optional target widget name, object path, or class. If empty, resolves single top-level user widget.
	 * @param WorldContext            Optional target world name or path. If empty, resolves active PIE/Game world.
	 * @param MaxDepth                Maximum hierarchy traversal depth (clamped to [1, 100], default 32).
	 * @param Format                  Output format: "NvdaText", "Json", or "Both" (default).
	 * @return Reconciled differential comparison report.
	 */
	static FSerializedRuntimeComparisonReport Compare(
		const FString& AssetPath,
		const FString& RuntimeWidgetReference = TEXT(""),
		const FString& WorldContext = TEXT(""),
		int32 MaxDepth = 32,
		const FString& Format = TEXT("Both"));

	/**
	 * Reconciles pre-existing serialized and runtime reports directly.
	 *
	 * @param SerializedReport  Pre-computed serialized widget blueprint report.
	 * @param RuntimeReport     Pre-computed runtime widget report.
	 * @param Format            Output format: "NvdaText", "Json", or "Both" (default).
	 * @return Reconciled differential comparison report.
	 */
	static FSerializedRuntimeComparisonReport ReconcileReports(
		const FSerializedWidgetBlueprintReport& SerializedReport,
		const FRuntimeWidgetReport& RuntimeReport,
		const FString& Format = TEXT("Both"));

	/** Converts comparison classification enum to string */
	static FString GetClassificationString(EWidgetComparisonClassification Classification);

private:

	/** Performs deterministic evidence-based matching across serialized and runtime nodes */
	static void MatchNodes(
		const FSerializedWidgetBlueprintReport& SerializedReport,
		const FRuntimeWidgetReport& RuntimeReport,
		TArray<FWidgetComparisonRecord>& OutRecords,
		TArray<FString>& OutAllDiscrepancies);

	/** Evaluates property and accessibility discrepancies on a matched pair */
	static void DetectDiscrepancies(
		const FSerializedWidgetRecord& S,
		const FRuntimeWidgetRecord& R,
		FWidgetComparisonRecord& OutRecord,
		TArray<FString>& OutAllDiscrepancies);

	/** Evaluates global runtime discrepancies (e.g. overlapping bounds, detached nodes) */
	static void DetectGlobalRuntimeDiscrepancies(
		const FRuntimeWidgetReport& RuntimeReport,
		TArray<FWidgetComparisonRecord>& ComparisonRecords,
		TArray<FString>& OutAllDiscrepancies);
};
