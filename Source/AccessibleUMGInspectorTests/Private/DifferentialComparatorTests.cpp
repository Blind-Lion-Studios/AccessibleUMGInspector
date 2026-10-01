// Copyright Blind Lion Studios. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "DifferentialComparator.h"
#include "SerializedWidgetInspector.h"
#include "RuntimeWidgetInspector.h"
#include "NvdaTextFormatter.h"
#include "AccessibleUMGInspectorToolset.h"
#include "AccessibleUMGInspectorTypes.h"
#include "TestConcreteUserWidget.h"

#include "WidgetBlueprint.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#define TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace DifferentialComparatorTestHelpers
{
	static FSerializedWidgetBlueprintReport CreateMockSerializedReport()
	{
		FSerializedWidgetBlueprintReport S;
		S.bSuccess = true;
		S.AssetPath = TEXT("/Game/UI/WBP_MockMenu");
		S.ParentClass = TEXT("UserWidget");

		// Node 0: RootCanvas
		FSerializedWidgetRecord R0;
		R0.NodeIndex = 0;
		R0.ParentIndex = -1;
		R0.WidgetName = TEXT("RootCanvas");
		R0.WidgetClass = TEXT("CanvasPanel");
		R0.ParentName = TEXT("None");
		R0.ChildIndex = 0;
		R0.DefaultVisibility = TEXT("Visible");
		R0.bIsEnabled = true;
		R0.ChildIndices = { 1 };
		S.Widgets.Add(R0);

		// Node 1: MainVerticalBox
		FSerializedWidgetRecord R1;
		R1.NodeIndex = 1;
		R1.ParentIndex = 0;
		R1.WidgetName = TEXT("MainVerticalBox");
		R1.WidgetClass = TEXT("VerticalBox");
		R1.ParentName = TEXT("RootCanvas");
		R1.ChildIndex = 0;
		R1.DefaultVisibility = TEXT("Visible");
		R1.bIsEnabled = true;
		R1.ChildIndices = { 2, 3, 4 };
		S.Widgets.Add(R1);

		// Node 2: Button_Start
		FSerializedWidgetRecord R2;
		R2.NodeIndex = 2;
		R2.ParentIndex = 1;
		R2.WidgetName = TEXT("Button_Start");
		R2.WidgetClass = TEXT("Button");
		R2.ParentName = TEXT("MainVerticalBox");
		R2.ChildIndex = 0;
		R2.DefaultVisibility = TEXT("Visible");
		R2.bIsEnabled = true;
		R2.bIsFocusable = true;
		R2.AccessibleName = TEXT("Start Game");
		R2.AccessibleSummary = TEXT("Begins the game");
		S.Widgets.Add(R2);

		// Node 3: Button_Settings
		FSerializedWidgetRecord R3;
		R3.NodeIndex = 3;
		R3.ParentIndex = 1;
		R3.WidgetName = TEXT("Button_Settings");
		R3.WidgetClass = TEXT("Button");
		R3.ParentName = TEXT("MainVerticalBox");
		R3.ChildIndex = 1;
		R3.DefaultVisibility = TEXT("Visible");
		R3.bIsEnabled = true;
		R3.bIsFocusable = true;
		R3.AccessibleName = TEXT("Settings");
		S.Widgets.Add(R3);

		// Node 4: Button_CollapsedPromo (Default Collapsed)
		FSerializedWidgetRecord R4;
		R4.NodeIndex = 4;
		R4.ParentIndex = 1;
		R4.WidgetName = TEXT("Button_CollapsedPromo");
		R4.WidgetClass = TEXT("Button");
		R4.ParentName = TEXT("MainVerticalBox");
		R4.ChildIndex = 2;
		R4.DefaultVisibility = TEXT("Collapsed");
		R4.bIsEnabled = false;
		S.Widgets.Add(R4);

		S.TotalWidgets = S.Widgets.Num();
		S.RootWidgetIndex = 0;
		return S;
	}

	static FRuntimeWidgetReport CreateMockRuntimeReport()
	{
		FRuntimeWidgetReport R;
		R.bSuccess = true;
		R.Availability = EInspectionAvailability::Available;
		R.WorldName = TEXT("PIEWorld_0");
		R.RootWidgetName = TEXT("WBP_MockMenu_C_0");
		R.RootWidgetClass = TEXT("WBP_MockMenu_C");

		// Node 0: UUserWidget Host
		FRuntimeWidgetRecord W0;
		W0.NodeIndex = 0;
		W0.ParentIndex = -1;
		W0.WidgetName = TEXT("WBP_MockMenu_C_0");
		W0.WidgetClass = TEXT("WBP_MockMenu_C");
		W0.ParentName = TEXT("None");
		W0.ChildIndex = 0;
		W0.RuntimeVisibility = TEXT("Visible");
		W0.bIsEnabled = true;
		W0.ChildIndices = { 1 };
		R.Widgets.Add(W0);

		// Node 1: RootCanvas
		FRuntimeWidgetRecord W1;
		W1.NodeIndex = 1;
		W1.ParentIndex = 0;
		W1.WidgetName = TEXT("RootCanvas");
		W1.WidgetClass = TEXT("CanvasPanel");
		W1.ParentName = TEXT("WBP_MockMenu_C_0");
		W1.ChildIndex = 0;
		W1.RuntimeVisibility = TEXT("Visible");
		W1.bIsEnabled = true;
		W1.ChildIndices = { 2 };
		R.Widgets.Add(W1);

		// Node 2: MainVerticalBox
		FRuntimeWidgetRecord W2;
		W2.NodeIndex = 2;
		W2.ParentIndex = 1;
		W2.WidgetName = TEXT("MainVerticalBox");
		W2.WidgetClass = TEXT("VerticalBox");
		W2.ParentName = TEXT("RootCanvas");
		W2.ChildIndex = 0;
		W2.RuntimeVisibility = TEXT("Visible");
		W2.bIsEnabled = true;
		W2.ChildIndices = { 3, 4, 5 };
		R.Widgets.Add(W2);

		// Node 3: Button_Start (Visible, Matched)
		FRuntimeWidgetRecord W3;
		W3.NodeIndex = 3;
		W3.ParentIndex = 2;
		W3.WidgetName = TEXT("Button_Start");
		W3.WidgetClass = TEXT("Button");
		W3.ParentName = TEXT("MainVerticalBox");
		W3.ChildIndex = 0;
		W3.RuntimeVisibility = TEXT("Visible");
		W3.bIsEnabled = true;
		W3.bIsFocusable = true;
		W3.bHasSlateWidget = true;
		W3.SlateWidgetType = TEXT("SButton");
		W3.ScreenBounds.X = 100.f;
		W3.ScreenBounds.Y = 200.f;
		W3.ScreenBounds.Width = 300.f;
		W3.ScreenBounds.Height = 60.f;
		W3.ScreenBounds.bIsValid = true;
		W3.AccessibleName = TEXT("Start Game");
		W3.AccessibleSummary = TEXT("Begins the game");
		R.Widgets.Add(W3);

		// Node 4: Button_Settings (Visible, Matched)
		FRuntimeWidgetRecord W4;
		W4.NodeIndex = 4;
		W4.ParentIndex = 2;
		W4.WidgetName = TEXT("Button_Settings");
		W4.WidgetClass = TEXT("Button");
		W4.ParentName = TEXT("MainVerticalBox");
		W4.ChildIndex = 1;
		W4.RuntimeVisibility = TEXT("Visible");
		W4.bIsEnabled = true;
		W4.bIsFocusable = true;
		W4.bHasSlateWidget = true;
		W4.SlateWidgetType = TEXT("SButton");
		W4.ScreenBounds.X = 100.f;
		W4.ScreenBounds.Y = 270.f;
		W4.ScreenBounds.Width = 300.f;
		W4.ScreenBounds.Height = 60.f;
		W4.ScreenBounds.bIsValid = true;
		W4.AccessibleName = TEXT("Settings");
		R.Widgets.Add(W4);

		// Node 5: DynamicRuntimeLabel (RuntimeOnly)
		FRuntimeWidgetRecord W5;
		W5.NodeIndex = 5;
		W5.ParentIndex = 2;
		W5.WidgetName = TEXT("DynamicRuntimeLabel");
		W5.WidgetClass = TEXT("TextBlock");
		W5.ParentName = TEXT("MainVerticalBox");
		W5.ChildIndex = 2;
		W5.RuntimeVisibility = TEXT("Visible");
		W5.bIsEnabled = true;
		W5.bHasSlateWidget = true;
		W5.SlateWidgetType = TEXT("STextBlock");
		W5.ScreenBounds.X = 100.f;
		W5.ScreenBounds.Y = 340.f;
		W5.ScreenBounds.Width = 300.f;
		W5.ScreenBounds.Height = 30.f;
		W5.ScreenBounds.bIsValid = true;
		R.Widgets.Add(W5);

		R.TotalWidgets = R.Widgets.Num();
		R.RootWidgetIndex = 0;
		return R;
	}

	static const FWidgetComparisonRecord* FindRecordByName(const FSerializedRuntimeComparisonReport& Report, const FString& Name)
	{
		for (const FWidgetComparisonRecord& Rec : Report.ComparisonRecords)
		{
			if (Rec.SerializedWidgetName == Name || Rec.RuntimeWidgetName == Name)
			{
				return &Rec;
			}
		}
		return nullptr;
	}
}

// ── Test 1: Exact Source-Identity Match ─────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffExactSourceIdentityMatchTest, "AccessibleUMGInspector.Differential.ExactSourceIdentityMatch", TEST_FLAGS)
bool FDiffExactSourceIdentityMatchTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	TestTrue(TEXT("Comparison succeeded"), Report.bSuccess);

	const FWidgetComparisonRecord* StartRec = DifferentialComparatorTestHelpers::FindRecordByName(Report, TEXT("Button_Start"));
	TestNotNull(TEXT("Button_Start record found"), StartRec);
	TestEqual(TEXT("Button_Start is Matched"), (uint8)StartRec->Classification, (uint8)EWidgetComparisonClassification::Matched);
	TestEqual(TEXT("Serialized node index"), StartRec->SerializedNodeIndex, 2);
	TestEqual(TEXT("Runtime node index"), StartRec->RuntimeNodeIndex, 3);
	TestTrue(TEXT("Evidence contains exact name"), StartRec->Evidence.Num() > 0);

	return true;
}

// ── Test 2: Name and Class Match Within Same Parent ────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffNameAndClassMatchWithinSameParentTest, "AccessibleUMGInspector.Differential.NameAndClassMatchWithinSameParent", TEST_FLAGS)
bool FDiffNameAndClassMatchWithinSameParentTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	const FWidgetComparisonRecord* SettingsRec = DifferentialComparatorTestHelpers::FindRecordByName(Report, TEXT("Button_Settings"));
	TestNotNull(TEXT("Button_Settings found"), SettingsRec);
	TestEqual(TEXT("Button_Settings is Matched"), (uint8)SettingsRec->Classification, (uint8)EWidgetComparisonClassification::Matched);
	TestEqual(TEXT("Parent name matches MainVerticalBox"), SettingsRec->SerializedParent, TEXT("MainVerticalBox"));

	return true;
}

// ── Test 3: Same Name Under Different Parents ──────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffSameNameUnderDifferentParentsTest, "AccessibleUMGInspector.Differential.SameNameUnderDifferentParents", TEST_FLAGS)
bool FDiffSameNameUnderDifferentParentsTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S;
	S.bSuccess = true;
	S.AssetPath = TEXT("/Game/UI/WBP_MultiParent");

	// S0: ParentA -> ChildBtn
	FSerializedWidgetRecord S_BtnA;
	S_BtnA.NodeIndex = 0;
	S_BtnA.WidgetName = TEXT("ActionBtn");
	S_BtnA.WidgetClass = TEXT("Button");
	S_BtnA.ParentName = TEXT("HeaderPanel");
	S.Widgets.Add(S_BtnA);

	// S1: ParentB -> ChildBtn
	FSerializedWidgetRecord S_BtnB;
	S_BtnB.NodeIndex = 1;
	S_BtnB.WidgetName = TEXT("ActionBtn");
	S_BtnB.WidgetClass = TEXT("Button");
	S_BtnB.ParentName = TEXT("FooterPanel");
	S.Widgets.Add(S_BtnB);

	FRuntimeWidgetReport R;
	R.bSuccess = true;
	R.RootWidgetName = TEXT("WBP_MultiParent_C_0");

	// R0: HeaderPanel -> ActionBtn
	FRuntimeWidgetRecord R_BtnA;
	R_BtnA.NodeIndex = 0;
	R_BtnA.WidgetName = TEXT("ActionBtn");
	R_BtnA.WidgetClass = TEXT("Button");
	R_BtnA.ParentName = TEXT("HeaderPanel");
	R.Widgets.Add(R_BtnA);

	// R1: FooterPanel -> ActionBtn
	FRuntimeWidgetRecord R_BtnB;
	R_BtnB.NodeIndex = 1;
	R_BtnB.WidgetName = TEXT("ActionBtn");
	R_BtnB.WidgetClass = TEXT("Button");
	R_BtnB.ParentName = TEXT("FooterPanel");
	R.Widgets.Add(R_BtnB);

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	TestTrue(TEXT("Comparison succeeded"), Report.bSuccess);
	TestEqual(TEXT("Both nodes matched by parent disambiguation"), Report.MatchedCount, 2);
	TestEqual(TEXT("Zero ambiguous"), Report.AmbiguousCount, 0);

	return true;
}

// ── Test 4: Duplicate Runtime Candidates Return Ambiguous ──────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffDuplicateRuntimeCandidatesAmbiguousTest, "AccessibleUMGInspector.Differential.DuplicateRuntimeCandidatesAmbiguous", TEST_FLAGS)
bool FDiffDuplicateRuntimeCandidatesAmbiguousTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S;
	S.bSuccess = true;
	FSerializedWidgetRecord S0;
	S0.NodeIndex = 0;
	S0.WidgetName = TEXT("DupButton");
	S0.WidgetClass = TEXT("Button");
	S0.ParentName = TEXT("PanelA");
	S.Widgets.Add(S0);

	FRuntimeWidgetReport R;
	R.bSuccess = true;
	// Two runtime buttons under PanelA with same name and class
	FRuntimeWidgetRecord R0;
	R0.NodeIndex = 0;
	R0.WidgetName = TEXT("DupButton");
	R0.WidgetClass = TEXT("Button");
	R0.ParentName = TEXT("PanelA");
	R.Widgets.Add(R0);

	FRuntimeWidgetRecord R1;
	R1.NodeIndex = 1;
	R1.WidgetName = TEXT("DupButton");
	R1.WidgetClass = TEXT("Button");
	R1.ParentName = TEXT("PanelA");
	R.Widgets.Add(R1);

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	TestTrue(TEXT("Report success"), Report.bSuccess);
	TestTrue(TEXT("Ambiguous candidates reported"), Report.AmbiguousCount >= 1);

	return true;
}

// ── Test 5: Duplicate Serialized Candidates Return Ambiguous ────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffDuplicateSerializedCandidatesAmbiguousTest, "AccessibleUMGInspector.Differential.DuplicateSerializedCandidatesAmbiguous", TEST_FLAGS)
bool FDiffDuplicateSerializedCandidatesAmbiguousTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S;
	S.bSuccess = true;
	// Two serialized buttons with identical name and parent
	FSerializedWidgetRecord S0;
	S0.NodeIndex = 0;
	S0.WidgetName = TEXT("DupBtn");
	S0.WidgetClass = TEXT("Button");
	S0.ParentName = TEXT("Container");
	S.Widgets.Add(S0);

	FSerializedWidgetRecord S1;
	S1.NodeIndex = 1;
	S1.WidgetName = TEXT("DupBtn");
	S1.WidgetClass = TEXT("Button");
	S1.ParentName = TEXT("Container");
	S.Widgets.Add(S1);

	FRuntimeWidgetReport R;
	R.bSuccess = true;
	FRuntimeWidgetRecord R0;
	R0.NodeIndex = 0;
	R0.WidgetName = TEXT("DupBtn");
	R0.WidgetClass = TEXT("Button");
	R0.ParentName = TEXT("Container");
	R.Widgets.Add(R0);

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	TestTrue(TEXT("Ambiguous reported"), Report.AmbiguousCount >= 1);

	return true;
}

// ── Test 6: Class Mismatch Prevents Exact Match ────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffClassMismatchPreventsExactMatchTest, "AccessibleUMGInspector.Differential.ClassMismatchPreventsExactMatch", TEST_FLAGS)
bool FDiffClassMismatchPreventsExactMatchTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S;
	S.bSuccess = true;
	FSerializedWidgetRecord S0;
	S0.NodeIndex = 0;
	S0.WidgetName = TEXT("ItemWidget");
	S0.WidgetClass = TEXT("Button"); // Serialized is Button
	S0.ParentName = TEXT("Root");
	S.Widgets.Add(S0);

	FRuntimeWidgetReport R;
	R.bSuccess = true;
	FRuntimeWidgetRecord R0;
	R0.NodeIndex = 0;
	R0.WidgetName = TEXT("ItemWidget");
	R0.WidgetClass = TEXT("TextBlock"); // Runtime is TextBlock
	R0.ParentName = TEXT("Root");
	R.Widgets.Add(R0);

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	TestEqual(TEXT("Zero matched due to class mismatch"), Report.MatchedCount, 0);
	TestTrue(TEXT("Reported as ambiguous / conflict"), Report.AmbiguousCount >= 1);

	return true;
}

// ── Test 7: Parent-Chain Mismatch ──────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffParentChainMismatchTest, "AccessibleUMGInspector.Differential.ParentChainMismatch", TEST_FLAGS)
bool FDiffParentChainMismatchTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S;
	S.bSuccess = true;
	FSerializedWidgetRecord S0;
	S0.NodeIndex = 0;
	S0.WidgetName = TEXT("UniqueBtn");
	S0.WidgetClass = TEXT("Button");
	S0.ParentName = TEXT("OriginalPanel");
	S.Widgets.Add(S0);

	FRuntimeWidgetReport R;
	R.bSuccess = true;
	FRuntimeWidgetRecord R0;
	R0.NodeIndex = 0;
	R0.WidgetName = TEXT("UniqueBtn");
	R0.WidgetClass = TEXT("Button");
	R0.ParentName = TEXT("ReparentedPanel"); // Different parent!
	R.Widgets.Add(R0);

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	const FWidgetComparisonRecord* Rec = DifferentialComparatorTestHelpers::FindRecordByName(Report, TEXT("UniqueBtn"));
	TestNotNull(TEXT("Record found"), Rec);
	TestTrue(TEXT("Contains parent mismatch discrepancy"), Rec->Discrepancies.Num() > 0);
	bool bFoundParentMismatch = false;
	for (const FString& Disc : Rec->Discrepancies)
	{
		if (Disc.Contains(TEXT("parent or containment mismatch")))
		{
			bFoundParentMismatch = true;
			break;
		}
	}
	TestTrue(TEXT("Discrepancy confirms parent mismatch"), bFoundParentMismatch);

	return true;
}

// ── Test 8: Child-Order Mismatch ───────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffChildOrderMismatchTest, "AccessibleUMGInspector.Differential.ChildOrderMismatch", TEST_FLAGS)
bool FDiffChildOrderMismatchTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S;
	S.bSuccess = true;
	FSerializedWidgetRecord S0;
	S0.NodeIndex = 0;
	S0.WidgetName = TEXT("OrderedBtn");
	S0.WidgetClass = TEXT("Button");
	S0.ParentName = TEXT("Panel");
	S0.ChildIndex = 0;
	S.Widgets.Add(S0);

	FRuntimeWidgetReport R;
	R.bSuccess = true;
	FRuntimeWidgetRecord R0;
	R0.NodeIndex = 0;
	R0.WidgetName = TEXT("OrderedBtn");
	R0.WidgetClass = TEXT("Button");
	R0.ParentName = TEXT("Panel");
	R0.ChildIndex = 3; // Shifted order!
	R.Widgets.Add(R0);

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	const FWidgetComparisonRecord* Rec = DifferentialComparatorTestHelpers::FindRecordByName(Report, TEXT("OrderedBtn"));
	TestNotNull(TEXT("Record found"), Rec);

	bool bFoundOrderMismatch = false;
	for (const FString& Disc : Rec->Discrepancies)
	{
		if (Disc.Contains(TEXT("child-order mismatch")))
		{
			bFoundOrderMismatch = true;
			break;
		}
	}
	TestTrue(TEXT("Discrepancy confirms child-order mismatch"), bFoundOrderMismatch);

	return true;
}

// ── Test 9: Serialized Visible and Runtime Visible Match ───────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffSerializedVisibleAndRuntimeVisibleMatchTest, "AccessibleUMGInspector.Differential.SerializedVisibleAndRuntimeVisibleMatch", TEST_FLAGS)
bool FDiffSerializedVisibleAndRuntimeVisibleMatchTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	const FWidgetComparisonRecord* StartRec = DifferentialComparatorTestHelpers::FindRecordByName(Report, TEXT("Button_Start"));
	TestNotNull(TEXT("Button_Start found"), StartRec);

	bool bHasVisibilityMismatch = false;
	for (const FString& Disc : StartRec->Discrepancies)
	{
		if (Disc.Contains(TEXT("visibility mismatch")))
		{
			bHasVisibilityMismatch = true;
			break;
		}
	}
	TestFalse(TEXT("No visibility mismatch on matching visible controls"), bHasVisibilityMismatch);

	return true;
}

// ── Test 10: Serialized Collapsed No Live Slate Valid SerializedOnly ────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffSerializedCollapsedValidSerializedOnlyTest, "AccessibleUMGInspector.Differential.SerializedCollapsedValidSerializedOnly", TEST_FLAGS)
bool FDiffSerializedCollapsedValidSerializedOnlyTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	const FWidgetComparisonRecord* CollapsedRec = DifferentialComparatorTestHelpers::FindRecordByName(Report, TEXT("Button_CollapsedPromo"));
	TestNotNull(TEXT("Button_CollapsedPromo found"), CollapsedRec);
	TestEqual(TEXT("Classification is SerializedOnly"), (uint8)CollapsedRec->Classification, (uint8)EWidgetComparisonClassification::SerializedOnly);

	bool bFoundValidEvidence = false;
	for (const FString& Ev : CollapsedRec->Evidence)
	{
		if (Ev.Contains(TEXT("Valid SerializedOnly")))
		{
			bFoundValidEvidence = true;
			break;
		}
	}
	TestTrue(TEXT("Evidence explains valid SerializedOnly collapsed control"), bFoundValidEvidence);

	return true;
}

// ── Test 11: Dynamically Created Runtime Control Is RuntimeOnly ────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffDynamicControlRuntimeOnlyTest, "AccessibleUMGInspector.Differential.DynamicControlRuntimeOnly", TEST_FLAGS)
bool FDiffDynamicControlRuntimeOnlyTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	const FWidgetComparisonRecord* DynamicRec = DifferentialComparatorTestHelpers::FindRecordByName(Report, TEXT("DynamicRuntimeLabel"));
	TestNotNull(TEXT("DynamicRuntimeLabel found"), DynamicRec);
	TestEqual(TEXT("Classification is RuntimeOnly"), (uint8)DynamicRec->Classification, (uint8)EWidgetComparisonClassification::RuntimeOnly);

	return true;
}

// ── Test 12: Internal Slate Composition Remains RuntimeOnly ────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffInternalSlateCompositionRuntimeOnlyTest, "AccessibleUMGInspector.Differential.InternalSlateCompositionRuntimeOnly", TEST_FLAGS)
bool FDiffInternalSlateCompositionRuntimeOnlyTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();

	// Add internal Slate composition node to runtime
	FRuntimeWidgetRecord SlateNode;
	SlateNode.NodeIndex = 6;
	SlateNode.WidgetName = TEXT("SBox_InternalBorder");
	SlateNode.WidgetClass = TEXT("Border");
	SlateNode.SlateWidgetType = TEXT("SBorder");
	SlateNode.ParentName = TEXT("MainVerticalBox");
	R.Widgets.Add(SlateNode);

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	const FWidgetComparisonRecord* SlateRec = DifferentialComparatorTestHelpers::FindRecordByName(Report, TEXT("SBox_InternalBorder"));
	TestNotNull(TEXT("Slate node record found"), SlateRec);
	TestEqual(TEXT("Internal Slate node is RuntimeOnly"), (uint8)SlateRec->Classification, (uint8)EWidgetComparisonClassification::RuntimeOnly);

	return true;
}

// ── Test 13: Accessible Name Used Only As Supporting Evidence ──────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffAccessibleNameSupportingEvidenceOnlyTest, "AccessibleUMGInspector.Differential.AccessibleNameSupportingEvidenceOnly", TEST_FLAGS)
bool FDiffAccessibleNameSupportingEvidenceOnlyTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S;
	S.bSuccess = true;
	FSerializedWidgetRecord S0;
	S0.NodeIndex = 0;
	S0.WidgetName = TEXT("Btn_A");
	S0.WidgetClass = TEXT("Button");
	S0.AccessibleName = TEXT("Shared Accessible Label");
	S.Widgets.Add(S0);

	FRuntimeWidgetReport R;
	R.bSuccess = true;
	FRuntimeWidgetRecord R0;
	R0.NodeIndex = 0;
	R0.WidgetName = TEXT("Btn_B"); // Different name!
	R0.WidgetClass = TEXT("Button");
	R0.AccessibleName = TEXT("Shared Accessible Label"); // Same accessible name!
	R.Widgets.Add(R0);

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	// Accessible name MUST NOT establish match by itself!
	TestEqual(TEXT("Zero matched despite sharing accessible name"), Report.MatchedCount, 0);
	TestEqual(TEXT("Btn_A is SerializedOnly"), Report.SerializedOnlyCount, 1);
	TestEqual(TEXT("Btn_B is RuntimeOnly"), Report.RuntimeOnlyCount, 1);

	return true;
}

// ── Test 14: Bounds Used Only For Discrepancy Reporting ────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffBoundsUsedOnlyForDiscrepancyTest, "AccessibleUMGInspector.Differential.BoundsUsedOnlyForDiscrepancy", TEST_FLAGS)
bool FDiffBoundsUsedOnlyForDiscrepancyTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S;
	S.bSuccess = true;
	FSerializedWidgetRecord S0;
	S0.NodeIndex = 0;
	S0.WidgetName = TEXT("WidgetOne");
	S0.WidgetClass = TEXT("Button");
	S.Widgets.Add(S0);

	FRuntimeWidgetReport R;
	R.bSuccess = true;
	FRuntimeWidgetRecord R0;
	R0.NodeIndex = 0;
	R0.WidgetName = TEXT("WidgetTwo"); // Different name!
	R0.WidgetClass = TEXT("Button");
	R0.ScreenBounds.X = 50.f;
	R0.ScreenBounds.Y = 50.f;
	R0.ScreenBounds.Width = 100.f;
	R0.ScreenBounds.Height = 100.f;
	R0.ScreenBounds.bIsValid = true;
	R.Widgets.Add(R0);

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	// Bounds MUST NOT establish identity!
	TestEqual(TEXT("Zero matched despite bounds"), Report.MatchedCount, 0);

	return true;
}

// ── Test 15: Runtime Geometry Unavailable ──────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffRuntimeGeometryUnavailableTest, "AccessibleUMGInspector.Differential.RuntimeGeometryUnavailable", TEST_FLAGS)
bool FDiffRuntimeGeometryUnavailableTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S;
	S.bSuccess = true;
	FSerializedWidgetRecord S0;
	S0.NodeIndex = 0;
	S0.WidgetName = TEXT("Button_Unrendered");
	S0.WidgetClass = TEXT("Button");
	S.Widgets.Add(S0);

	FRuntimeWidgetReport R;
	R.bSuccess = true;
	FRuntimeWidgetRecord R0;
	R0.NodeIndex = 0;
	R0.WidgetName = TEXT("Button_Unrendered");
	R0.WidgetClass = TEXT("Button");
	R0.bHasSlateWidget = true;
	R0.ScreenBounds.bIsValid = false; // Zero or invalid bounds!
	R.Widgets.Add(R0);

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	const FWidgetComparisonRecord* Rec = DifferentialComparatorTestHelpers::FindRecordByName(Report, TEXT("Button_Unrendered"));
	TestNotNull(TEXT("Record found"), Rec);

	bool bFoundGeomDisc = false;
	for (const FString& Disc : Rec->Discrepancies)
	{
		if (Disc.Contains(TEXT("unavailable geometry")))
		{
			bFoundGeomDisc = true;
			break;
		}
	}
	TestTrue(TEXT("Unavailable geometry discrepancy logged"), bFoundGeomDisc);

	return true;
}

// ── Test 16: Runtime Node Invalid or Destroyed ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffRuntimeNodeInvalidOrDestroyedTest, "AccessibleUMGInspector.Differential.RuntimeNodeInvalidOrDestroyed", TEST_FLAGS)
bool FDiffRuntimeNodeInvalidOrDestroyedTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S;
	S.bSuccess = true;

	FRuntimeWidgetReport R;
	R.bSuccess = true;
	FRuntimeWidgetRecord R0;
	R0.NodeIndex = 0;
	R0.WidgetName = TEXT("DetachedWidget");
	R0.WidgetClass = TEXT("Button");
	R0.ObjectPath = TEXT(""); // Empty detached path!
	R.Widgets.Add(R0);

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	bool bFoundStaleDisc = false;
	for (const FString& Disc : Report.Discrepancies)
	{
		if (Disc.Contains(TEXT("stale, invalid, or detached runtime record")))
		{
			bFoundStaleDisc = true;
			break;
		}
	}
	TestTrue(TEXT("Stale or detached discrepancy logged"), bFoundStaleDisc);

	return true;
}

// ── Test 17: Deterministic Results Under Reordered Input Arrays ────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffDeterministicReorderedInputsTest, "AccessibleUMGInspector.Differential.DeterministicReorderedInputs", TEST_FLAGS)
bool FDiffDeterministicReorderedInputsTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S1 = DifferentialComparatorTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R1 = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();

	FSerializedRuntimeComparisonReport Report1 = FDifferentialComparator::ReconcileReports(S1, R1);

	// Reorder serialized widgets array (reverse)
	FSerializedWidgetBlueprintReport S2 = S1;
	for (int32 i = 0; i < S2.Widgets.Num() / 2; ++i)
	{
		S2.Widgets.Swap(i, S2.Widgets.Num() - 1 - i);
	}

	FSerializedRuntimeComparisonReport Report2 = FDifferentialComparator::ReconcileReports(S2, R1);

	TestEqual(TEXT("Matched count identical"), Report1.MatchedCount, Report2.MatchedCount);
	TestEqual(TEXT("Ambiguous count identical"), Report1.AmbiguousCount, Report2.AmbiguousCount);
	TestEqual(TEXT("SerializedOnly count identical"), Report1.SerializedOnlyCount, Report2.SerializedOnlyCount);
	TestEqual(TEXT("RuntimeOnly count identical"), Report1.RuntimeOnlyCount, Report2.RuntimeOnlyCount);
	TestEqual(TEXT("Total comparison records identical"), Report1.ComparisonRecords.Num(), Report2.ComparisonRecords.Num());

	return true;
}

// ── Test 18: Bounded Output and Truncation Forwarding ──────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffBoundedOutputAndTruncationTest, "AccessibleUMGInspector.Differential.BoundedOutputAndTruncation", TEST_FLAGS)
bool FDiffBoundedOutputAndTruncationTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();
	R.bTruncated = true; // Truncated runtime report

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	TestTrue(TEXT("Truncation flag forwarded"), Report.bTruncated);

	return true;
}

// ── Test 19: Serialized Inspection Failure Forwarding ──────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffSerializedInspectionFailureForwardingTest, "AccessibleUMGInspector.Differential.SerializedInspectionFailureForwarding", TEST_FLAGS)
bool FDiffSerializedInspectionFailureForwardingTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S;
	S.bSuccess = false;
	S.ErrorMessage = TEXT("Asset not found: /Game/Missing_BP");

	FRuntimeWidgetReport R = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	TestFalse(TEXT("Comparison report should fail"), Report.bSuccess);
	TestTrue(TEXT("Contains original serialized error message"), Report.ErrorMessage.Contains(TEXT("Asset not found")));

	return true;
}

// ── Test 20: Runtime Inspection Failure Forwarding ─────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffRuntimeInspectionFailureForwardingTest, "AccessibleUMGInspector.Differential.RuntimeInspectionFailureForwarding", TEST_FLAGS)
bool FDiffRuntimeInspectionFailureForwardingTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();

	FRuntimeWidgetReport R;
	R.bSuccess = false;
	R.Availability = EInspectionAvailability::Unavailable;
	R.ErrorMessage = TEXT("No active PIE world found.");

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	TestFalse(TEXT("Comparison report should fail"), Report.bSuccess);
	TestEqual(TEXT("Availability forwarded as Unavailable"), (uint8)Report.Availability, (uint8)EInspectionAvailability::Unavailable);
	TestTrue(TEXT("Contains original runtime error message"), Report.ErrorMessage.Contains(TEXT("No active PIE world")));

	return true;
}

// ── Test 21: Ambiguous World Forwarding ────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffAmbiguousWorldForwardingTest, "AccessibleUMGInspector.Differential.AmbiguousWorldForwarding", TEST_FLAGS)
bool FDiffAmbiguousWorldForwardingTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();

	FRuntimeWidgetReport R;
	R.bSuccess = false;
	R.Availability = EInspectionAvailability::Ambiguous;
	R.ErrorMessage = TEXT("Multiple active PIE worlds found.");
	R.Candidates = { TEXT("PIEWorld_0"), TEXT("PIEWorld_1") };

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	TestFalse(TEXT("Comparison should not succeed for ambiguous world"), Report.bSuccess);
	TestEqual(TEXT("Availability forwarded as Ambiguous"), (uint8)Report.Availability, (uint8)EInspectionAvailability::Ambiguous);
	TestEqual(TEXT("World candidates forwarded"), Report.Candidates.Num(), 2);

	return true;
}

// ── Test 22: Ambiguous Runtime Widget Forwarding ───────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffAmbiguousRuntimeWidgetForwardingTest, "AccessibleUMGInspector.Differential.AmbiguousRuntimeWidgetForwarding", TEST_FLAGS)
bool FDiffAmbiguousRuntimeWidgetForwardingTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();

	FRuntimeWidgetReport R;
	R.bSuccess = false;
	R.Availability = EInspectionAvailability::Ambiguous;
	R.ErrorMessage = TEXT("Multiple top-level UUserWidgets found.");
	R.Candidates = { TEXT("WBP_Menu_A"), TEXT("WBP_Menu_B") };

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R);
	TestFalse(TEXT("Comparison should not succeed for ambiguous widget"), Report.bSuccess);
	TestEqual(TEXT("Availability forwarded as Ambiguous"), (uint8)Report.Availability, (uint8)EInspectionAvailability::Ambiguous);
	TestEqual(TEXT("Widget candidates forwarded"), Report.Candidates.Num(), 2);

	return true;
}

// ── Test 23: NVDA Text Formatting ──────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffNvdaFormattingTest, "AccessibleUMGInspector.Differential.NvdaFormatting", TEST_FLAGS)
bool FDiffNvdaFormattingTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R, TEXT("NvdaText"));
	TestTrue(TEXT("Report success"), Report.bSuccess);
	TestTrue(TEXT("NvdaText populated"), !Report.NvdaText.IsEmpty());

	// Verify required sections
	TestTrue(TEXT("Headline present"), Report.NvdaText.Contains(TEXT("Accessible UMG Inspector: Reconciliation")));
	TestTrue(TEXT("Selected identities present"), Report.NvdaText.Contains(TEXT("Asset: /Game/UI/WBP_MockMenu")));
	TestTrue(TEXT("Numbered entries present"), Report.NvdaText.Contains(TEXT("Reconciled Widget Comparison Entries:")));
	TestTrue(TEXT("Contains [MATCHED]"), Report.NvdaText.Contains(TEXT("[MATCHED]")));
	TestTrue(TEXT("Contains [SERIALIZEDONLY]"), Report.NvdaText.Contains(TEXT("[SERIALIZEDONLY]")));
	TestTrue(TEXT("Contains [RUNTIMEONLY]"), Report.NvdaText.Contains(TEXT("[RUNTIMEONLY]")));
	TestTrue(TEXT("Limitations section present"), Report.NvdaText.Contains(TEXT("Reconciliation Limitations:")));

	return true;
}

// ── Test 24: JSON Output Validity ──────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffJsonValidityTest, "AccessibleUMGInspector.Differential.JsonValidity", TEST_FLAGS)
bool FDiffJsonValidityTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R, TEXT("Json"));
	TestTrue(TEXT("Report success"), Report.bSuccess);
	TestTrue(TEXT("JsonData non-empty"), !Report.JsonData.IsEmpty());

	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Report.JsonData);
	bool bParsed = FJsonSerializer::Deserialize(Reader, JsonObj);
	TestTrue(TEXT("JSON deserialization succeeded"), bParsed);
	TestNotNull(TEXT("JSON root object valid"), JsonObj.Get());

	bool bSuccessField = false;
	TestTrue(TEXT("bSuccess field exists"), JsonObj->TryGetBoolField(TEXT("bSuccess"), bSuccessField));
	TestTrue(TEXT("bSuccess is true"), bSuccessField);

	int32 MatchedField = 0;
	TestTrue(TEXT("matchedCount exists"), JsonObj->TryGetNumberField(TEXT("matchedCount"), MatchedField));
	TestTrue(TEXT("matchedCount matches report"), MatchedField == Report.MatchedCount);

	const TArray<TSharedPtr<FJsonValue>>* RecordsArray = nullptr;
	TestTrue(TEXT("comparisonRecords array exists"), JsonObj->TryGetArrayField(TEXT("comparisonRecords"), RecordsArray));
	TestNotNull(TEXT("comparisonRecords valid"), RecordsArray);

	return true;
}

// ── Test 25: No Asset or Runtime Mutation ──────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffNoAssetOrRuntimeMutationTest, "AccessibleUMGInspector.Differential.NoAssetOrRuntimeMutation", TEST_FLAGS)
bool FDiffNoAssetOrRuntimeMutationTest::RunTest(const FString& Parameters)
{
	UPackage* TransientPkg = GetTransientPackage();
	bool bWasDirty = TransientPkg->IsDirty();

	FSerializedWidgetBlueprintReport S = DifferentialComparatorTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = DifferentialComparatorTestHelpers::CreateMockRuntimeReport();

	FSerializedRuntimeComparisonReport Report = FDifferentialComparator::ReconcileReports(S, R, TEXT("Both"));
	TestTrue(TEXT("Comparison succeeded"), Report.bSuccess);

	bool bIsDirtyAfter = TransientPkg->IsDirty();
	TestEqual(TEXT("Transient package dirty state invariant"), bWasDirty, bIsDirtyAfter);

	return true;
}

// ── Test 26: Capabilities and Milestone 1/2 Regression ─────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiffCapabilitiesAndRegressionTest, "AccessibleUMGInspector.Differential.CapabilitiesAndRegression", TEST_FLAGS)
bool FDiffCapabilitiesAndRegressionTest::RunTest(const FString& Parameters)
{
	FInspectorCapabilities Caps = UAccessibleUMGInspectorToolset::GetInspectorCapabilities();
	TestTrue(TEXT("Caps has InspectSerializedWidgetBlueprint"), Caps.SupportedOperations.Contains(TEXT("InspectSerializedWidgetBlueprint")));
	TestTrue(TEXT("Caps has InspectRuntimeUserWidget"), Caps.SupportedOperations.Contains(TEXT("InspectRuntimeUserWidget")));
	TestTrue(TEXT("Caps has CompareSerializedAndRuntime"), Caps.SupportedOperations.Contains(TEXT("CompareSerializedAndRuntime")));

	return true;
}
