// Copyright Blind Lion Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AccessibleUMGInspectorTypes.h"

class UWidget;
class UPanelWidget;
class UWidgetBlueprint;

class ACCESSIBLEUMGINSPECTOR_API FSerializedWidgetInspector
{
public:
	/** Inspects a serialized Widget Blueprint asset without mutation */
	static FSerializedWidgetBlueprintReport InspectAsset(const FString& AssetPath, const FString& Format = TEXT("Both"));

	/** Inspects a UWidgetBlueprint instance directly (useful for tests and synthetic fixtures) */
	static FSerializedWidgetBlueprintReport InspectBlueprint(UWidgetBlueprint* Blueprint, const FString& Format = TEXT("Both"));

private:
	static int32 InspectWidgetRecursively(
		UWidget* Widget,
		const FString& ParentName,
		int32 ParentNodeIndex,
		int32 ChildIndexInParent,
		const UWidgetBlueprint* Blueprint,
		FSerializedWidgetBlueprintReport& OutReport,
		TMap<FString, int32>& OutWidgetNameCounts,
		TMap<FString, int32>& OutAccessibleNameCounts);

	static FString GetVisibilityString(int32 VisibilityEnumVal);
	static void ExtractNavigationRules(UWidget* Widget, TArray<FAccessibleNavigationRecord>& OutRules);
	static void ExtractSlotProperties(UWidget* Widget, FAccessibleWidgetSlotRecord& OutSlot);
};
