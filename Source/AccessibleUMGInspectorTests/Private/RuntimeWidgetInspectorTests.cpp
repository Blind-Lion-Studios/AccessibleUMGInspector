// Copyright Blind Lion Studios. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "RuntimeWidgetInspector.h"
#include "NvdaTextFormatter.h"
#include "AccessibleUMGInspectorToolset.h"
#include "AccessibleUMGInspectorTypes.h"
#include "SerializedWidgetInspector.h"

#include "WidgetBlueprint.h"
#include "TestConcreteUserWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetNavigation.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#define TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace RuntimeWidgetInspectorTestHelpers
{
	static UWorld* CreateSyntheticWorld(const FString& BaseName = TEXT("RuntimeTestWorld"))
	{
		FName WorldName = FName(*FString::Printf(TEXT("%s_%s"), *BaseName, *FGuid::NewGuid().ToString()));
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName);
		return World;
	}

	static void DestroySyntheticWorld(UWorld* World)
	{
		if (World)
		{
			World->DestroyWorld(false);
		}
	}

	static UUserWidget* CreateSyntheticUserWidget(UWorld* World, const FString& WidgetName = TEXT("WBP_SyntheticShell"))
	{
		UTestConcreteUserWidget* UserWidget = NewObject<UTestConcreteUserWidget>(World, FName(*WidgetName));
		UserWidget->WidgetTree = NewObject<UWidgetTree>(UserWidget, FName(TEXT("WidgetTree")));

		// Root Canvas
		UCanvasPanel* RootCanvas = UserWidget->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UserWidget->WidgetTree->RootWidget = RootCanvas;

		// Vertical Box
		UVerticalBox* VertBox = UserWidget->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVerticalBox"));
		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(VertBox);
		CanvasSlot->SetOffsets(FMargin(10.f, 20.f, 400.f, 600.f));

		// Button 1: Primary (Visible, Focusable, Nav rules)
		UButton* BtnPrimary = UserWidget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Primary"));
		BtnPrimary->SetIsEnabled(true);
		BtnPrimary->SetVisibility(ESlateVisibility::Visible);
		if (FBoolProperty* FocusProp = CastField<FBoolProperty>(BtnPrimary->GetClass()->FindPropertyByName(TEXT("IsFocusable"))))
		{
			FocusProp->SetPropertyValue_InContainer(BtnPrimary, true);
		}
#if WITH_EDITORONLY_DATA
		BtnPrimary->bOverrideAccessibleDefaults = true;
		BtnPrimary->AccessibleText = FText::FromString(TEXT("Primary Action"));
		BtnPrimary->AccessibleSummaryText = FText::FromString(TEXT("Triggers main operation"));
		BtnPrimary->AccessibleBehavior = ESlateAccessibleBehavior::Custom;
#endif
		BtnPrimary->Navigation = NewObject<UWidgetNavigation>(BtnPrimary);
		BtnPrimary->Navigation->Down.Rule = EUINavigationRule::Explicit;
		BtnPrimary->Navigation->Down.WidgetToFocus = FName(TEXT("Button_Secondary"));

		VertBox->AddChildToVerticalBox(BtnPrimary);

		// Button 2: Secondary (Visible)
		UButton* BtnSecondary = UserWidget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Secondary"));
		BtnSecondary->SetIsEnabled(true);
		BtnSecondary->SetVisibility(ESlateVisibility::Visible);
		VertBox->AddChildToVerticalBox(BtnSecondary);

		// Button 3: Collapsed
		UButton* BtnCollapsed = UserWidget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Collapsed"));
		BtnCollapsed->SetVisibility(ESlateVisibility::Collapsed);
		VertBox->AddChildToVerticalBox(BtnCollapsed);

		// Button 4: Hidden
		UButton* BtnHidden = UserWidget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Hidden"));
		BtnHidden->SetVisibility(ESlateVisibility::Hidden);
		VertBox->AddChildToVerticalBox(BtnHidden);

		// TextBlock: HitTestInvisible
		UTextBlock* TextHitTestInv = UserWidget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_HitTestInv"));
		TextHitTestInv->SetVisibility(ESlateVisibility::HitTestInvisible);
		VertBox->AddChildToVerticalBox(TextHitTestInv);

		// TextBlock: SelfHitTestInvisible
		UTextBlock* TextSelfHitTestInv = UserWidget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_SelfHitTestInv"));
		TextSelfHitTestInv->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		VertBox->AddChildToVerticalBox(TextSelfHitTestInv);

		return UserWidget;
	}

	static const FRuntimeWidgetRecord* FindWidgetByName(const FRuntimeWidgetReport& Report, const FString& Name)
	{
		for (const FRuntimeWidgetRecord& W : Report.Widgets)
		{
			if (W.WidgetName == Name)
			{
				return &W;
			}
		}
		return nullptr;
	}
}

// ── Test 1: Exact Resolution By Name ───────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeExactResolutionByNameTest, "AccessibleUMGInspector.Runtime.ExactResolutionByName", TEST_FLAGS)
bool FRuntimeExactResolutionByNameTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_TargetShell"));
	TestNotNull(TEXT("Widget created"), Widget);

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectRuntimeUserWidget(TEXT("WBP_TargetShell"), World->GetName(), 32, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);
	TestEqual(TEXT("Report availability Available"), (uint8)Report.Availability, (uint8)EInspectionAvailability::Available);
	TestEqual(TEXT("RootWidgetName matches"), Report.RootWidgetName, TEXT("WBP_TargetShell"));

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 2: Exact Resolution By Path ───────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeExactResolutionByPathTest, "AccessibleUMGInspector.Runtime.ExactResolutionByPath", TEST_FLAGS)
bool FRuntimeExactResolutionByPathTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_PathTarget"));
	TestNotNull(TEXT("Widget created"), Widget);

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectRuntimeUserWidget(Widget->GetPathName(), World->GetName(), 32, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);
	TestEqual(TEXT("RootObjectPath matches"), Report.RootObjectPath, Widget->GetPathName());

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 3: No Eligible Widget ─────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeNoEligibleWidgetTest, "AccessibleUMGInspector.Runtime.NoEligibleWidget", TEST_FLAGS)
bool FRuntimeNoEligibleWidgetTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectRuntimeUserWidget(TEXT(""), World->GetName(), 32, TEXT("Both"));
	TestFalse(TEXT("Report should fail when no widgets exist"), Report.bSuccess);
	TestEqual(TEXT("Report availability should be Unavailable"), (uint8)Report.Availability, (uint8)EInspectionAvailability::Unavailable);
	TestTrue(TEXT("ErrorMessage describes missing widget"), Report.ErrorMessage.Contains(TEXT("No top-level")));

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 4: Ambiguous Widgets ──────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeAmbiguousWidgetsTest, "AccessibleUMGInspector.Runtime.AmbiguousWidgets", TEST_FLAGS)
bool FRuntimeAmbiguousWidgetsTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* WidgetA = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_WidgetA"));
	UUserWidget* WidgetB = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_WidgetB"));
	TestNotNull(TEXT("WidgetA created"), WidgetA);
	TestNotNull(TEXT("WidgetB created"), WidgetB);

	// Empty widget reference with multiple top-level widgets
	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectRuntimeUserWidget(TEXT(""), World->GetName(), 32, TEXT("Both"));
	TestFalse(TEXT("Report should not succeed for ambiguous widgets"), Report.bSuccess);
	TestEqual(TEXT("Report availability should be Ambiguous"), (uint8)Report.Availability, (uint8)EInspectionAvailability::Ambiguous);
	TestTrue(TEXT("Candidates list should contain both widgets"), Report.Candidates.Num() >= 2);

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 5: Ambiguous Worlds ───────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeAmbiguousWorldsTest, "AccessibleUMGInspector.Runtime.AmbiguousWorlds", TEST_FLAGS)
bool FRuntimeAmbiguousWorldsTest::RunTest(const FString& Parameters)
{
	// ResolveTargetWorld directly with empty context
	EInspectionAvailability Avail = EInspectionAvailability::Unavailable;
	FString ErrorMsg;
	TArray<FString> Candidates;

	// In editor tests, multiple worlds or 0 candidate worlds may exist
	UWorld* World = FRuntimeWidgetInspector::ResolveTargetWorld(TEXT(""), Avail, ErrorMsg, Candidates);
	if (!World)
	{
		TestTrue(TEXT("Availability must be Unavailable or Ambiguous"), Avail == EInspectionAvailability::Unavailable || Avail == EInspectionAvailability::Ambiguous);
		if (Avail == EInspectionAvailability::Ambiguous)
		{
			TestTrue(TEXT("Candidates must list worlds"), Candidates.Num() > 1);
		}
	}

	return true;
}

// ── Test 6: Deterministic Child Traversal Order ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeDeterministicOrderTest, "AccessibleUMGInspector.Runtime.DeterministicOrder", TEST_FLAGS)
bool FRuntimeDeterministicOrderTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_OrderTest"));
	TestNotNull(TEXT("Widget created"), Widget);

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);

	// Find MainVerticalBox
	const FRuntimeWidgetRecord* VertBoxRec = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("MainVerticalBox"));
	TestNotNull(TEXT("MainVerticalBox found"), VertBoxRec);
	TestEqual(TEXT("VertBox has 6 children"), VertBoxRec->ChildIndices.Num(), 6);

	// Children must appear in exact order of addition
	TArray<FString> ExpectedNames = {
		TEXT("Button_Primary"),
		TEXT("Button_Secondary"),
		TEXT("Button_Collapsed"),
		TEXT("Button_Hidden"),
		TEXT("Text_HitTestInv"),
		TEXT("Text_SelfHitTestInv")
	};

	for (int32 Idx = 0; Idx < ExpectedNames.Num(); ++Idx)
	{
		int32 ChildNodeIndex = VertBoxRec->ChildIndices[Idx];
		TestTrue(TEXT("Child index valid"), Report.Widgets.IsValidIndex(ChildNodeIndex));
		TestEqual(TEXT("Child name matches order"), Report.Widgets[ChildNodeIndex].WidgetName, ExpectedNames[Idx]);
		TestEqual(TEXT("ChildIndex matches container position"), Report.Widgets[ChildNodeIndex].ChildIndex, Idx);
	}

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 7: Visibility States ──────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeVisibilityStatesTest, "AccessibleUMGInspector.Runtime.VisibilityStates", TEST_FLAGS)
bool FRuntimeVisibilityStatesTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_VisibilityTest"));
	TestNotNull(TEXT("Widget created"), Widget);

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);

	const FRuntimeWidgetRecord* Primary = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Primary"));
	TestNotNull(TEXT("Button_Primary found"), Primary);
	TestEqual(TEXT("Primary Visible"), Primary->RuntimeVisibility, TEXT("Visible"));

	const FRuntimeWidgetRecord* Collapsed = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Collapsed"));
	TestNotNull(TEXT("Button_Collapsed found"), Collapsed);
	TestEqual(TEXT("Collapsed"), Collapsed->RuntimeVisibility, TEXT("Collapsed"));

	const FRuntimeWidgetRecord* Hidden = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Hidden"));
	TestNotNull(TEXT("Button_Hidden found"), Hidden);
	TestEqual(TEXT("Hidden"), Hidden->RuntimeVisibility, TEXT("Hidden"));

	const FRuntimeWidgetRecord* HTI = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("Text_HitTestInv"));
	TestNotNull(TEXT("Text_HitTestInv found"), HTI);
	TestEqual(TEXT("HitTestInvisible"), HTI->RuntimeVisibility, TEXT("HitTestInvisible"));

	const FRuntimeWidgetRecord* SHTI = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("Text_SelfHitTestInv"));
	TestNotNull(TEXT("Text_SelfHitTestInv found"), SHTI);
	TestEqual(TEXT("SelfHitTestInvisible"), SHTI->RuntimeVisibility, TEXT("SelfHitTestInvisible"));

	// Summary counts
	TestEqual(TEXT("HiddenOrCollapsedCount count"), Report.HiddenOrCollapsedCount, 2);
	TestTrue(TEXT("ActiveWidgetCount > 0"), Report.ActiveWidgetCount >= 5);

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 8: Passive Focus Extraction ───────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimePassiveFocusExtractionTest, "AccessibleUMGInspector.Runtime.PassiveFocusExtraction", TEST_FLAGS)
bool FRuntimePassiveFocusExtractionTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_FocusTest"));
	TestNotNull(TEXT("Widget created"), Widget);

	// Inspect without setting focus
	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);

	const FRuntimeWidgetRecord* Primary = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Primary"));
	TestNotNull(TEXT("Primary found"), Primary);
	TestTrue(TEXT("Primary is focusable"), Primary->bIsFocusable);
	// No focus set, so bHasKeyboardFocus should be false
	TestFalse(TEXT("Primary does not have focus"), Primary->bHasKeyboardFocus);

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 9: Passive Geometry Extraction ────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimePassiveGeometryExtractionTest, "AccessibleUMGInspector.Runtime.PassiveGeometryExtraction", TEST_FLAGS)
bool FRuntimePassiveGeometryExtractionTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_GeometryTest"));
	TestNotNull(TEXT("Widget created"), Widget);

	// Construct Slate widget explicitly for the UserWidget
	TSharedRef<SWidget> SlateRef = Widget->TakeWidget();
	TestTrue(TEXT("TakeWidget produced Slate widget"), SlateRef->GetVisibility().IsVisible());

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);

	const FRuntimeWidgetRecord* RootRec = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("WBP_GeometryTest"));
	TestNotNull(TEXT("Root found"), RootRec);
	TestTrue(TEXT("Root has Slate widget"), RootRec->bHasSlateWidget);
	TestTrue(TEXT("Slate widget type populated"), !RootRec->SlateWidgetType.IsEmpty());

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 10: Destroyed/Invalid Widget Handling ──────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeDestroyedWidgetHandlingTest, "AccessibleUMGInspector.Runtime.DestroyedWidgetHandling", TEST_FLAGS)
bool FRuntimeDestroyedWidgetHandlingTest::RunTest(const FString& Parameters)
{
	// Null widget
	FRuntimeWidgetReport ReportNull = FRuntimeWidgetInspector::InspectUserWidgetInstance(nullptr, 32, TEXT("Both"));
	TestFalse(TEXT("Null widget report should fail"), ReportNull.bSuccess);
	TestEqual(TEXT("Availability should be Unavailable"), (uint8)ReportNull.Availability, (uint8)EInspectionAvailability::Unavailable);

	// Widget marked for destruction
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_DestroyTest"));
	Widget->MarkAsGarbage();

	FRuntimeWidgetReport ReportGarbage = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("Both"));
	TestFalse(TEXT("Garbage widget report should fail"), ReportGarbage.bSuccess);
	TestEqual(TEXT("Garbage availability should be Unavailable"), (uint8)ReportGarbage.Availability, (uint8)EInspectionAvailability::Unavailable);

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 11: Depth Truncation ──────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeDepthTruncationTest, "AccessibleUMGInspector.Runtime.DepthTruncation", TEST_FLAGS)
bool FRuntimeDepthTruncationTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_DepthTest"));

	// Inspect with shallow MaxDepth = 1 (UserWidget is depth 0, RootCanvas is depth 1, VertBox is depth 2)
	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 1, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);
	TestTrue(TEXT("Truncated flag set"), Report.bTruncated);

	for (const FRuntimeWidgetRecord& W : Report.Widgets)
	{
		TestTrue(TEXT("Widget depth within limit"), W.Depth <= 1);
	}

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 12: Node Count Limit ──────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeNodeCountLimitTest, "AccessibleUMGInspector.Runtime.NodeCountLimit", TEST_FLAGS)
bool FRuntimeNodeCountLimitTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UTestConcreteUserWidget* Widget = NewObject<UTestConcreteUserWidget>(World, TEXT("WBP_LargeWidget"));
	Widget->WidgetTree = NewObject<UWidgetTree>(Widget);
	UVerticalBox* VertBox = Widget->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LargeVertBox"));
	Widget->WidgetTree->RootWidget = VertBox;

	// Add 550 buttons to exceed limit of 500
	for (int32 i = 0; i < 550; ++i)
	{
		UButton* Btn = Widget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("Btn_%d"), i));
		VertBox->AddChildToVerticalBox(Btn);
	}

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 100, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);
	TestTrue(TEXT("Report truncated"), Report.bTruncated);
	TestEqual(TEXT("Widgets count clamped at 500"), Report.Widgets.Num(), 500);

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 13: No TakeWidget Call On Unconstructed Widgets ───────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeNoTakeWidgetTest, "AccessibleUMGInspector.Runtime.NoTakeWidget", TEST_FLAGS)
bool FRuntimeNoTakeWidgetTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_NoTakeWidgetTest"));

	// Before inspection: GetCachedWidget must be null
	TestFalse(TEXT("CachedWidget null before inspection"), Widget->GetCachedWidget().IsValid());

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);

	// After inspection: GetCachedWidget must STILL be null (proving TakeWidget was never invoked)
	TestFalse(TEXT("CachedWidget remains null after inspection"), Widget->GetCachedWidget().IsValid());

	const FRuntimeWidgetRecord* RootRec = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("WBP_NoTakeWidgetTest"));
	TestNotNull(TEXT("Root found"), RootRec);
	TestFalse(TEXT("Report records no Slate widget"), RootRec->bHasSlateWidget);

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 14: Hovered Detection ─────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeHoveredDetectionTest, "AccessibleUMGInspector.Runtime.HoveredDetection", TEST_FLAGS)
bool FRuntimeHoveredDetectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_HoverTest"));

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);

	const FRuntimeWidgetRecord* Primary = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Primary"));
	TestNotNull(TEXT("Primary found"), Primary);
	// In headless unhovered state, bIsHovered should be false
	TestFalse(TEXT("Primary is not hovered"), Primary->bIsHovered);

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 15: Navigation Rules Extraction ───────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeNavigationRulesTest, "AccessibleUMGInspector.Runtime.NavigationRules", TEST_FLAGS)
bool FRuntimeNavigationRulesTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_NavTest"));

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);

	const FRuntimeWidgetRecord* Primary = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Primary"));
	TestNotNull(TEXT("Primary found"), Primary);
	TestEqual(TEXT("Primary has 1 navigation rule"), Primary->NavigationRules.Num(), 1);
	TestEqual(TEXT("Rule direction Down"), Primary->NavigationRules[0].Direction, TEXT("Down"));
	TestEqual(TEXT("Rule Explicit"), Primary->NavigationRules[0].Rule, TEXT("Explicit"));
	TestEqual(TEXT("TargetWidget Button_Secondary"), Primary->NavigationRules[0].TargetWidget, TEXT("Button_Secondary"));

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 16: Slot Properties Extraction ────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeSlotPropertiesTest, "AccessibleUMGInspector.Runtime.SlotProperties", TEST_FLAGS)
bool FRuntimeSlotPropertiesTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_SlotTest"));

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);

	const FRuntimeWidgetRecord* VertBoxRec = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("MainVerticalBox"));
	TestNotNull(TEXT("VertBox found"), VertBoxRec);
	TestTrue(TEXT("VertBox has slot"), VertBoxRec->bHasSlot);
	TestEqual(TEXT("Slot type CanvasPanelSlot"), VertBoxRec->Slot.SlotType, TEXT("CanvasPanelSlot"));
	TestEqual(TEXT("Parent widget RootCanvas"), VertBoxRec->Slot.ParentWidgetName, TEXT("RootCanvas"));

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 17: Accessible Text Extraction ────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeAccessibleTextExtractionTest, "AccessibleUMGInspector.Runtime.AccessibleTextExtraction", TEST_FLAGS)
bool FRuntimeAccessibleTextExtractionTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_A11yTextTest"));

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("Both"));
	TestTrue(TEXT("Report success"), Report.bSuccess);

	const FRuntimeWidgetRecord* Primary = RuntimeWidgetInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Primary"));
	TestNotNull(TEXT("Primary found"), Primary);
#if WITH_EDITORONLY_DATA
	TestEqual(TEXT("AccessibleName extracted"), Primary->AccessibleName, TEXT("Primary Action"));
	TestEqual(TEXT("AccessibleSummary extracted"), Primary->AccessibleSummary, TEXT("Triggers main operation"));
	TestEqual(TEXT("AccessibleBehavior Custom"), Primary->AccessibleBehavior, TEXT("Custom"));
#endif

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 18: JSON Validity & Serialization ─────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeJsonValidityTest, "AccessibleUMGInspector.Runtime.JsonValidity", TEST_FLAGS)
bool FRuntimeJsonValidityTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_JsonTest"));

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("Json"));
	TestTrue(TEXT("Report success"), Report.bSuccess);
	TestTrue(TEXT("JsonData non-empty"), !Report.JsonData.IsEmpty());

	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Report.JsonData);
	bool bParsed = FJsonSerializer::Deserialize(Reader, JsonObj);
	TestTrue(TEXT("JSON deserialization succeeded"), bParsed);
	TestNotNull(TEXT("JSON root object valid"), JsonObj.Get());

	bool bSuccessField = false;
	TestTrue(TEXT("bSuccess field present"), JsonObj->TryGetBoolField(TEXT("bSuccess"), bSuccessField));
	TestTrue(TEXT("bSuccess is true"), bSuccessField);

	const TArray<TSharedPtr<FJsonValue>>* WidgetsArray = nullptr;
	TestTrue(TEXT("Widgets array present"), JsonObj->TryGetArrayField(TEXT("widgets"), WidgetsArray));
	TestNotNull(TEXT("Widgets array valid"), WidgetsArray);
	TestEqual(TEXT("Widgets array has 9 entries"), WidgetsArray->Num(), 9);
	TestEqual(TEXT("Widgets array matches report total"), WidgetsArray->Num(), Report.TotalWidgets);

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 19: NVDA Text Formatting ──────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeNvdaFormattingTest, "AccessibleUMGInspector.Runtime.NvdaFormatting", TEST_FLAGS)
bool FRuntimeNvdaFormattingTest::RunTest(const FString& Parameters)
{
	UWorld* World = RuntimeWidgetInspectorTestHelpers::CreateSyntheticWorld();
	TestNotNull(TEXT("World created"), World);

	UUserWidget* Widget = RuntimeWidgetInspectorTestHelpers::CreateSyntheticUserWidget(World, TEXT("WBP_NvdaTest"));

	FRuntimeWidgetReport Report = FRuntimeWidgetInspector::InspectUserWidgetInstance(Widget, 32, TEXT("NvdaText"));
	TestTrue(TEXT("Report success"), Report.bSuccess);
	TestTrue(TEXT("NvdaText non-empty"), !Report.NvdaText.IsEmpty());

	// Must contain headline
	TestTrue(TEXT("Headline present"), Report.NvdaText.Contains(TEXT("Accessible UMG Inspector: WBP_NvdaTest [Runtime Viewport / UserWidget]")));

	// Must contain Active Controls Hierarchy
	TestTrue(TEXT("Active controls section present"), Report.NvdaText.Contains(TEXT("Active Controls Hierarchy:")));

	// Must contain hierarchical numbering
	TestTrue(TEXT("Hierarchical numbering 1. present"), Report.NvdaText.Contains(TEXT("1. WBP_NvdaTest")));

	// Must contain Inactive / Collapsed / Hidden section
	TestTrue(TEXT("Hidden section present"), Report.NvdaText.Contains(TEXT("Inactive / Collapsed / Hidden Subtrees:")));
	TestTrue(TEXT("Collapsed button listed in hidden section"), Report.NvdaText.Contains(TEXT("Button_Collapsed")));
	TestTrue(TEXT("Hidden button listed in hidden section"), Report.NvdaText.Contains(TEXT("Button_Hidden")));

	RuntimeWidgetInspectorTestHelpers::DestroySyntheticWorld(World);
	return true;
}

// ── Test 20: Capabilities & Milestone 1 Regression ─────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeCapabilitiesAndRegressionTest, "AccessibleUMGInspector.Runtime.CapabilitiesAndRegression", TEST_FLAGS)
bool FRuntimeCapabilitiesAndRegressionTest::RunTest(const FString& Parameters)
{
	// 1. Toolset capabilities include InspectRuntimeUserWidget
	FInspectorCapabilities Caps = UAccessibleUMGInspectorToolset::GetInspectorCapabilities();
	TestTrue(TEXT("Caps includes InspectSerializedWidgetBlueprint"), Caps.SupportedOperations.Contains(TEXT("InspectSerializedWidgetBlueprint")));
	TestTrue(TEXT("Caps includes InspectRuntimeUserWidget"), Caps.SupportedOperations.Contains(TEXT("InspectRuntimeUserWidget")));

	// 2. Milestone 1 serialized blueprint inspection remains functional
	UWidgetBlueprint* BP = NewObject<UWidgetBlueprint>(GetTransientPackage(), TEXT("WBP_M1Regression"));
	BP->WidgetTree = NewObject<UWidgetTree>(BP);
	UCanvasPanel* RootCanvas = BP->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	BP->WidgetTree->RootWidget = RootCanvas;

	FSerializedWidgetBlueprintReport SerializedReport = FSerializedWidgetInspector::InspectBlueprint(BP, TEXT("Both"));
	TestTrue(TEXT("Serialized report succeeds"), SerializedReport.bSuccess);
	TestEqual(TEXT("Serialized report total widgets"), SerializedReport.TotalWidgets, 1);
	TestEqual(TEXT("Serialized root widget name"), SerializedReport.Widgets[0].WidgetName, TEXT("RootCanvas"));

	return true;
}

// ── Test 21: Visible child under Collapsed parent ─────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeVisibleChildUnderCollapsedParentTest, "AccessibleUMGInspector.Runtime.VisibleChildUnderCollapsedParent", TEST_FLAGS)
bool FRuntimeVisibleChildUnderCollapsedParentTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport Report;
	Report.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("Root");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1 };
	Report.Widgets.Add(Root);

	FRuntimeWidgetRecord Parent;
	Parent.NodeIndex = 1;
	Parent.ParentIndex = 0;
	Parent.WidgetName = TEXT("CollapsedParent");
	Parent.LocalVisibility = TEXT("Collapsed");
	Parent.bIsEnabled = true;
	Parent.ChildIndices = { 2 };
	Report.Widgets.Add(Parent);

	FRuntimeWidgetRecord Child;
	Child.NodeIndex = 2;
	Child.ParentIndex = 1;
	Child.WidgetName = TEXT("VisibleChild");
	Child.LocalVisibility = TEXT("Visible");
	Child.bIsEnabled = true;
	Child.bIsFocusable = true;
	Report.Widgets.Add(Child);

	FRuntimeWidgetInspector::ComputeEffectiveVisibility(Report);

	const FRuntimeWidgetRecord& EvaluatedChild = Report.Widgets[2];
	TestEqual(TEXT("Child local visibility preserved as Visible"), EvaluatedChild.LocalVisibility, TEXT("Visible"));
	TestEqual(TEXT("Child effective visibility is Collapsed"), EvaluatedChild.EffectiveVisibility, TEXT("Collapsed"));
	TestFalse(TEXT("Child is not effectively visible"), EvaluatedChild.bEffectivelyVisible);
	TestFalse(TEXT("Child is not effectively interactive"), EvaluatedChild.bEffectivelyInteractive);
	TestFalse(TEXT("Child is not effectively focusable"), EvaluatedChild.bEffectivelyFocusable);
	TestEqual(TEXT("Blocking ancestor index is 1"), EvaluatedChild.BlockingAncestorIndex, 1);
	TestEqual(TEXT("Blocking ancestor name is CollapsedParent"), EvaluatedChild.BlockingAncestorName, TEXT("CollapsedParent"));

	return true;
}

// ── Test 22: Visible child under Hidden parent ────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeVisibleChildUnderHiddenParentTest, "AccessibleUMGInspector.Runtime.VisibleChildUnderHiddenParent", TEST_FLAGS)
bool FRuntimeVisibleChildUnderHiddenParentTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport Report;
	Report.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("Root");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1 };
	Report.Widgets.Add(Root);

	FRuntimeWidgetRecord Parent;
	Parent.NodeIndex = 1;
	Parent.ParentIndex = 0;
	Parent.WidgetName = TEXT("HiddenParent");
	Parent.LocalVisibility = TEXT("Hidden");
	Parent.bIsEnabled = true;
	Parent.ChildIndices = { 2 };
	Report.Widgets.Add(Parent);

	FRuntimeWidgetRecord Child;
	Child.NodeIndex = 2;
	Child.ParentIndex = 1;
	Child.WidgetName = TEXT("VisibleChild");
	Child.LocalVisibility = TEXT("Visible");
	Child.bIsEnabled = true;
	Report.Widgets.Add(Child);

	FRuntimeWidgetInspector::ComputeEffectiveVisibility(Report);

	const FRuntimeWidgetRecord& EvaluatedChild = Report.Widgets[2];
	TestEqual(TEXT("Child effective visibility is Hidden"), EvaluatedChild.EffectiveVisibility, TEXT("Hidden"));
	TestFalse(TEXT("Child is not effectively visible"), EvaluatedChild.bEffectivelyVisible);
	TestEqual(TEXT("Blocking ancestor index is 1"), EvaluatedChild.BlockingAncestorIndex, 1);
	TestEqual(TEXT("Blocking ancestor name is HiddenParent"), EvaluatedChild.BlockingAncestorName, TEXT("HiddenParent"));

	return true;
}

// ── Test 23: Visible grandchild under Collapsed grandparent ───────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeVisibleGrandchildUnderCollapsedGrandparentTest, "AccessibleUMGInspector.Runtime.VisibleGrandchildUnderCollapsedGrandparent", TEST_FLAGS)
bool FRuntimeVisibleGrandchildUnderCollapsedGrandparentTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport Report;
	Report.bSuccess = true;

	FRuntimeWidgetRecord Grandparent;
	Grandparent.NodeIndex = 0;
	Grandparent.ParentIndex = -1;
	Grandparent.WidgetName = TEXT("CollapsedGrandparent");
	Grandparent.LocalVisibility = TEXT("Collapsed");
	Grandparent.bIsEnabled = true;
	Grandparent.ChildIndices = { 1 };
	Report.Widgets.Add(Grandparent);

	FRuntimeWidgetRecord Parent;
	Parent.NodeIndex = 1;
	Parent.ParentIndex = 0;
	Parent.WidgetName = TEXT("VisibleParent");
	Parent.LocalVisibility = TEXT("Visible");
	Parent.bIsEnabled = true;
	Parent.ChildIndices = { 2 };
	Report.Widgets.Add(Parent);

	FRuntimeWidgetRecord Grandchild;
	Grandchild.NodeIndex = 2;
	Grandchild.ParentIndex = 1;
	Grandchild.WidgetName = TEXT("VisibleGrandchild");
	Grandchild.LocalVisibility = TEXT("Visible");
	Grandchild.bIsEnabled = true;
	Report.Widgets.Add(Grandchild);

	FRuntimeWidgetInspector::ComputeEffectiveVisibility(Report);

	const FRuntimeWidgetRecord& EvaluatedGrandchild = Report.Widgets[2];
	TestEqual(TEXT("Grandchild effective visibility is Collapsed"), EvaluatedGrandchild.EffectiveVisibility, TEXT("Collapsed"));
	TestFalse(TEXT("Grandchild is not effectively visible"), EvaluatedGrandchild.bEffectivelyVisible);
	TestEqual(TEXT("Nearest blocking ancestor is grandparent index 0"), EvaluatedGrandchild.BlockingAncestorIndex, 0);
	TestEqual(TEXT("Blocking ancestor name is CollapsedGrandparent"), EvaluatedGrandchild.BlockingAncestorName, TEXT("CollapsedGrandparent"));

	return true;
}

// ── Test 24: Disabled ancestor propagation ────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeDisabledAncestorPropagationTest, "AccessibleUMGInspector.Runtime.DisabledAncestorPropagation", TEST_FLAGS)
bool FRuntimeDisabledAncestorPropagationTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport Report;
	Report.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("Root");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1 };
	Report.Widgets.Add(Root);

	FRuntimeWidgetRecord DisabledParent;
	DisabledParent.NodeIndex = 1;
	DisabledParent.ParentIndex = 0;
	DisabledParent.WidgetName = TEXT("DisabledParent");
	DisabledParent.LocalVisibility = TEXT("Visible");
	DisabledParent.bIsEnabled = false; // Disabled!
	DisabledParent.ChildIndices = { 2 };
	Report.Widgets.Add(DisabledParent);

	FRuntimeWidgetRecord EnabledChild;
	EnabledChild.NodeIndex = 2;
	EnabledChild.ParentIndex = 1;
	EnabledChild.WidgetName = TEXT("EnabledChild");
	EnabledChild.LocalVisibility = TEXT("Visible");
	EnabledChild.bIsEnabled = true; // Authored as enabled
	EnabledChild.bIsFocusable = true;
	Report.Widgets.Add(EnabledChild);

	FRuntimeWidgetInspector::ComputeEffectiveVisibility(Report);

	const FRuntimeWidgetRecord& EvaluatedChild = Report.Widgets[2];
	TestTrue(TEXT("Local enabled preserved as true"), EvaluatedChild.bIsEnabled);
	TestFalse(TEXT("Effective interaction is false due to disabled ancestor"), EvaluatedChild.bEffectivelyInteractive);
	TestFalse(TEXT("Effective focusability is false due to disabled ancestor"), EvaluatedChild.bEffectivelyFocusable);

	return true;
}

// ── Test 25: HitTestInvisible propagation ─────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeHitTestInvisiblePropagationTest, "AccessibleUMGInspector.Runtime.HitTestInvisiblePropagation", TEST_FLAGS)
bool FRuntimeHitTestInvisiblePropagationTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport Report;
	Report.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("Root");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1 };
	Report.Widgets.Add(Root);

	FRuntimeWidgetRecord HTIParent;
	HTIParent.NodeIndex = 1;
	HTIParent.ParentIndex = 0;
	HTIParent.WidgetName = TEXT("HTIParent");
	HTIParent.LocalVisibility = TEXT("HitTestInvisible");
	HTIParent.bIsEnabled = true;
	HTIParent.ChildIndices = { 2 };
	Report.Widgets.Add(HTIParent);

	FRuntimeWidgetRecord Child;
	Child.NodeIndex = 2;
	Child.ParentIndex = 1;
	Child.WidgetName = TEXT("Child");
	Child.LocalVisibility = TEXT("Visible");
	Child.bIsEnabled = true;
	Child.bIsFocusable = true;
	Report.Widgets.Add(Child);

	FRuntimeWidgetInspector::ComputeEffectiveVisibility(Report);

	const FRuntimeWidgetRecord& EvaluatedChild = Report.Widgets[2];
	TestEqual(TEXT("Child effective visibility is HitTestInvisible"), EvaluatedChild.EffectiveVisibility, TEXT("HitTestInvisible"));
	TestTrue(TEXT("Child is effectively visible"), EvaluatedChild.bEffectivelyVisible);
	TestFalse(TEXT("Child is not effectively interactive under HitTestInvisible"), EvaluatedChild.bEffectivelyInteractive);

	return true;
}

// ── Test 26: SelfHitTestInvisible child behavior ──────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeSelfHitTestInvisibleChildBehaviorTest, "AccessibleUMGInspector.Runtime.SelfHitTestInvisibleChildBehavior", TEST_FLAGS)
bool FRuntimeSelfHitTestInvisibleChildBehaviorTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport Report;
	Report.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("Root");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1 };
	Report.Widgets.Add(Root);

	FRuntimeWidgetRecord SHTIParent;
	SHTIParent.NodeIndex = 1;
	SHTIParent.ParentIndex = 0;
	SHTIParent.WidgetName = TEXT("SHTIParent");
	SHTIParent.LocalVisibility = TEXT("SelfHitTestInvisible");
	SHTIParent.bIsEnabled = true;
	SHTIParent.ChildIndices = { 2 };
	Report.Widgets.Add(SHTIParent);

	FRuntimeWidgetRecord Child;
	Child.NodeIndex = 2;
	Child.ParentIndex = 1;
	Child.WidgetName = TEXT("ChildButton");
	Child.LocalVisibility = TEXT("Visible");
	Child.bIsEnabled = true;
	Child.bIsFocusable = true;
	Report.Widgets.Add(Child);

	FRuntimeWidgetInspector::ComputeEffectiveVisibility(Report);

	const FRuntimeWidgetRecord& EvaluatedParent = Report.Widgets[1];
	TestEqual(TEXT("Parent effective visibility is SelfHitTestInvisible"), EvaluatedParent.EffectiveVisibility, TEXT("SelfHitTestInvisible"));
	TestFalse(TEXT("Parent itself is not effectively interactive"), EvaluatedParent.bEffectivelyInteractive);

	const FRuntimeWidgetRecord& EvaluatedChild = Report.Widgets[2];
	TestEqual(TEXT("Child effective visibility remains Visible"), EvaluatedChild.EffectiveVisibility, TEXT("Visible"));
	TestTrue(TEXT("Child remains effectively visible"), EvaluatedChild.bEffectivelyVisible);
	TestTrue(TEXT("Child remains effectively interactive"), EvaluatedChild.bEffectivelyInteractive);

	return true;
}

// ── Test 27: Cached geometry under collapsed ancestor ─────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeCachedGeometryUnderCollapsedAncestorTest, "AccessibleUMGInspector.Runtime.CachedGeometryUnderCollapsedAncestor", TEST_FLAGS)
bool FRuntimeCachedGeometryUnderCollapsedAncestorTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport Report;
	Report.bSuccess = true;

	FRuntimeWidgetRecord Parent;
	Parent.NodeIndex = 0;
	Parent.ParentIndex = -1;
	Parent.WidgetName = TEXT("CollapsedParent");
	Parent.LocalVisibility = TEXT("Collapsed");
	Parent.bIsEnabled = true;
	Parent.ChildIndices = { 1 };
	Report.Widgets.Add(Parent);

	FRuntimeWidgetRecord Child;
	Child.NodeIndex = 1;
	Child.ParentIndex = 0;
	Child.WidgetName = TEXT("ChildWithCachedBounds");
	Child.LocalVisibility = TEXT("Visible");
	Child.bIsEnabled = true;
	Child.ScreenBounds.X = 100.f;
	Child.ScreenBounds.Y = 200.f;
	Child.ScreenBounds.Width = 300.f;
	Child.ScreenBounds.Height = 50.f;
	Child.ScreenBounds.bIsValid = true;
	Report.Widgets.Add(Child);

	FRuntimeWidgetInspector::ComputeEffectiveVisibility(Report);

	const FRuntimeWidgetRecord& EvaluatedChild = Report.Widgets[1];
	TestFalse(TEXT("Effective geometry is not available under collapsed ancestor"), EvaluatedChild.bEffectiveGeometryAvailable);
	TestFalse(TEXT("Child is not effectively visible"), EvaluatedChild.bEffectivelyVisible);
	TestTrue(TEXT("Limitations record collapsed geometry suppression"), EvaluatedChild.EffectiveStateLimitations.Num() > 0);

	return true;
}

// ── Test 28: Local focusability under collapsed ancestor ──────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeLocalFocusabilityUnderCollapsedAncestorTest, "AccessibleUMGInspector.Runtime.LocalFocusabilityUnderCollapsedAncestor", TEST_FLAGS)
bool FRuntimeLocalFocusabilityUnderCollapsedAncestorTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport Report;
	Report.bSuccess = true;

	FRuntimeWidgetRecord Parent;
	Parent.NodeIndex = 0;
	Parent.ParentIndex = -1;
	Parent.WidgetName = TEXT("CollapsedParent");
	Parent.LocalVisibility = TEXT("Collapsed");
	Parent.bIsEnabled = true;
	Parent.ChildIndices = { 1 };
	Report.Widgets.Add(Parent);

	FRuntimeWidgetRecord Child;
	Child.NodeIndex = 1;
	Child.ParentIndex = 0;
	Child.WidgetName = TEXT("FocusableChild");
	Child.LocalVisibility = TEXT("Visible");
	Child.bIsEnabled = true;
	Child.bIsFocusable = true; // Authored focusable
	Report.Widgets.Add(Child);

	FRuntimeWidgetInspector::ComputeEffectiveVisibility(Report);

	const FRuntimeWidgetRecord& EvaluatedChild = Report.Widgets[1];
	TestTrue(TEXT("Local focusability preserved as true"), EvaluatedChild.bIsFocusable);
	TestFalse(TEXT("Effective focusability is false"), EvaluatedChild.bEffectivelyFocusable);

	return true;
}

// ── Test 29: Effective-state deterministic calculation ────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeEffectiveStateDeterministicCalculationTest, "AccessibleUMGInspector.Runtime.EffectiveStateDeterministicCalculation", TEST_FLAGS)
bool FRuntimeEffectiveStateDeterministicCalculationTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport Report;
	Report.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("Root");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1 };
	Report.Widgets.Add(Root);

	FRuntimeWidgetRecord Panel;
	Panel.NodeIndex = 1;
	Panel.ParentIndex = 0;
	Panel.WidgetName = TEXT("Panel");
	Panel.LocalVisibility = TEXT("Collapsed");
	Panel.bIsEnabled = true;
	Panel.ChildIndices = { 2 };
	Report.Widgets.Add(Panel);

	FRuntimeWidgetRecord Button;
	Button.NodeIndex = 2;
	Button.ParentIndex = 1;
	Button.WidgetName = TEXT("Button");
	Button.LocalVisibility = TEXT("Visible");
	Button.bIsEnabled = true;
	Button.bIsFocusable = true;
	Report.Widgets.Add(Button);

	// First pass
	FRuntimeWidgetInspector::ComputeEffectiveVisibility(Report);
	FString FirstEffectiveVis = Report.Widgets[2].EffectiveVisibility;
	int32 FirstBlockingIdx = Report.Widgets[2].BlockingAncestorIndex;
	int32 FirstLimitCount = Report.Widgets[2].EffectiveStateLimitations.Num();

	// Second pass: should be idempotent and deterministic
	FRuntimeWidgetInspector::ComputeEffectiveVisibility(Report);
	TestEqual(TEXT("Effective visibility is deterministic"), Report.Widgets[2].EffectiveVisibility, FirstEffectiveVis);
	TestEqual(TEXT("Blocking index is deterministic"), Report.Widgets[2].BlockingAncestorIndex, FirstBlockingIdx);
	TestEqual(TEXT("Limitations count does not duplicate"), Report.Widgets[2].EffectiveStateLimitations.Num(), FirstLimitCount);

	return true;
}

// ── Test 30: Blocking ancestor identity ────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRuntimeBlockingAncestorIdentityTest, "AccessibleUMGInspector.Runtime.BlockingAncestorIdentity", TEST_FLAGS)
bool FRuntimeBlockingAncestorIdentityTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport Report;
	Report.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("CanvasRoot");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1 };
	Report.Widgets.Add(Root);

	FRuntimeWidgetRecord SectionA;
	SectionA.NodeIndex = 1;
	SectionA.ParentIndex = 0;
	SectionA.WidgetName = TEXT("SectionA_Visible");
	SectionA.LocalVisibility = TEXT("Visible");
	SectionA.bIsEnabled = true;
	SectionA.ChildIndices = { 2 };
	Report.Widgets.Add(SectionA);

	FRuntimeWidgetRecord ContainerB;
	ContainerB.NodeIndex = 2;
	ContainerB.ParentIndex = 1;
	ContainerB.WidgetName = TEXT("ContainerB_Collapsed");
	ContainerB.LocalVisibility = TEXT("Collapsed");
	ContainerB.bIsEnabled = true;
	ContainerB.ChildIndices = { 3 };
	Report.Widgets.Add(ContainerB);

	FRuntimeWidgetRecord ButtonC;
	ButtonC.NodeIndex = 3;
	ButtonC.ParentIndex = 2;
	ButtonC.WidgetName = TEXT("ButtonC_Visible");
	ButtonC.LocalVisibility = TEXT("Visible");
	ButtonC.bIsEnabled = true;
	Report.Widgets.Add(ButtonC);

	FRuntimeWidgetInspector::ComputeEffectiveVisibility(Report);

	const FRuntimeWidgetRecord& EvaluatedButton = Report.Widgets[3];
	TestEqual(TEXT("Blocking ancestor index is exactly 2"), EvaluatedButton.BlockingAncestorIndex, 2);
	TestEqual(TEXT("Blocking ancestor name is ContainerB_Collapsed"), EvaluatedButton.BlockingAncestorName, TEXT("ContainerB_Collapsed"));

	return true;
}
