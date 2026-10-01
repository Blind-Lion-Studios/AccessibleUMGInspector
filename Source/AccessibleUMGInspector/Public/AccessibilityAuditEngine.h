// Copyright Blind Lion Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AccessibleUMGInspectorTypes.h"

/**
 * Read-only accessibility audit engine for UMG user interfaces.
 * Evaluates rules A11Y-01 through A11Y-14 across serialized, runtime, and differential evidence.
 * Does not mutate any assets, widgets, packages, or engine state.
 */
class ACCESSIBLEUMGINSPECTOR_API FAccessibilityAuditEngine
{
public:
	/**
	 * Run complete audit by resolving inputs, executing underlying inspectors, and running rules.
	 * At least one of AssetPath or RuntimeWidgetReference must be non-empty.
	 *
	 * @param AssetPath               Optional asset path to Widget Blueprint.
	 * @param RuntimeWidgetReference  Optional runtime widget name, object path, or class.
	 * @param WorldContext            Optional target world context.
	 * @param RuleSet                 Optional list of rule IDs to evaluate. Empty evaluates all applicable rules.
	 * @param MaxDepth                Maximum hierarchy traversal depth (clamped to [1, 100], default 32).
	 * @param Format                  Output format: "NvdaText", "Json", or "Both" (default "Both").
	 */
	static FAccessibilityAuditReport Audit(
		const FString& AssetPath = TEXT(""),
		const FString& RuntimeWidgetReference = TEXT(""),
		const FString& WorldContext = TEXT(""),
		const TArray<FString>& RuleSet = {},
		int32 MaxDepth = 32,
		const FString& Format = TEXT("Both"));

	/**
	 * Run audit directly against pre-existing reports (allows fast, project-neutral unit testing).
	 *
	 * @param SerializedReport        Optional pointer to pre-existing serialized report.
	 * @param RuntimeReport           Optional pointer to pre-existing runtime report.
	 * @param ComparisonReport        Optional pointer to pre-existing comparison report.
	 * @param RuleSet                 Optional list of rule IDs to evaluate.
	 * @param Format                  Output format.
	 * @param OverrideAssetPath       Optional asset path override.
	 * @param OverrideRuntimeWidget   Optional runtime widget identity override.
	 * @param OverrideWorld           Optional world identity override.
	 */
	static FAccessibilityAuditReport AuditReports(
		const FSerializedWidgetBlueprintReport* SerializedReport,
		const FRuntimeWidgetReport* RuntimeReport,
		const FSerializedRuntimeComparisonReport* ComparisonReport,
		const TArray<FString>& RuleSet = {},
		const FString& Format = TEXT("Both"),
		const FString& OverrideAssetPath = TEXT(""),
		const FString& OverrideRuntimeWidget = TEXT(""),
		const FString& OverrideWorld = TEXT(""));

	static FString GetClassificationString(EAuditFindingClassification Classification);
	static FString GetConfidenceBasisString(EAuditConfidenceBasis Basis);
	static bool IsInteractiveClass(const FString& WidgetClass);

private:
	// Rule evaluation functions
	static void EvaluateA11Y01(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y02(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y03(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y04(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y05(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y06(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y07(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y08(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y09(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y10(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y11(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y12(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y13(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
	static void EvaluateA11Y14(const FSerializedWidgetBlueprintReport* S, const FRuntimeWidgetReport* R, const FSerializedRuntimeComparisonReport* C, TArray<FAuditFindingRecord>& OutFindings, TArray<FString>& OutPassed, TArray<FString>& OutNotEvaluated);
};
