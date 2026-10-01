// Copyright Blind Lion Studios. All Rights Reserved.

#include "RuntimeWidgetInspector.h"
#include "NvdaTextFormatter.h"
#include "AccessibleUMGInspectorTypes.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Blueprint/WidgetNavigation.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/Geometry.h"
#include "Widgets/SWidget.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UObject/UObjectIterator.h"
#include "JsonObjectConverter.h"

FRuntimeWidgetReport FRuntimeWidgetInspector::InspectRuntimeUserWidget(
	const FString& WidgetReference,
	const FString& WorldContext,
	int32 MaxDepth,
	const FString& Format)
{
	FRuntimeWidgetReport Report;

	EInspectionAvailability WorldAvail = EInspectionAvailability::Unavailable;
	FString WorldError;
	TArray<FString> WorldCandidates;

	UWorld* TargetWorld = ResolveTargetWorld(WorldContext, WorldAvail, WorldError, WorldCandidates);
	if (!TargetWorld)
	{
		Report.bSuccess = false;
		Report.Availability = WorldAvail;
		Report.ErrorMessage = WorldError;
		Report.Candidates = WorldCandidates;

		bool bWantsNvda = Format.Equals(TEXT("NvdaText"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);
		bool bWantsJson = Format.Equals(TEXT("Json"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);

		if (bWantsNvda)
		{
			Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatRuntimeReport(Report);
		}
		if (bWantsJson)
		{
			FString JsonStr;
			FJsonObjectConverter::UStructToJsonObjectString(Report, JsonStr, 0, 0, 0, nullptr, false);
			Report.JsonData = JsonStr;
		}

		return Report;
	}

	EInspectionAvailability WidgetAvail = EInspectionAvailability::Unavailable;
	FString WidgetError;
	TArray<FString> WidgetCandidates;

	UUserWidget* TargetWidget = ResolveTargetWidget(TargetWorld, WidgetReference, WidgetAvail, WidgetError, WidgetCandidates);
	if (!TargetWidget)
	{
		Report.bSuccess = false;
		Report.Availability = WidgetAvail;
		Report.ErrorMessage = WidgetError;
		Report.Candidates = WidgetCandidates;
		Report.WorldName = TargetWorld->GetName();
		Report.RootWidgetName = WidgetReference;

		bool bWantsNvda = Format.Equals(TEXT("NvdaText"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);
		bool bWantsJson = Format.Equals(TEXT("Json"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);

		if (bWantsNvda)
		{
			Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatRuntimeReport(Report);
		}
		if (bWantsJson)
		{
			FString JsonStr;
			FJsonObjectConverter::UStructToJsonObjectString(Report, JsonStr, 0, 0, 0, nullptr, false);
			Report.JsonData = JsonStr;
		}

		return Report;
	}

	return InspectUserWidgetInstance(TargetWidget, MaxDepth, Format);
}

FRuntimeWidgetReport FRuntimeWidgetInspector::InspectUserWidgetInstance(
	UUserWidget* UserWidget,
	int32 MaxDepth,
	const FString& Format)
{
	FRuntimeWidgetReport Report;

	if (!IsValid(UserWidget))
	{
		Report.bSuccess = false;
		Report.Availability = EInspectionAvailability::Unavailable;
		Report.ErrorMessage = TEXT("UserWidget is invalid or null.");
		return Report;
	}

	Report.bSuccess = true;
	Report.Availability = EInspectionAvailability::Available;
	Report.RootWidgetName = UserWidget->GetName();
	Report.RootWidgetClass = UserWidget->GetClass()->GetName();
	Report.RootObjectPath = UserWidget->GetPathName();

	if (UWorld* World = UserWidget->GetWorld())
	{
		Report.WorldName = World->GetName();
	}

	int32 ClampedMaxDepth = FMath::Clamp(MaxDepth, 1, 100);

	// Passive Slate focus query
	TSharedPtr<SWidget> KeyboardFocusedSlateWidget;
	TSharedPtr<SWidget> UserFocusedSlateWidget;
	if (FSlateApplication::IsInitialized())
	{
		KeyboardFocusedSlateWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
		UserFocusedSlateWidget = FSlateApplication::Get().GetUserFocusedWidget(0);
	}

	TSet<UWidget*> VisitedWidgets;
	Report.RootWidgetIndex = InspectWidgetRecursively(
		UserWidget,
		TEXT("None"),
		-1,
		0,
		0,
		ClampedMaxDepth,
		VisitedWidgets,
		KeyboardFocusedSlateWidget,
		UserFocusedSlateWidget,
		Report);

	Report.TotalWidgets = Report.Widgets.Num();

	// Compute effective visibility and ancestor suppression across the hierarchy
	ComputeEffectiveVisibility(Report);

	// Calculate active vs hidden/collapsed and focus summary
	for (const FRuntimeWidgetRecord& Rec : Report.Widgets)
	{
		if (Rec.bEffectivelyVisible)
		{
			Report.ActiveWidgetCount++;
		}
		else
		{
			Report.HiddenOrCollapsedCount++;
		}

		if (Rec.bHasKeyboardFocus || Rec.bHasUserFocus)
		{
			Report.bHasFocus = true;
			if (Report.FocusedWidgetName.IsEmpty())
			{
				Report.FocusedWidgetName = Rec.WidgetName;
			}
		}
	}

	// Format outputs based on Format parameter
	bool bWantsNvda = Format.Equals(TEXT("NvdaText"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);
	bool bWantsJson = Format.Equals(TEXT("Json"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);

	if (bWantsNvda)
	{
		Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatRuntimeReport(Report);
	}

	if (bWantsJson)
	{
		FString JsonStr;
		FJsonObjectConverter::UStructToJsonObjectString(Report, JsonStr, 0, 0, 0, nullptr, false);
		Report.JsonData = JsonStr;
	}

	return Report;
}

UWorld* FRuntimeWidgetInspector::ResolveTargetWorld(
	const FString& WorldContext,
	EInspectionAvailability& OutAvailability,
	FString& OutError,
	TArray<FString>& OutCandidates)
{
	if (!GEngine)
	{
		OutAvailability = EInspectionAvailability::Unavailable;
		OutError = TEXT("GEngine is null.");
		return nullptr;
	}

	TArray<UWorld*> CandidateWorlds;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* World = Context.World();
		if (World && (World->IsGameWorld() || Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::GamePreview))
		{
			CandidateWorlds.AddUnique(World);
		}
	}

	// If WorldContext was specified
	if (!WorldContext.IsEmpty())
	{
		for (UWorld* World : CandidateWorlds)
		{
			if (World->GetName().Equals(WorldContext, ESearchCase::IgnoreCase) ||
				World->GetPathName().Equals(WorldContext, ESearchCase::IgnoreCase))
			{
				OutAvailability = EInspectionAvailability::Available;
				return World;
			}
		}

		// Also check any valid world across memory matching name or path (e.g. synthetic test fixture worlds)
		for (TObjectIterator<UWorld> It; It; ++It)
		{
			UWorld* World = *It;
			if (IsValid(World) && (World->GetName().Equals(WorldContext, ESearchCase::IgnoreCase) || World->GetPathName().Equals(WorldContext, ESearchCase::IgnoreCase)))
			{
				OutAvailability = EInspectionAvailability::Available;
				return World;
			}
		}

		OutAvailability = EInspectionAvailability::Unavailable;
		OutError = FString::Printf(TEXT("No active world found matching WorldContext '%s'."), *WorldContext);
		return nullptr;
	}

	// WorldContext is empty: must resolve from candidate worlds
	if (CandidateWorlds.Num() == 0)
	{
		OutAvailability = EInspectionAvailability::Unavailable;
		OutError = TEXT("No active PIE or Game world found.");
		return nullptr;
	}
	else if (CandidateWorlds.Num() == 1)
	{
		OutAvailability = EInspectionAvailability::Available;
		return CandidateWorlds[0];
	}
	else
	{
		OutAvailability = EInspectionAvailability::Ambiguous;
		OutError = TEXT("Multiple active PIE or Game worlds found. Please specify WorldContext.");
		for (UWorld* W : CandidateWorlds)
		{
			OutCandidates.Add(W->GetPathName());
		}
		return nullptr;
	}
}

UUserWidget* FRuntimeWidgetInspector::ResolveTargetWidget(
	UWorld* World,
	const FString& WidgetReference,
	EInspectionAvailability& OutAvailability,
	FString& OutError,
	TArray<FString>& OutCandidates)
{
	if (!World)
	{
		OutAvailability = EInspectionAvailability::Unavailable;
		OutError = TEXT("Target world is null.");
		return nullptr;
	}

	// Collect eligible UUserWidgets in this World
	TArray<UUserWidget*> AllWorldWidgets;
	TArray<UUserWidget*> TopLevelWidgets;

	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (IsValid(Widget) && !Widget->IsTemplate() && !Widget->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
		{
			if (Widget->GetWorld() == World)
			{
				AllWorldWidgets.Add(Widget);
				if (Widget->GetParent() == nullptr)
				{
					TopLevelWidgets.Add(Widget);
				}
			}
		}
	}

	// If WidgetReference is specified
	if (!WidgetReference.IsEmpty())
	{
		// 1. Try exact object path
		for (UUserWidget* W : AllWorldWidgets)
		{
			if (W->GetPathName().Equals(WidgetReference, ESearchCase::IgnoreCase))
			{
				OutAvailability = EInspectionAvailability::Available;
				return W;
			}
		}

		// 2. Try exact name match
		TArray<UUserWidget*> NameMatches;
		for (UUserWidget* W : AllWorldWidgets)
		{
			if (W->GetName().Equals(WidgetReference, ESearchCase::IgnoreCase))
			{
				NameMatches.Add(W);
			}
		}
		if (NameMatches.Num() == 1)
		{
			OutAvailability = EInspectionAvailability::Available;
			return NameMatches[0];
		}
		else if (NameMatches.Num() > 1)
		{
			OutAvailability = EInspectionAvailability::Ambiguous;
			OutError = FString::Printf(TEXT("Multiple widgets match name '%s'."), *WidgetReference);
			for (UUserWidget* W : NameMatches)
			{
				OutCandidates.Add(W->GetPathName());
			}
			return nullptr;
		}

		// 3. Try class name match
		TArray<UUserWidget*> ClassMatches;
		for (UUserWidget* W : AllWorldWidgets)
		{
			FString ClassName = W->GetClass()->GetName();
			if (ClassName.Equals(WidgetReference, ESearchCase::IgnoreCase) ||
				ClassName.StartsWith(WidgetReference, ESearchCase::IgnoreCase))
			{
				ClassMatches.Add(W);
			}
		}
		if (ClassMatches.Num() == 1)
		{
			OutAvailability = EInspectionAvailability::Available;
			return ClassMatches[0];
		}
		else if (ClassMatches.Num() > 1)
		{
			OutAvailability = EInspectionAvailability::Ambiguous;
			OutError = FString::Printf(TEXT("Multiple widgets match class '%s'."), *WidgetReference);
			for (UUserWidget* W : ClassMatches)
			{
				OutCandidates.Add(W->GetPathName());
			}
			return nullptr;
		}

		OutAvailability = EInspectionAvailability::Unavailable;
		OutError = FString::Printf(TEXT("No runtime UUserWidget matching reference '%s' found in world '%s'."), *WidgetReference, *World->GetName());
		return nullptr;
	}

	// WidgetReference is empty: Look for single eligible top-level widget
	if (TopLevelWidgets.Num() == 0)
	{
		OutAvailability = EInspectionAvailability::Unavailable;
		OutError = FString::Printf(TEXT("No top-level UUserWidget found in world '%s'."), *World->GetName());
		return nullptr;
	}
	else if (TopLevelWidgets.Num() == 1)
	{
		OutAvailability = EInspectionAvailability::Available;
		return TopLevelWidgets[0];
	}
	else
	{
		OutAvailability = EInspectionAvailability::Ambiguous;
		OutError = FString::Printf(TEXT("Multiple top-level UUserWidgets found in world '%s'. Please specify WidgetReference."), *World->GetName());
		for (UUserWidget* W : TopLevelWidgets)
		{
			OutCandidates.Add(W->GetPathName());
		}
		return nullptr;
	}
}

int32 FRuntimeWidgetInspector::InspectWidgetRecursively(
	UWidget* Widget,
	const FString& ParentName,
	int32 ParentNodeIndex,
	int32 ChildIndexInParent,
	int32 CurrentDepth,
	int32 MaxDepth,
	TSet<UWidget*>& VisitedWidgets,
	const TSharedPtr<SWidget>& KeyboardFocusedSlateWidget,
	const TSharedPtr<SWidget>& UserFocusedSlateWidget,
	FRuntimeWidgetReport& OutReport)
{
	if (!IsValid(Widget))
	{
		return -1;
	}

	if (VisitedWidgets.Contains(Widget))
	{
		return -1;
	}

	// Safety: enforce node limit
	const int32 MaxNodeCount = 500;
	if (OutReport.Widgets.Num() >= MaxNodeCount)
	{
		OutReport.bTruncated = true;
		return -1;
	}

	// Safety: enforce depth limit
	if (CurrentDepth > MaxDepth)
	{
		OutReport.bTruncated = true;
		return -1;
	}

	VisitedWidgets.Add(Widget);

	int32 CurrentNodeIndex = OutReport.Widgets.Num();
	FRuntimeWidgetRecord Record;

	Record.NodeIndex = CurrentNodeIndex;
	Record.ParentIndex = ParentNodeIndex;
	Record.WidgetName = Widget->GetName();
	Record.WidgetClass = Widget->GetClass()->GetName();
	Record.ObjectPath = Widget->GetPathName();
	Record.ParentName = ParentName;
	Record.ChildIndex = ChildIndexInParent;
	Record.Depth = CurrentDepth;

	// Runtime visibility and interactive state
	Record.LocalVisibility = GetVisibilityString((int32)Widget->GetVisibility());
	Record.RuntimeVisibility = Record.LocalVisibility;
	Record.bIsEnabled = Widget->GetIsEnabled();
	Record.bIsHovered = Widget->IsHovered();

	// Reflected focusability
	FBoolProperty* FocusableProp = CastField<FBoolProperty>(Widget->GetClass()->FindPropertyByName(TEXT("IsFocusable")));
	if (FocusableProp)
	{
		Record.bIsFocusable = FocusableProp->GetPropertyValue_InContainer(Widget);
	}
	else
	{
		Record.bIsFocusable = false;
	}

	// Reflected accessibility metadata
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

	if (Record.AccessibleName.IsEmpty())
	{
		FText DynamicText = Widget->GetAccessibleText();
		if (!DynamicText.IsEmpty())
		{
			Record.AccessibleName = DynamicText.ToString();
		}
	}

	// Passive Slate inspection via GetCachedWidget() ONLY. Never call TakeWidget().
	TSharedPtr<SWidget> CachedSlate = Widget->GetCachedWidget();
	if (CachedSlate.IsValid())
	{
		Record.bHasSlateWidget = true;
		Record.SlateWidgetType = CachedSlate->GetTypeAsString();
		Record.bSlateVisible = CachedSlate->GetVisibility().IsVisible();
		Record.bSlateEnabled = CachedSlate->IsEnabled();
		if (CachedSlate->IsHovered())
		{
			Record.bIsHovered = true;
		}

		// Passive focus inspection
		if (KeyboardFocusedSlateWidget.IsValid() && (KeyboardFocusedSlateWidget == CachedSlate || CachedSlate->HasKeyboardFocus()))
		{
			Record.bHasKeyboardFocus = true;
		}
		if (UserFocusedSlateWidget.IsValid() && (UserFocusedSlateWidget == CachedSlate || CachedSlate->HasUserFocus(0).IsSet()))
		{
			Record.bHasUserFocus = true;
		}

		// Passive geometry inspection via GetTickSpaceGeometry()
		const FGeometry& Geometry = CachedSlate->GetTickSpaceGeometry();
		const FVector2D AbsolutePos = Geometry.GetAbsolutePosition();
		const FVector2D AbsoluteSize = Geometry.GetAbsoluteSize();

		Record.ScreenBounds.X = (float)AbsolutePos.X;
		Record.ScreenBounds.Y = (float)AbsolutePos.Y;
		Record.ScreenBounds.Width = (float)AbsoluteSize.X;
		Record.ScreenBounds.Height = (float)AbsoluteSize.Y;
		Record.ScreenBounds.bIsValid = (AbsoluteSize.X > 0.0f || AbsoluteSize.Y > 0.0f);
	}
	else
	{
		Record.bHasSlateWidget = false;
		Record.bSlateVisible = false;
		Record.bSlateEnabled = false;
		Record.ScreenBounds.bIsValid = false;
	}

	// Extract slot properties
	ExtractSlotProperties(Widget, Record.Slot);
	Record.bHasSlot = !Record.Slot.SlotType.IsEmpty();

	// Extract navigation rules
	ExtractNavigationRules(Widget, Record.NavigationRules);

	// Add record placeholder to report
	OutReport.Widgets.Add(Record);

	// If node limit reached, do not process children
	if (OutReport.Widgets.Num() >= MaxNodeCount)
	{
		OutReport.bTruncated = true;
		return CurrentNodeIndex;
	}

	// Traverse children:
	// Case 1: UUserWidget has a root widget in its WidgetTree
	if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
	{
		if (UWidget* RootChild = UserWidget->GetRootWidget())
		{
			int32 ChildNodeIndex = InspectWidgetRecursively(
				RootChild,
				Record.WidgetName,
				CurrentNodeIndex,
				0,
				CurrentDepth + 1,
				MaxDepth,
				VisitedWidgets,
				KeyboardFocusedSlateWidget,
				UserFocusedSlateWidget,
				OutReport);

			if (ChildNodeIndex >= 0)
			{
				OutReport.Widgets[CurrentNodeIndex].ChildIndices.Add(ChildNodeIndex);
			}
		}
	}
	// Case 2: UPanelWidget has indexed child widgets
	else if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
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
					CurrentDepth + 1,
					MaxDepth,
					VisitedWidgets,
					KeyboardFocusedSlateWidget,
					UserFocusedSlateWidget,
					OutReport);

				if (ChildNodeIndex >= 0)
				{
					OutReport.Widgets[CurrentNodeIndex].ChildIndices.Add(ChildNodeIndex);
				}
			}
		}
	}

	return CurrentNodeIndex;
}

void FRuntimeWidgetInspector::ExtractNavigationRules(UWidget* Widget, TArray<FAccessibleNavigationRecord>& OutRules)
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

void FRuntimeWidgetInspector::ExtractSlotProperties(UWidget* Widget, FAccessibleWidgetSlotRecord& OutSlot)
{
	if (!Widget || !Widget->Slot)
	{
		return;
	}

	UPanelSlot* Slot = Widget->Slot;
	OutSlot.SlotType = Slot->GetClass()->GetName();
	OutSlot.ParentWidgetName = Slot->Parent ? Slot->Parent->GetName() : TEXT("None");

	for (TFieldIterator<FProperty> PropIt(Slot->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Prop = *PropIt;
		if (!Prop) continue;

		FName PropName = Prop->GetFName();
		if (PropName == TEXT("Padding") || PropName == TEXT("HorizontalAlignment") || PropName == TEXT("VerticalAlignment") || PropName == TEXT("Size") || PropName == TEXT("Offsets") || PropName == TEXT("Anchors") || PropName == TEXT("bOrderPriority"))
		{
			FString ValueStr;
			Prop->ExportText_InContainer(0, ValueStr, Slot, Slot, Slot, PPF_None);
			OutSlot.Properties.Add(PropName.ToString(), ValueStr);
		}
	}
}

FString FRuntimeWidgetInspector::GetVisibilityString(int32 VisibilityEnumVal)
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

void FRuntimeWidgetInspector::ComputeEffectiveVisibility(FRuntimeWidgetReport& Report)
{
	for (int32 NodeIdx = 0; NodeIdx < Report.Widgets.Num(); ++NodeIdx)
	{
		FRuntimeWidgetRecord& Rec = Report.Widgets[NodeIdx];
		if (Rec.LocalVisibility.IsEmpty())
		{
			Rec.LocalVisibility = !Rec.RuntimeVisibility.IsEmpty() ? Rec.RuntimeVisibility : TEXT("Visible");
		}
		Rec.RuntimeVisibility = Rec.LocalVisibility;

		// Walk the complete parent chain
		TArray<int32> AncestorIndices;
		TSet<int32> VisitedAncestors;
		int32 CurrParent = Rec.ParentIndex;
		while (CurrParent >= 0 && CurrParent < Report.Widgets.Num() && !VisitedAncestors.Contains(CurrParent))
		{
			VisitedAncestors.Add(CurrParent);
			AncestorIndices.Add(CurrParent);
			CurrParent = Report.Widgets[CurrParent].ParentIndex;
		}

		int32 NearestCollapsedAncestor = -1;
		int32 NearestHiddenAncestor = -1;
		int32 NearestDisabledAncestor = -1;
		int32 NearestHitTestInvisibleAncestor = -1;

		for (int32 AncIdx : AncestorIndices)
		{
			const FRuntimeWidgetRecord& Anc = Report.Widgets[AncIdx];
			const FString& AncVis = Anc.LocalVisibility;
			if (AncVis.Equals(TEXT("Collapsed"), ESearchCase::IgnoreCase))
			{
				if (NearestCollapsedAncestor == -1)
				{
					NearestCollapsedAncestor = AncIdx;
				}
			}
			else if (AncVis.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase))
			{
				if (NearestHiddenAncestor == -1)
				{
					NearestHiddenAncestor = AncIdx;
				}
			}
			else if (AncVis.Equals(TEXT("HitTestInvisible"), ESearchCase::IgnoreCase))
			{
				if (NearestHitTestInvisibleAncestor == -1)
				{
					NearestHitTestInvisibleAncestor = AncIdx;
				}
			}

			if (!Anc.bIsEnabled)
			{
				if (NearestDisabledAncestor == -1)
				{
					NearestDisabledAncestor = AncIdx;
				}
			}
		}

		bool bSelfCollapsed = Rec.LocalVisibility.Equals(TEXT("Collapsed"), ESearchCase::IgnoreCase);
		bool bSelfHidden = Rec.LocalVisibility.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase);
		bool bSelfHTI = Rec.LocalVisibility.Equals(TEXT("HitTestInvisible"), ESearchCase::IgnoreCase);
		bool bSelfSHTI = Rec.LocalVisibility.Equals(TEXT("SelfHitTestInvisible"), ESearchCase::IgnoreCase);

		Rec.VisibilityEvidence.Empty();
		Rec.EffectiveStateLimitations.Empty();

		// Rule 2: If the widget or any ancestor is Collapsed
		if (bSelfCollapsed || NearestCollapsedAncestor != -1)
		{
			Rec.EffectiveVisibility = TEXT("Collapsed");
			Rec.bEffectivelyVisible = false;
			Rec.bEffectivelyInteractive = false;
			Rec.bEffectivelyFocusable = false;
			Rec.bEffectiveGeometryAvailable = false;

			if (NearestCollapsedAncestor != -1)
			{
				Rec.BlockingAncestorIndex = NearestCollapsedAncestor;
				Rec.BlockingAncestorName = Report.Widgets[NearestCollapsedAncestor].WidgetName;
				Rec.VisibilityEvidence.Add(FString::Printf(TEXT("Suppressed by collapsed ancestor '%s' (index %d)"),
					*Rec.BlockingAncestorName, Rec.BlockingAncestorIndex));
			}
			else
			{
				Rec.BlockingAncestorIndex = -1;
				Rec.BlockingAncestorName = TEXT("");
				Rec.VisibilityEvidence.Add(TEXT("Locally Collapsed"));
			}
		}
		// Rule 3: Else if the widget or any ancestor is Hidden
		else if (bSelfHidden || NearestHiddenAncestor != -1)
		{
			Rec.EffectiveVisibility = TEXT("Hidden");
			Rec.bEffectivelyVisible = false;
			Rec.bEffectivelyInteractive = false;
			Rec.bEffectivelyFocusable = false;
			Rec.bEffectiveGeometryAvailable = false;

			if (NearestHiddenAncestor != -1)
			{
				Rec.BlockingAncestorIndex = NearestHiddenAncestor;
				Rec.BlockingAncestorName = Report.Widgets[NearestHiddenAncestor].WidgetName;
				Rec.VisibilityEvidence.Add(FString::Printf(TEXT("Suppressed by hidden ancestor '%s' (index %d)"),
					*Rec.BlockingAncestorName, Rec.BlockingAncestorIndex));
			}
			else
			{
				Rec.BlockingAncestorIndex = -1;
				Rec.BlockingAncestorName = TEXT("");
				Rec.VisibilityEvidence.Add(TEXT("Locally Hidden"));
			}
		}
		// Rule 4: HitTestInvisible
		else if (NearestHitTestInvisibleAncestor != -1)
		{
			Rec.EffectiveVisibility = TEXT("HitTestInvisible");
			Rec.bEffectivelyVisible = true;
			Rec.BlockingAncestorIndex = -1;
			Rec.BlockingAncestorName = TEXT("");
			Rec.VisibilityEvidence.Add(FString::Printf(TEXT("Descendant of HitTestInvisible ancestor '%s' (index %d)"),
				*Report.Widgets[NearestHitTestInvisibleAncestor].WidgetName, NearestHitTestInvisibleAncestor));
		}
		else if (bSelfHTI)
		{
			Rec.EffectiveVisibility = TEXT("HitTestInvisible");
			Rec.bEffectivelyVisible = true;
			Rec.BlockingAncestorIndex = -1;
			Rec.BlockingAncestorName = TEXT("");
			Rec.VisibilityEvidence.Add(TEXT("Locally HitTestInvisible"));
		}
		// Rule 5: SelfHitTestInvisible
		else if (bSelfSHTI)
		{
			Rec.EffectiveVisibility = TEXT("SelfHitTestInvisible");
			Rec.bEffectivelyVisible = true;
			Rec.BlockingAncestorIndex = -1;
			Rec.BlockingAncestorName = TEXT("");
			Rec.VisibilityEvidence.Add(TEXT("Locally SelfHitTestInvisible"));
		}
		else
		{
			Rec.EffectiveVisibility = TEXT("Visible");
			Rec.bEffectivelyVisible = true;
			Rec.BlockingAncestorIndex = -1;
			Rec.BlockingAncestorName = TEXT("");
			Rec.VisibilityEvidence.Add(TEXT("Locally Visible"));
		}

		// Rule 6: Parent enabled state must propagate
		bool bEffectivelyEnabled = Rec.bIsEnabled && (NearestDisabledAncestor == -1);
		if (!bEffectivelyEnabled && NearestDisabledAncestor != -1)
		{
			Rec.VisibilityEvidence.Add(FString::Printf(TEXT("Effectively disabled by ancestor '%s' (index %d)"),
				*Report.Widgets[NearestDisabledAncestor].WidgetName, NearestDisabledAncestor));
		}

		// Rule 7: A local focusable property does not establish effective focusability through a Hidden, Collapsed, disabled, invalid, or unavailable ancestor
		if (Rec.bEffectivelyVisible && bEffectivelyEnabled && Rec.bIsFocusable)
		{
			Rec.bEffectivelyFocusable = true;
		}
		else
		{
			Rec.bEffectivelyFocusable = false;
		}

		// Rule 8: Cached geometry under an effectively hidden tree must not establish visibility, interaction, overlap, or pointer routing
		if (!Rec.bEffectivelyVisible)
		{
			Rec.bEffectiveGeometryAvailable = false;
		}
		else
		{
			Rec.bEffectiveGeometryAvailable = Rec.ScreenBounds.bIsValid &&
				(Rec.ScreenBounds.Width > 0.0f || Rec.ScreenBounds.Height > 0.0f);
		}

		// Effective interaction
		bool bIsPointerHitTestable = Rec.bEffectivelyVisible &&
			!Rec.EffectiveVisibility.Equals(TEXT("HitTestInvisible"), ESearchCase::IgnoreCase) &&
			!Rec.EffectiveVisibility.Equals(TEXT("SelfHitTestInvisible"), ESearchCase::IgnoreCase);

		if (bEffectivelyEnabled && bIsPointerHitTestable &&
			(Rec.bEffectivelyFocusable || Rec.bIsFocusable ||
			 Rec.WidgetClass.Contains(TEXT("Button")) || Rec.WidgetClass.Contains(TEXT("CheckBox")) ||
			 Rec.WidgetClass.Contains(TEXT("Slider")) || Rec.WidgetClass.Contains(TEXT("Editable")) ||
			 Rec.WidgetClass.Contains(TEXT("ComboBox")) || Rec.WidgetClass.Contains(TEXT("SpinBox"))))
		{
			Rec.bEffectivelyInteractive = true;
		}
		else
		{
			Rec.bEffectivelyInteractive = false;
		}

		// Rule 9: Do not claim OS accessibility exposure from UMG or Slate visibility alone
		Rec.EffectiveStateLimitations.Add(TEXT("Effective visibility reflects UMG/Slate hierarchy state; does not prove OS accessibility tree (UIA/IA2) exposure."));
		Rec.EffectiveStateLimitations.Add(TEXT("Pointer interaction is inferred from Slate visibility flags; does not verify hit-test grid or clipping bounds."));
	}
}
