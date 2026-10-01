// Copyright Blind Lion Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "AccessibleUMGInspectorTypes.h"
#include "AccessibleUMGInspectorToolset.generated.h"

/**
 * Accessible UMG Inspector Toolset for Unreal MCP.
 *
 * Exposes read-only, accessibility-first serialized and runtime inspection
 * operations for blind and visually impaired developers. Registered via UToolsetRegistry.
 */
UCLASS(BlueprintType, MinimalAPI)
class UAccessibleUMGInspectorToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:

	/**
	 * Returns inspector plugin version, supported engine version, operations, and safety guarantees.
	 */
	UFUNCTION(meta = (AICallable), Category = "AccessibleUMGInspector")
	static ACCESSIBLEUMGINSPECTOR_API FInspectorCapabilities GetInspectorCapabilities();

	/**
	 * Performs a strict, read-only inspection of a Widget Blueprint asset, emitting
	 * structured JSON and concise NVDA-oriented plain text.
	 *
	 * @param AssetPath  Full asset path (e.g. "/Game/UI/WBP_Menu"). Must not be empty.
	 * @param Format     Output format: "NvdaText", "Json", or "Both" (default).
	 */
	UFUNCTION(meta = (AICallable), Category = "AccessibleUMGInspector")
	static ACCESSIBLEUMGINSPECTOR_API FSerializedWidgetBlueprintReport InspectSerializedWidgetBlueprint(const FString& AssetPath, const FString& Format = TEXT("Both"));

	/**
	 * Performs a strict, read-only inspection of a live runtime UUserWidget in the viewport or PIE session,
	 * capturing active visibility, passive Slate geometry, focus states, and accessibility properties.
	 *
	 * @param WidgetReference Optional widget name, path, or class. If empty, resolves single top-level user widget.
	 * @param WorldContext    Optional world identifier (PIE/Game). If empty, resolves active PIE/Game world.
	 * @param MaxDepth        Maximum hierarchy depth to inspect (clamped to [1, 100], default 32).
	 * @param Format          Output format: "NvdaText", "Json", or "Both" (default).
	 */
	UFUNCTION(meta = (AICallable), Category = "AccessibleUMGInspector")
	static ACCESSIBLEUMGINSPECTOR_API FRuntimeWidgetReport InspectRuntimeUserWidget(
		const FString& WidgetReference = TEXT(""),
		const FString& WorldContext = TEXT(""),
		int32 MaxDepth = 32,
		const FString& Format = TEXT("Both"));

	/**
	 * Reconciles serialized Widget Blueprint hierarchy against active runtime Viewport / UserWidget state,
	 * producing an authoritative differential analysis of matched, ambiguous, serialized-only, and runtime-only nodes.
	 *
	 * @param AssetPath               Full asset path to Widget Blueprint (e.g. "/Game/UI/WBP_Menu"). Must not be empty.
	 * @param RuntimeWidgetReference  Optional target widget name, object path, or class. If empty, resolves single top-level user widget.
	 * @param WorldContext            Optional target world name or path. If empty, resolves active PIE/Game world.
	 * @param MaxDepth                Maximum hierarchy traversal depth (clamped to [1, 100], default 32).
	 * @param Format                  Output format: "NvdaText", "Json", or "Both" (default).
	 */
	UFUNCTION(meta = (AICallable), Category = "AccessibleUMGInspector")
	static ACCESSIBLEUMGINSPECTOR_API FSerializedRuntimeComparisonReport CompareSerializedAndRuntime(
		const FString& AssetPath,
		const FString& RuntimeWidgetReference = TEXT(""),
		const FString& WorldContext = TEXT(""),
		int32 MaxDepth = 32,
		const FString& Format = TEXT("Both"));

	/**
	 * Performs a comprehensive accessibility audit across serialized, runtime, and differential evidence,
	 * evaluating rules A11Y-01 through A11Y-14 without mutating any assets, widgets, or engine state.
	 *
	 * @param AssetPath               Optional asset path to Widget Blueprint.
	 * @param RuntimeWidgetReference  Optional target widget name, object path, or class.
	 * @param WorldContext            Optional target world name or path.
	 * @param RuleSet                 Optional list of rule IDs to evaluate. Empty evaluates all applicable rules.
	 * @param MaxDepth                Maximum hierarchy traversal depth (clamped to [1, 100], default 32).
	 * @param Format                  Output format: "NvdaText", "Json", or "Both" (default).
	 */
	UFUNCTION(meta = (AICallable), Category = "AccessibleUMGInspector")
	static ACCESSIBLEUMGINSPECTOR_API FAccessibilityAuditReport AuditWidgetAccessibility(
		const FString& AssetPath,
		const FString& RuntimeWidgetReference,
		const FString& WorldContext,
		const TArray<FString>& RuleSet,
		int32 MaxDepth = 32,
		const FString& Format = TEXT("Both"));

	static ACCESSIBLEUMGINSPECTOR_API FAccessibilityAuditReport AuditWidgetAccessibility(
		const FString& AssetPath = TEXT(""),
		const FString& RuntimeWidgetReference = TEXT(""))
	{
		return AuditWidgetAccessibility(AssetPath, RuntimeWidgetReference, TEXT(""), {}, 32, TEXT("Both"));
	}
};
