// Copyright Blind Lion Studios. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AccessibleUMGInspectorTypes.generated.h"

/** Describes the classification state of an inspected field or element */
UENUM(BlueprintType)
enum class EInspectionAvailability : uint8
{
	Available,
	Unavailable,
	Unsupported,
	Ambiguous
};

/** Describes a single panel slot record */
USTRUCT(BlueprintType)
struct FAccessibleWidgetSlotRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString SlotType;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ParentWidgetName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TMap<FString, FString> Properties;
};

/** Describes an explicit navigation rule on a widget */
USTRUCT(BlueprintType)
struct FAccessibleNavigationRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString Direction;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString Rule;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString TargetWidget;
};

/** Describes a delegate or event binding on a widget */
USTRUCT(BlueprintType)
struct FAccessibleWidgetBindingRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString PropertyName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString FunctionName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString BindingKind;
};

/** Describes a single serialized widget node in the hierarchy */
USTRUCT(BlueprintType)
struct FSerializedWidgetRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 NodeIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 ParentIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<int32> ChildIndices;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString WidgetName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString WidgetClass;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ObjectPath;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ParentName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 ChildIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString DefaultVisibility;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bIsEnabled = true;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bIsFocusable = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bIsVariable = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bOverrideAccessibleDefaults = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString AccessibleBehavior;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString AccessibleSummaryBehavior;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString AccessibleName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString AccessibleSummary;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bHasSlot = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FAccessibleWidgetSlotRecord Slot;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FAccessibleNavigationRecord> NavigationRules;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FAccessibleWidgetBindingRecord> Bindings;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bIsNamedSlotHost = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> ExposedNamedSlots;
};

/** High-level report for a serialized Widget Blueprint inspection */
USTRUCT(BlueprintType)
struct FSerializedWidgetBlueprintReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString AssetPath;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ParentClass;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 TotalWidgets = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 NamedSlotCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> DuplicateWidgetNames;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> DuplicateAccessibleNames;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bPackageWasCleanBefore = true;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bPackageRemainsCleanAfter = true;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 RootWidgetIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FSerializedWidgetRecord> Widgets;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString NvdaText;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString JsonData;
};

/** Screen-space bounding box for a runtime Slate/UMG widget */
USTRUCT(BlueprintType)
struct FAccessibleScreenBounds
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	float X = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	float Y = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	float Width = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	float Height = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bIsValid = false;
};

/** Describes a single runtime widget node in the active viewport/UserWidget hierarchy */
USTRUCT(BlueprintType)
struct FRuntimeWidgetRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 NodeIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 ParentIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<int32> ChildIndices;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString WidgetName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString WidgetClass;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ObjectPath;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ParentName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 ChildIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 Depth = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString LocalVisibility;

	// Deprecated / Backwards-compatibility alias: mirrors LocalVisibility
	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RuntimeVisibility;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString EffectiveVisibility;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 BlockingAncestorIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString BlockingAncestorName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bEffectivelyVisible = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bEffectivelyInteractive = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bEffectivelyFocusable = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bEffectiveGeometryAvailable = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> VisibilityEvidence;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> EffectiveStateLimitations;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bIsEnabled = true;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bIsFocusable = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bHasKeyboardFocus = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bHasUserFocus = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bIsHovered = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bOverrideAccessibleDefaults = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString AccessibleBehavior;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString AccessibleSummaryBehavior;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString AccessibleName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString AccessibleSummary;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bHasSlateWidget = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString SlateWidgetType;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bSlateVisible = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bSlateEnabled = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FAccessibleScreenBounds ScreenBounds;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bHasSlot = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FAccessibleWidgetSlotRecord Slot;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FAccessibleNavigationRecord> NavigationRules;
};

/** High-level report for a runtime Viewport / UserWidget inspection */
USTRUCT(BlueprintType)
struct FRuntimeWidgetReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	EInspectionAvailability Availability = EInspectionAvailability::Unavailable;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString WorldName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RootWidgetName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RootWidgetClass;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RootObjectPath;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 TotalWidgets = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 ActiveWidgetCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 HiddenOrCollapsedCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString FocusedWidgetName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bHasFocus = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bTruncated = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> Candidates;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 RootWidgetIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FRuntimeWidgetRecord> Widgets;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString NvdaText;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString JsonData;
};

/** Classification of a widget node during serialized-to-runtime reconciliation */
UENUM(BlueprintType)
enum class EWidgetComparisonClassification : uint8
{
	Matched,
	Ambiguous,
	SerializedOnly,
	RuntimeOnly,
	Unavailable,
	Unsupported
};

/** Detailed comparison record for a single reconciled widget entry */
USTRUCT(BlueprintType)
struct FWidgetComparisonRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	EWidgetComparisonClassification Classification = EWidgetComparisonClassification::Unavailable;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ClassificationString;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 SerializedNodeIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 RuntimeNodeIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString SerializedObjectPath;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RuntimeObjectPath;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString SerializedWidgetName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RuntimeWidgetName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString SerializedClass;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RuntimeWidgetClass;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RuntimeSlateType;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString SerializedParent;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RuntimeParent;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 SerializedChildOrder = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 RuntimeChildOrder = -1;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> Evidence;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> ConflictingEvidence;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> UnavailableEvidence;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> Limitations;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> Discrepancies;
};

/** High-level report for serialized-to-runtime reconciliation */
USTRUCT(BlueprintType)
struct FSerializedRuntimeComparisonReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	EInspectionAvailability Availability = EInspectionAvailability::Unavailable;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString AssetPath;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RuntimeWidgetIdentity;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString WorldIdentity;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 MatchedCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 AmbiguousCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 SerializedOnlyCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 RuntimeOnlyCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 UnavailableCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 UnsupportedCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bTruncated = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> Limitations;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> Candidates;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FWidgetComparisonRecord> ComparisonRecords;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> Discrepancies;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString NvdaText;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString JsonData;
};

/** Classification of an accessibility audit finding */
UENUM(BlueprintType)
enum class EAuditFindingClassification : uint8
{
	Error,
	Warning,
	Info,
	NotEvaluated
};

/** Basis of confidence for an audit finding (non-numeric) */
UENUM(BlueprintType)
enum class EAuditConfidenceBasis : uint8
{
	Direct,
	Derived,
	Ambiguous,
	Unavailable
};

/** Record of a single accessibility audit finding */
USTRUCT(BlueprintType)
struct FAuditFindingRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RuleId;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RuleName;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	EAuditFindingClassification Classification = EAuditFindingClassification::Info;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ClassificationString;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString Message;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString WidgetIdentity;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> SerializedEvidence;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> RuntimeEvidence;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> ComparisonEvidence;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> UnavailableEvidence;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	EAuditConfidenceBasis ConfidenceBasis = EAuditConfidenceBasis::Direct;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ConfidenceBasisString;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RemediationGuidance;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> Limitations;
};

/** Complete report emitted by AuditWidgetAccessibility */
USTRUCT(BlueprintType)
struct FAccessibilityAuditReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	EInspectionAvailability Availability = EInspectionAvailability::Unavailable;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString ErrorMessage;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString AssetPath;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RuntimeWidgetIdentity;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString WorldIdentity;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString EvidenceMode;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString RuleSetVersion = TEXT("1.0.0");

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 ErrorCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 WarningCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 InfoCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 NotEvaluatedCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	int32 PassedCount = 0;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bTruncated = false;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FAuditFindingRecord> Findings;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> PassedRuleIds;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> NotEvaluatedRules;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> Limitations;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString SourceSerializedReport;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString SourceRuntimeReport;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString SourceComparisonReport;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> Candidates;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString NvdaText;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString JsonData;
};

/** Capability metadata returned by GetInspectorCapabilities */
USTRUCT(BlueprintType)
struct FInspectorCapabilities
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString PluginName = TEXT("AccessibleUMGInspector");

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString PluginVersion = TEXT("0.1.0-rc.1");

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	FString SupportedEngineVersion = TEXT("5.8.2");

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> SupportedOperations = {
		TEXT("GetInspectorCapabilities"),
		TEXT("InspectSerializedWidgetBlueprint"),
		TEXT("InspectRuntimeUserWidget"),
		TEXT("CompareSerializedAndRuntime"),
		TEXT("AuditWidgetAccessibility")
	};

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	TArray<FString> SupportedFormats = {
		TEXT("NvdaText"),
		TEXT("Json"),
		TEXT("Both")
	};

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bReadOnlyGuaranteed = true;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bRequiresExplicitAssetPath = true;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bZeroSyntheticInput = true;

	UPROPERTY(BlueprintReadWrite, Category = "AccessibleUMGInspector")
	bool bZeroPackageMutation = true;
};
