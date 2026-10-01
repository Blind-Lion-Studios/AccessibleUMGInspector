// Copyright Blind Lion Studios. All Rights Reserved.

#include "SerializedWidgetInspector.h"
#include "NvdaTextFormatter.h"

#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Components/NamedSlotInterface.h"
#include "Blueprint/WidgetNavigation.h"
#include "JsonObjectConverter.h"
#include "UObject/Package.h"

FSerializedWidgetBlueprintReport FSerializedWidgetInspector::InspectAsset(const FString& AssetPath, const FString& Format)
{
	FSerializedWidgetBlueprintReport Report;
	Report.AssetPath = AssetPath;

	if (AssetPath.TrimStartAndEnd().IsEmpty())
	{
		Report.bSuccess = false;
		Report.ErrorMessage = TEXT("AssetPath is empty. Explicit asset path required.");
		Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatSerializedReport(Report);
		return Report;
	}

	// Resolve package first to check clean state
	UPackage* ExistingPackage = FindPackage(nullptr, *AssetPath);
	bool bWasDirtyBefore = ExistingPackage ? ExistingPackage->IsDirty() : false;
	Report.bPackageWasCleanBefore = !bWasDirtyBefore;

	// Load asset safely
	UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!Blueprint)
	{
		Report.bSuccess = false;
		Report.ErrorMessage = FString::Printf(TEXT("Failed to load UWidgetBlueprint at path '%s'."), *AssetPath);
		Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatSerializedReport(Report);
		return Report;
	}

	// Perform inspection on loaded blueprint
	Report = InspectBlueprint(Blueprint, Format);
	Report.AssetPath = AssetPath;

	// Verify post-inspection package dirty state
	UPackage* PostPackage = Blueprint->GetOutermost();
	bool bIsDirtyAfter = PostPackage ? PostPackage->IsDirty() : false;
	Report.bPackageRemainsCleanAfter = !bIsDirtyAfter;

	// Enforce fail-closed read-only invariant: if package was clean before, it must remain clean after
	if (!bWasDirtyBefore && bIsDirtyAfter)
	{
		Report.bSuccess = false;
		Report.ErrorMessage = TEXT("Read-only safety violation: asset package became dirty during inspection.");
		Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatSerializedReport(Report);
	}

	return Report;
}

FSerializedWidgetBlueprintReport FSerializedWidgetInspector::InspectBlueprint(UWidgetBlueprint* Blueprint, const FString& Format)
{
	FSerializedWidgetBlueprintReport Report;

	if (!Blueprint)
	{
		Report.bSuccess = false;
		Report.ErrorMessage = TEXT("Null UWidgetBlueprint supplied to inspector.");
		Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatSerializedReport(Report);
		return Report;
	}

	UPackage* Package = Blueprint->GetOutermost();
	bool bWasDirty = Package ? Package->IsDirty() : false;
	Report.bPackageWasCleanBefore = !bWasDirty;

	Report.AssetPath = Blueprint->GetPathName();
	Report.ParentClass = Blueprint->ParentClass ? Blueprint->ParentClass->GetName() : TEXT("None");

	UWidgetTree* Tree = Blueprint->WidgetTree;
	if (!Tree)
	{
		Report.bSuccess = false;
		Report.ErrorMessage = TEXT("WidgetBlueprint has no valid WidgetTree.");
		Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatSerializedReport(Report);
		return Report;
	}

	TMap<FString, int32> WidgetNameCounts;
	TMap<FString, int32> AccessibleNameCounts;

	if (Tree->RootWidget)
	{
		Report.RootWidgetIndex = InspectWidgetRecursively(
			Tree->RootWidget,
			TEXT("None"),
			-1,
			0,
			Blueprint,
			Report,
			WidgetNameCounts,
			AccessibleNameCounts);
	}

	Report.TotalWidgets = Report.Widgets.Num();

	// Identify duplicate widget names
	for (const auto& Pair : WidgetNameCounts)
	{
		if (Pair.Value > 1)
		{
			Report.DuplicateWidgetNames.Add(Pair.Key);
		}
	}

	// Identify duplicate accessible names across interactive controls
	for (const auto& Pair : AccessibleNameCounts)
	{
		if (Pair.Value > 1 && !Pair.Key.IsEmpty())
		{
			Report.DuplicateAccessibleNames.Add(Pair.Key);
		}
	}

	// Check package dirty state after inspection
	bool bIsDirtyAfter = Package ? Package->IsDirty() : false;
	Report.bPackageRemainsCleanAfter = !bIsDirtyAfter;

	if (!bWasDirty && bIsDirtyAfter)
	{
		Report.bSuccess = false;
		Report.ErrorMessage = TEXT("Read-only safety violation: asset package became dirty during inspection.");
	}
	else
	{
		Report.bSuccess = true;
	}

	// Format outputs based on Format parameter
	bool bWantsNvda = Format.Equals(TEXT("NvdaText"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);
	bool bWantsJson = Format.Equals(TEXT("Json"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);

	if (bWantsNvda)
	{
		Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatSerializedReport(Report);
	}

	if (bWantsJson)
	{
		FString JsonStr;
		FJsonObjectConverter::UStructToJsonObjectString(Report, JsonStr, 0, 0, 0, nullptr, false);
		Report.JsonData = JsonStr;
	}

	return Report;
}

int32 FSerializedWidgetInspector::InspectWidgetRecursively(
	UWidget* Widget,
	const FString& ParentName,
	int32 ParentNodeIndex,
	int32 ChildIndexInParent,
	const UWidgetBlueprint* Blueprint,
	FSerializedWidgetBlueprintReport& OutReport,
	TMap<FString, int32>& OutWidgetNameCounts,
	TMap<FString, int32>& OutAccessibleNameCounts)
{
	if (!Widget)
	{
		return -1;
	}

	int32 CurrentNodeIndex = OutReport.Widgets.Num();
	FSerializedWidgetRecord Record;

	Record.NodeIndex = CurrentNodeIndex;
	Record.ParentIndex = ParentNodeIndex;
	Record.WidgetName = Widget->GetName();
	Record.WidgetClass = Widget->GetClass()->GetName();
	Record.ObjectPath = Widget->GetPathName();
	Record.ParentName = ParentName;
	Record.ChildIndex = ChildIndexInParent;

	// Track widget name occurrences
	OutWidgetNameCounts.FindOrAdd(Record.WidgetName)++;

	// Reflected Visibility & Enabled
	Record.DefaultVisibility = GetVisibilityString((int32)Widget->GetVisibility());
	Record.bIsEnabled = Widget->GetIsEnabled();
	Record.bIsVariable = Widget->bIsVariable;

	// Reflected Focusability
	FBoolProperty* FocusableProp = CastField<FBoolProperty>(Widget->GetClass()->FindPropertyByName(TEXT("IsFocusable")));
	if (FocusableProp)
	{
		Record.bIsFocusable = FocusableProp->GetPropertyValue_InContainer(Widget);
	}
	else
	{
		Record.bIsFocusable = false;
	}

	// Reflected Accessibility Metadata
#if WITH_EDITORONLY_DATA
	Record.bOverrideAccessibleDefaults = (Widget->bOverrideAccessibleDefaults != 0);
	Record.AccessibleName = Widget->AccessibleText.ToString();
	Record.AccessibleSummary = Widget->AccessibleSummaryText.ToString();

	switch (Widget->AccessibleBehavior)
	{
	case ESlateAccessibleBehavior::NotAccessible: Record.AccessibleBehavior = TEXT("NotAccessible"); break;
	case ESlateAccessibleBehavior::Auto:          Record.AccessibleBehavior = TEXT("Auto"); break;
	case ESlateAccessibleBehavior::Summary:       Record.AccessibleBehavior = TEXT("Summary"); break;
	case ESlateAccessibleBehavior::Custom:        Record.AccessibleBehavior = TEXT("Custom"); break;
	case ESlateAccessibleBehavior::ToolTip:       Record.AccessibleBehavior = TEXT("ToolTip"); break;
	default:                                      Record.AccessibleBehavior = TEXT("Auto"); break;
	}

	switch (Widget->AccessibleSummaryBehavior)
	{
	case ESlateAccessibleBehavior::NotAccessible: Record.AccessibleSummaryBehavior = TEXT("NotAccessible"); break;
	case ESlateAccessibleBehavior::Auto:          Record.AccessibleSummaryBehavior = TEXT("Auto"); break;
	case ESlateAccessibleBehavior::Summary:       Record.AccessibleSummaryBehavior = TEXT("Summary"); break;
	case ESlateAccessibleBehavior::Custom:        Record.AccessibleSummaryBehavior = TEXT("Custom"); break;
	case ESlateAccessibleBehavior::ToolTip:       Record.AccessibleSummaryBehavior = TEXT("ToolTip"); break;
	default:                                      Record.AccessibleSummaryBehavior = TEXT("Auto"); break;
	}
#endif

	// If AccessibleName was not explicitly authored, attempt safe read from widget if available
	if (Record.AccessibleName.IsEmpty())
	{
		FText DynamicAccessibleText = Widget->GetAccessibleText();
		if (!DynamicAccessibleText.IsEmpty())
		{
			Record.AccessibleName = DynamicAccessibleText.ToString();
		}
	}

	// Track accessible names for duplicate detection on interactive widgets
	if (Record.bIsFocusable || Record.WidgetClass.Contains(TEXT("Button")) || Record.WidgetClass.Contains(TEXT("CheckBox")) || Record.WidgetClass.Contains(TEXT("Slider")))
	{
		if (!Record.AccessibleName.IsEmpty())
		{
			OutAccessibleNameCounts.FindOrAdd(Record.AccessibleName)++;
		}
	}

	// Extract slot properties
	ExtractSlotProperties(Widget, Record.Slot);
	Record.bHasSlot = !Record.Slot.SlotType.IsEmpty();

	// Extract navigation rules
	ExtractNavigationRules(Widget, Record.NavigationRules);

	// Extract bindings from Blueprint
	if (Blueprint)
	{
		for (const FDelegateEditorBinding& Binding : Blueprint->Bindings)
		{
			if (Binding.ObjectName == Record.WidgetName)
			{
				FAccessibleWidgetBindingRecord BindRec;
				BindRec.PropertyName = Binding.PropertyName.ToString();
				BindRec.FunctionName = Binding.FunctionName.ToString();
				BindRec.BindingKind = (Binding.Kind == EBindingKind::Function) ? TEXT("Function") : TEXT("Property");
				Record.Bindings.Add(BindRec);
			}
		}
	}

	// Named slot interface
	INamedSlotInterface* NamedSlotHost = Cast<INamedSlotInterface>(Widget);
	if (NamedSlotHost)
	{
		Record.bIsNamedSlotHost = true;
		TArray<FName> SlotNames;
		NamedSlotHost->GetSlotNames(SlotNames);
		for (const FName& SlotName : SlotNames)
		{
			Record.ExposedNamedSlots.Add(SlotName.ToString());
			OutReport.NamedSlotCount++;
		}
	}

	// Add record placeholder to report
	OutReport.Widgets.Add(Record);

	// Recursively process panel children
	UPanelWidget* Panel = Cast<UPanelWidget>(Widget);
	if (Panel)
	{
		int32 ChildCount = Panel->GetChildrenCount();
		for (int32 Idx = 0; Idx < ChildCount; ++Idx)
		{
			UWidget* ChildWidget = Panel->GetChildAt(Idx);
			if (ChildWidget)
			{
				int32 ChildNodeIndex = InspectWidgetRecursively(
					ChildWidget,
					Record.WidgetName,
					CurrentNodeIndex,
					Idx,
					Blueprint,
					OutReport,
					OutWidgetNameCounts,
					OutAccessibleNameCounts);

				if (ChildNodeIndex >= 0)
				{
					OutReport.Widgets[CurrentNodeIndex].ChildIndices.Add(ChildNodeIndex);
				}
			}
		}
	}

	return CurrentNodeIndex;
}

FString FSerializedWidgetInspector::GetVisibilityString(int32 VisibilityEnumVal)
{
	switch ((ESlateVisibility)VisibilityEnumVal)
	{
	case ESlateVisibility::Visible:               return TEXT("Visible");
	case ESlateVisibility::Collapsed:             return TEXT("Collapsed");
	case ESlateVisibility::Hidden:                return TEXT("Hidden");
	case ESlateVisibility::HitTestInvisible:      return TEXT("HitTestInvisible");
	case ESlateVisibility::SelfHitTestInvisible:  return TEXT("SelfHitTestInvisible");
	default:                                      return TEXT("Unknown");
	}
}

void FSerializedWidgetInspector::ExtractNavigationRules(UWidget* Widget, TArray<FAccessibleNavigationRecord>& OutRules)
{
	if (!Widget || !Widget->Navigation)
	{
		return;
	}

	UWidgetNavigation* Nav = Widget->Navigation;

	auto ProcessDirection = [&](EUINavigation Dir, const TCHAR* DirName)
	{
		EUINavigationRule Rule = Nav->GetNavigationRule(Dir);
		if (Rule != EUINavigationRule::Escape && Rule != EUINavigationRule::Invalid)
		{
			FAccessibleNavigationRecord Rec;
			Rec.Direction = DirName;

			switch (Rule)
			{
			case EUINavigationRule::Stop:           Rec.Rule = TEXT("Stop"); break;
			case EUINavigationRule::Wrap:           Rec.Rule = TEXT("Wrap"); break;
			case EUINavigationRule::Explicit:       Rec.Rule = TEXT("Explicit"); break;
			case EUINavigationRule::Custom:         Rec.Rule = TEXT("Custom"); break;
			case EUINavigationRule::CustomBoundary: Rec.Rule = TEXT("CustomBoundary"); break;
			default:                                Rec.Rule = TEXT("Other"); break;
			}

			FWidgetNavigationData& NavData = Nav->GetNavigationData(Dir);
			Rec.TargetWidget = NavData.WidgetToFocus.ToString();
			OutRules.Add(Rec);
		}
	};

	ProcessDirection(EUINavigation::Left, TEXT("Left"));
	ProcessDirection(EUINavigation::Right, TEXT("Right"));
	ProcessDirection(EUINavigation::Up, TEXT("Up"));
	ProcessDirection(EUINavigation::Down, TEXT("Down"));
	ProcessDirection(EUINavigation::Next, TEXT("Next"));
	ProcessDirection(EUINavigation::Previous, TEXT("Previous"));
}

void FSerializedWidgetInspector::ExtractSlotProperties(UWidget* Widget, FAccessibleWidgetSlotRecord& OutSlot)
{
	if (!Widget || !Widget->Slot)
	{
		return;
	}

	UPanelSlot* Slot = Widget->Slot;
	OutSlot.SlotType = Slot->GetClass()->GetName();
	OutSlot.ParentWidgetName = Slot->Parent ? Slot->Parent->GetName() : TEXT("None");

	// Reflect common slot properties safely
	for (TFieldIterator<FProperty> PropIt(Slot->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Prop = *PropIt;
		if (!Prop) continue;

		FName PropName = Prop->GetFName();
		if (PropName == TEXT("Padding") || PropName == TEXT("HorizontalAlignment") || PropName == TEXT("VerticalAlignment") || PropName == TEXT("Size") || PropName == TEXT("Offsets") || PropName == TEXT("Anchors") || PropName == TEXT("bOrderPriority"))
		{
			FString ValueStr;
			const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Slot);
			Prop->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, nullptr, PPF_None);
			OutSlot.Properties.Add(PropName.ToString(), ValueStr);
		}
	}
}
