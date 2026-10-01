// Copyright Blind Lion Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AccessibleUMGInspectorTypes.h"

class ACCESSIBLEUMGINSPECTOR_API FAccessibleUMGNvdaTextFormatter
{
public:
	/** Formats a full serialized report into an NVDA-friendly plain text transcript */
	static FString FormatSerializedReport(const FSerializedWidgetBlueprintReport& Report);

	/** Formats a full runtime report into an NVDA-friendly plain text transcript */
	static FString FormatRuntimeReport(const FRuntimeWidgetReport& Report);

	/** Formats a differential comparison report into an NVDA-friendly plain text transcript */
	static FString FormatComparisonReport(const FSerializedRuntimeComparisonReport& Report);

	/** Formats an accessibility audit report into an NVDA-friendly plain text transcript */
	static FString FormatAuditReport(const FAccessibilityAuditReport& Report);

private:
	static void FormatWidgetNode(FStringBuilderBase& OutBuilder, const FSerializedWidgetBlueprintReport& Report, int32 NodeIndex, const FString& Prefix);
	static void FormatRuntimeWidgetNode(FStringBuilderBase& OutBuilder, const FRuntimeWidgetReport& Report, int32 NodeIndex, const FString& Prefix, bool bOnlyActive);
};
