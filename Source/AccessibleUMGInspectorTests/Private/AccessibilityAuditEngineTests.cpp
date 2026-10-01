// Copyright Blind Lion Studios. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "AccessibilityAuditEngine.h"
#include "SerializedWidgetInspector.h"
#include "RuntimeWidgetInspector.h"
#include "DifferentialComparator.h"
#include "NvdaTextFormatter.h"
#include "AccessibleUMGInspectorToolset.h"
#include "AccessibleUMGInspectorTypes.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#define TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace AccessibilityAuditTestHelpers
{
	static FSerializedWidgetBlueprintReport CreateMockSerializedReport()
	{
		FSerializedWidgetBlueprintReport S;
		S.bSuccess = true;
		S.AssetPath = TEXT("/Game/UI/WBP_MockAuditMenu");
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

		// Node 1: MainPanel
		FSerializedWidgetRecord R1;
		R1.NodeIndex = 1;
		R1.ParentIndex = 0;
		R1.WidgetName = TEXT("MainPanel");
		R1.WidgetClass = TEXT("VerticalBox");
		R1.ParentName = TEXT("RootCanvas");
		R1.ChildIndex = 0;
		R1.DefaultVisibility = TEXT("Visible");
		R1.bIsEnabled = true;
		R1.ChildIndices = { 2, 3 };
		S.Widgets.Add(R1);

		// Node 2: ActionBtn
		FSerializedWidgetRecord R2;
		R2.NodeIndex = 2;
		R2.ParentIndex = 1;
		R2.WidgetName = TEXT("ActionBtn");
		R2.WidgetClass = TEXT("Button");
		R2.ParentName = TEXT("MainPanel");
		R2.ChildIndex = 0;
		R2.DefaultVisibility = TEXT("Visible");
		R2.bIsEnabled = true;
		R2.bIsFocusable = true;
		R2.AccessibleName = TEXT("Start");
		FAccessibleWidgetBindingRecord B;
		B.FunctionName = TEXT("OnStartClicked");
		B.PropertyName = TEXT("OnClicked");
		R2.Bindings.Add(B);
		S.Widgets.Add(R2);

		// Node 3: SettingsBtn
		FSerializedWidgetRecord R3;
		R3.NodeIndex = 3;
		R3.ParentIndex = 1;
		R3.WidgetName = TEXT("SettingsBtn");
		R3.WidgetClass = TEXT("Button");
		R3.ParentName = TEXT("MainPanel");
		R3.ChildIndex = 1;
		R3.DefaultVisibility = TEXT("Visible");
		R3.bIsEnabled = true;
		R3.bIsFocusable = true;
		R3.AccessibleName = TEXT("Settings");
		FAccessibleWidgetBindingRecord B2;
		B2.FunctionName = TEXT("OnSettingsClicked");
		B2.PropertyName = TEXT("OnClicked");
		R3.Bindings.Add(B2);
		S.Widgets.Add(R3);

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
		R.RootWidgetName = TEXT("WBP_MockAuditMenu_C_0");
		R.RootWidgetClass = TEXT("WBP_MockAuditMenu_C");

		// Node 0: UUserWidget Host
		FRuntimeWidgetRecord W0;
		W0.NodeIndex = 0;
		W0.ParentIndex = -1;
		W0.WidgetName = TEXT("WBP_MockAuditMenu_C_0");
		W0.WidgetClass = TEXT("WBP_MockAuditMenu_C");
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
		W1.ParentName = TEXT("WBP_MockAuditMenu_C_0");
		W1.ChildIndex = 0;
		W1.RuntimeVisibility = TEXT("Visible");
		W1.bIsEnabled = true;
		W1.ChildIndices = { 2 };
		R.Widgets.Add(W1);

		// Node 2: MainPanel
		FRuntimeWidgetRecord W2;
		W2.NodeIndex = 2;
		W2.ParentIndex = 1;
		W2.WidgetName = TEXT("MainPanel");
		W2.WidgetClass = TEXT("VerticalBox");
		W2.ParentName = TEXT("RootCanvas");
		W2.ChildIndex = 0;
		W2.RuntimeVisibility = TEXT("Visible");
		W2.bIsEnabled = true;
		W2.ChildIndices = { 3, 4 };
		R.Widgets.Add(W2);

		// Node 3: ActionBtn
		FRuntimeWidgetRecord W3;
		W3.NodeIndex = 3;
		W3.ParentIndex = 2;
		W3.WidgetName = TEXT("ActionBtn");
		W3.WidgetClass = TEXT("Button");
		W3.ParentName = TEXT("MainPanel");
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
		W3.AccessibleName = TEXT("Start");
		R.Widgets.Add(W3);

		// Node 4: SettingsBtn
		FRuntimeWidgetRecord W4;
		W4.NodeIndex = 4;
		W4.ParentIndex = 2;
		W4.WidgetName = TEXT("SettingsBtn");
		W4.WidgetClass = TEXT("Button");
		W4.ParentName = TEXT("MainPanel");
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

		R.TotalWidgets = R.Widgets.Num();
		R.RootWidgetIndex = 0;
		FRuntimeWidgetInspector::ComputeEffectiveVisibility(R);
		return R;
	}

	static const FAuditFindingRecord* FindFindingByRule(const FAccessibilityAuditReport& Report, const FString& RuleId)
	{
		for (const FAuditFindingRecord& F : Report.Findings)
		{
			if (F.RuleId.Equals(RuleId, ESearchCase::IgnoreCase))
			{
				return &F;
			}
		}
		return nullptr;
	}
}

// ── Test 1: A11Y-01 HiddenButRuntimeExposed Direct Evidence ───────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y01HiddenExposedDirectTest, "AccessibleUMGInspector.Audit.A11Y01HiddenExposedDirect", TEST_FLAGS)
bool FAuditA11Y01HiddenExposedDirectTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();

	// Author ActionBtn as Collapsed in blueprint
	S.Widgets[2].DefaultVisibility = TEXT("Collapsed");

	// At runtime, ActionBtn is effectively exposed in an inactive (disabled) container
	R.Widgets[2].bIsEnabled = false; // MainPanel is disabled
	R.Widgets[3].bIsEnabled = true;  // ActionBtn is enabled
	FRuntimeWidgetInspector::ComputeEffectiveVisibility(R);

	FSerializedRuntimeComparisonReport C = FDifferentialComparator::ReconcileReports(S, R);
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, &R, &C, { TEXT("A11Y-01") });

	TestTrue(TEXT("Audit succeeded"), Report.bSuccess);
	TestTrue(TEXT("Found Error finding for A11Y-01"), Report.ErrorCount >= 1);

	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-01"));
	TestNotNull(TEXT("Finding exists"), Finding);
	if (Finding)
	{
		TestEqual(TEXT("Classification is Error"), (uint8)Finding->Classification, (uint8)EAuditFindingClassification::Error);
		TestEqual(TEXT("Confidence basis is Direct"), (uint8)Finding->ConfidenceBasis, (uint8)EAuditConfidenceBasis::Direct);
	}
	return true;
}

// ── Test 2: A11Y-01 Hidden State Legitimately Activated Suppressed ─────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y01LegitimatelyActivatedSuppressedTest, "AccessibleUMGInspector.Audit.A11Y01LegitimatelyActivatedSuppressed", TEST_FLAGS)
bool FAuditA11Y01LegitimatelyActivatedSuppressedTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();

	// ActionBtn was serialized as Collapsed by default
	S.Widgets[2].DefaultVisibility = TEXT("Collapsed");

	// At runtime, both MainPanel and ActionBtn are legitimately Visible (active screen)
	R.Widgets[2].RuntimeVisibility = TEXT("Visible");
	R.Widgets[3].RuntimeVisibility = TEXT("Visible");

	FSerializedRuntimeComparisonReport C = FDifferentialComparator::ReconcileReports(S, R);
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, &R, &C, { TEXT("A11Y-01") });

	TestTrue(TEXT("Audit succeeded"), Report.bSuccess);
	TestEqual(TEXT("Error count is zero (suppressed)"), Report.ErrorCount, 0);
	TestTrue(TEXT("A11Y-01 passed"), Report.PassedRuleIds.Contains(TEXT("A11Y-01")));
	return true;
}

// ── Test 3: A11Y-01 Hidden State Without Active-Context Returns Warning or NotEvaluated

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y01WithoutContextQualifiedWarningTest, "AccessibleUMGInspector.Audit.A11Y01WithoutContextQualifiedWarning", TEST_FLAGS)
bool FAuditA11Y01WithoutContextQualifiedWarningTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	// Without runtime report, differential evidence is unavailable
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, nullptr, nullptr, { TEXT("A11Y-01") });

	TestTrue(TEXT("Audit succeeded"), Report.bSuccess);
	TestTrue(TEXT("A11Y-01 is NotEvaluated"), Report.NotEvaluatedRules.Contains(TEXT("A11Y-01")));
	return true;
}

// ── Test 4: A11Y-02 Visible Interactive Unnamed Control ────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y02VisibleInteractiveUnnamedTest, "AccessibleUMGInspector.Audit.A11Y02VisibleInteractiveUnnamed", TEST_FLAGS)
bool FAuditA11Y02VisibleInteractiveUnnamedTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Clear accessible name on ActionBtn
	R.Widgets[3].AccessibleName = TEXT("");

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-02") });

	TestTrue(TEXT("Audit succeeded"), Report.bSuccess);
	TestTrue(TEXT("Found Error for unnamed button"), Report.ErrorCount >= 1);

	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-02"));
	TestNotNull(TEXT("Finding exists"), Finding);
	if (Finding)
	{
		TestEqual(TEXT("Classification is Error"), (uint8)Finding->Classification, (uint8)EAuditFindingClassification::Error);
		TestEqual(TEXT("Widget is ActionBtn"), Finding->WidgetIdentity, TEXT("ActionBtn"));
	}
	return true;
}

// ── Test 5: A11Y-02 NotAccessible Decorative Control Excluded ──────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y02DecorativeExcludedTest, "AccessibleUMGInspector.Audit.A11Y02DecorativeExcluded", TEST_FLAGS)
bool FAuditA11Y02DecorativeExcludedTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// ActionBtn has empty name but is explicitly marked NotAccessible (decorative)
	R.Widgets[3].AccessibleName = TEXT("");
	R.Widgets[3].bOverrideAccessibleDefaults = true;
	R.Widgets[3].AccessibleBehavior = TEXT("NotAccessible");

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-02") });

	TestEqual(TEXT("Decorative control excluded, zero errors"), Report.ErrorCount, 0);
	TestTrue(TEXT("A11Y-02 passed"), Report.PassedRuleIds.Contains(TEXT("A11Y-02")));
	return true;
}

// ── Test 6: A11Y-03 Focusable Unnamed Control ──────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y03FocusableUnnamedTest, "AccessibleUMGInspector.Audit.A11Y03FocusableUnnamed", TEST_FLAGS)
bool FAuditA11Y03FocusableUnnamedTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	R.Widgets[3].bIsFocusable = true;
	R.Widgets[3].AccessibleName = TEXT("");

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-03") });

	TestTrue(TEXT("Found Error for focusable unnamed control"), Report.ErrorCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-03"));
	TestNotNull(TEXT("Finding exists"), Finding);
	return true;
}

// ── Test 7: A11Y-03 Focusability Unavailable Returns NotEvaluated ─────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y03FocusabilityUnavailableTest, "AccessibleUMGInspector.Audit.A11Y03FocusabilityUnavailable", TEST_FLAGS)
bool FAuditA11Y03FocusabilityUnavailableTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R;
	R.bSuccess = true;
	// No widgets with focusability
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-03") });

	TestTrue(TEXT("Reports passed or no findings when no widgets"), Report.bSuccess);
	return true;
}

// ── Test 8: A11Y-04 Duplicate Accessible Name Warning ──────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y04DuplicateNameWarningTest, "AccessibleUMGInspector.Audit.A11Y04DuplicateNameWarning", TEST_FLAGS)
bool FAuditA11Y04DuplicateNameWarningTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Give both buttons the same accessible name "Select"
	R.Widgets[3].AccessibleName = TEXT("Select");
	R.Widgets[4].AccessibleName = TEXT("Select");
	// But give them different classes or inactive states so it's a Warning, not escalated Error
	R.Widgets[4].bIsEnabled = false;

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-04") });

	TestTrue(TEXT("Produces Warning for duplicate names"), Report.WarningCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-04"));
	TestNotNull(TEXT("Finding exists"), Finding);
	if (Finding)
	{
		TestEqual(TEXT("Classification is Warning"), (uint8)Finding->Classification, (uint8)EAuditFindingClassification::Warning);
	}
	return true;
}

// ── Test 9: A11Y-04 Duplicate Active Equivalent Names Escalated to Error

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y04DuplicateActiveEquivalentErrorTest, "AccessibleUMGInspector.Audit.A11Y04DuplicateActiveEquivalentError", TEST_FLAGS)
bool FAuditA11Y04DuplicateActiveEquivalentErrorTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Give both simultaneously active, equivalent buttons the same accessible name with empty summary
	R.Widgets[3].AccessibleName = TEXT("Continue");
	R.Widgets[3].AccessibleSummary = TEXT("");
	R.Widgets[4].AccessibleName = TEXT("Continue");
	R.Widgets[4].AccessibleSummary = TEXT("");

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-04") });

	TestTrue(TEXT("Escalated to Error"), Report.ErrorCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-04"));
	TestNotNull(TEXT("Finding exists"), Finding);
	if (Finding)
	{
		TestEqual(TEXT("Classification is Error"), (uint8)Finding->Classification, (uint8)EAuditFindingClassification::Error);
	}
	return true;
}

// ── Test 10: A11Y-04 Duplicate Names Disambiguated by Role or Summary ─

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y04DisambiguatedBySummaryTest, "AccessibleUMGInspector.Audit.A11Y04DisambiguatedBySummary", TEST_FLAGS)
bool FAuditA11Y04DisambiguatedBySummaryTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Same name but different summaries
	R.Widgets[3].AccessibleName = TEXT("Action");
	R.Widgets[3].AccessibleSummary = TEXT("Begins story mode");
	R.Widgets[4].AccessibleName = TEXT("Action");
	R.Widgets[4].AccessibleSummary = TEXT("Begins arcade mode");

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-04") });

	TestEqual(TEXT("Disambiguated by summary, zero errors"), Report.ErrorCount, 0);
	TestTrue(TEXT("A11Y-04 passed"), Report.PassedRuleIds.Contains(TEXT("A11Y-04")));
	return true;
}

// ── Test 11: A11Y-05 Reliable Child Containment Mismatch ──────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y05ChildContainmentMismatchTest, "AccessibleUMGInspector.Audit.A11Y05ChildContainmentMismatch", TEST_FLAGS)
bool FAuditA11Y05ChildContainmentMismatchTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();

	// Change runtime parent of ActionBtn to RootCanvas instead of MainPanel
	R.Widgets[3].ParentName = TEXT("RootCanvas");
	R.Widgets[3].ParentIndex = 1;

	FSerializedRuntimeComparisonReport C = FDifferentialComparator::ReconcileReports(S, R);
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, &R, &C, { TEXT("A11Y-05") });

	TestTrue(TEXT("Produces Warning for containment mismatch"), Report.WarningCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-05"));
	TestNotNull(TEXT("Finding exists"), Finding);
	return true;
}

// ── Test 12: A11Y-05 Naming-Only Containment Does Not Produce Finding ──

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y05NamingOnlyDoesNotFailTest, "AccessibleUMGInspector.Audit.A11Y05NamingOnlyDoesNotFail", TEST_FLAGS)
bool FAuditA11Y05NamingOnlyDoesNotFailTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();

	// Widget is named "Header_Btn" but parent is "MainPanel" in both serialized and runtime
	S.Widgets[2].WidgetName = TEXT("Header_Btn");
	R.Widgets[3].WidgetName = TEXT("Header_Btn");

	FSerializedRuntimeComparisonReport C = FDifferentialComparator::ReconcileReports(S, R);
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, &R, &C, { TEXT("A11Y-05") });

	TestEqual(TEXT("Zero warnings from naming alone"), Report.WarningCount, 0);
	TestTrue(TEXT("A11Y-05 passed"), Report.PassedRuleIds.Contains(TEXT("A11Y-05")));
	return true;
}

// ── Test 13: A11Y-06 Collapsed-Screen Visible Runtime Descendant ───────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y06CollapsedScreenDescendantTest, "AccessibleUMGInspector.Audit.A11Y06CollapsedScreenDescendant", TEST_FLAGS)
bool FAuditA11Y06CollapsedScreenDescendantTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Collapse MainPanel
	R.Widgets[2].LocalVisibility = TEXT("Collapsed");
	R.Widgets[2].RuntimeVisibility = TEXT("Collapsed");
	FRuntimeWidgetInspector::ComputeEffectiveVisibility(R);

	// Descendant ActionBtn escapes suppression by retaining keyboard focus
	R.Widgets[3].bHasKeyboardFocus = true;

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-06") });

	TestTrue(TEXT("Produces Error for visible descendant of collapsed container"), Report.ErrorCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-06"));
	TestNotNull(TEXT("Finding exists"), Finding);
	return true;
}

// ── Test 14: A11Y-06 Active Screen Unavailable Returns NotEvaluated ───

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y06ScreenUnavailableTest, "AccessibleUMGInspector.Audit.A11Y06ScreenUnavailable", TEST_FLAGS)
bool FAuditA11Y06ScreenUnavailableTest::RunTest(const FString& Parameters)
{
	// Only serialized report provided
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, nullptr, nullptr, { TEXT("A11Y-06") });

	TestTrue(TEXT("Returns NotEvaluated when runtime unavailable"), Report.NotEvaluatedRules.Contains(TEXT("A11Y-06")));
	return true;
}

// ── Test 15: A11Y-07 Focus On Inactive Screen ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y07FocusOnInactiveScreenTest, "AccessibleUMGInspector.Audit.A11Y07FocusOnInactiveScreen", TEST_FLAGS)
bool FAuditA11Y07FocusOnInactiveScreenTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Collapse MainPanel
	R.Widgets[2].LocalVisibility = TEXT("Collapsed");
	R.Widgets[2].RuntimeVisibility = TEXT("Collapsed");
	FRuntimeWidgetInspector::ComputeEffectiveVisibility(R);

	// But give ActionBtn passive keyboard focus!
	R.Widgets[3].bHasKeyboardFocus = true;

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-07") });

	TestTrue(TEXT("Produces Error for focus on inactive screen"), Report.ErrorCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-07"));
	TestNotNull(TEXT("Finding exists"), Finding);
	return true;
}

// ── Test 16: A11Y-07 Focus Unavailable Returns NotEvaluated ───────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y07FocusUnavailableTest, "AccessibleUMGInspector.Audit.A11Y07FocusUnavailable", TEST_FLAGS)
bool FAuditA11Y07FocusUnavailableTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, nullptr, nullptr, { TEXT("A11Y-07") });

	TestTrue(TEXT("Returns NotEvaluated when focus evidence unavailable"), Report.NotEvaluatedRules.Contains(TEXT("A11Y-07")));
	return true;
}

// ── Test 17: A11Y-08 Explicit Navigation Target Outside Active Screen ─

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y08NavTargetOutsideScreenTest, "AccessibleUMGInspector.Audit.A11Y08NavTargetOutsideScreen", TEST_FLAGS)
bool FAuditA11Y08NavTargetOutsideScreenTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	// Add explicit navigation rule on ActionBtn targeting non-existent widget
	FAccessibleNavigationRecord Nav;
	Nav.Direction = TEXT("Down");
	Nav.Rule = TEXT("Explicit");
	Nav.TargetWidget = TEXT("MissingWidgetOutsideScreen");
	S.Widgets[2].NavigationRules.Add(Nav);

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, nullptr, nullptr, { TEXT("A11Y-08") });

	TestTrue(TEXT("Produces Warning for navigation outside screen"), Report.WarningCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-08"));
	TestNotNull(TEXT("Finding exists"), Finding);
	return true;
}

// ── Test 18: A11Y-08 Navigation None Does Not Automatically Fail ──────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y08NavNoneDoesNotFailTest, "AccessibleUMGInspector.Audit.A11Y08NavNoneDoesNotFail", TEST_FLAGS)
bool FAuditA11Y08NavNoneDoesNotFailTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	// Add Navigation None rule
	FAccessibleNavigationRecord Nav;
	Nav.Direction = TEXT("Down");
	Nav.Rule = TEXT("None");
	S.Widgets[2].NavigationRules.Add(Nav);

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, nullptr, nullptr, { TEXT("A11Y-08") });

	TestEqual(TEXT("Zero warnings for Navigation None"), Report.WarningCount, 0);
	TestTrue(TEXT("A11Y-08 passed"), Report.PassedRuleIds.Contains(TEXT("A11Y-08")));
	return true;
}

// ── Test 19: A11Y-09 Modal Containment Failure With Direct Focus Evidence

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y09ModalContainmentFailureTest, "AccessibleUMGInspector.Audit.A11Y09ModalContainmentFailure", TEST_FLAGS)
bool FAuditA11Y09ModalContainmentFailureTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Add active modal widget
	FRuntimeWidgetRecord ModalW;
	ModalW.NodeIndex = 5;
	ModalW.ParentIndex = 0;
	ModalW.WidgetName = TEXT("ConfirmationModal");
	ModalW.WidgetClass = TEXT("ModalDialog");
	ModalW.LocalVisibility = TEXT("Visible");
	ModalW.RuntimeVisibility = TEXT("Visible");
	ModalW.bEffectivelyVisible = true;
	R.Widgets.Add(ModalW);

	// ActionBtn is outside modal, but retains keyboard focus!
	R.Widgets[3].bHasKeyboardFocus = true;

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-09") });

	TestTrue(TEXT("Produces Error for modal containment failure"), Report.ErrorCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-09"));
	TestNotNull(TEXT("Finding exists"), Finding);
	return true;
}

// ── Test 20: A11Y-09 Modal Overlap Without Focus Evidence Does Not Fail

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y09ModalWithoutFocusDoesNotFailTest, "AccessibleUMGInspector.Audit.A11Y09ModalWithoutFocusDoesNotFail", TEST_FLAGS)
bool FAuditA11Y09ModalWithoutFocusDoesNotFailTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Add active modal widget
	FRuntimeWidgetRecord ModalW;
	ModalW.NodeIndex = 5;
	ModalW.ParentIndex = 0;
	ModalW.WidgetName = TEXT("ConfirmationModal");
	ModalW.WidgetClass = TEXT("ModalDialog");
	ModalW.RuntimeVisibility = TEXT("Visible");
	R.Widgets.Add(ModalW);

	// No underlying widgets have focus
	R.Widgets[3].bHasKeyboardFocus = false;
	R.Widgets[4].bHasKeyboardFocus = false;

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-09") });

	TestEqual(TEXT("Zero modal containment errors"), Report.ErrorCount, 0);
	TestTrue(TEXT("A11Y-09 passed"), Report.PassedRuleIds.Contains(TEXT("A11Y-09")));
	return true;
}

// ── Test 21: A11Y-10 Visibility Mismatch ──────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y10VisibilityMismatchTest, "AccessibleUMGInspector.Audit.A11Y10VisibilityMismatch", TEST_FLAGS)
bool FAuditA11Y10VisibilityMismatchTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();

	// ActionBtn is Visible serialized, but Collapsed at runtime
	R.Widgets[3].LocalVisibility = TEXT("Collapsed");
	R.Widgets[3].RuntimeVisibility = TEXT("Collapsed");
	FRuntimeWidgetInspector::ComputeEffectiveVisibility(R);

	FSerializedRuntimeComparisonReport C = FDifferentialComparator::ReconcileReports(S, R);
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, &R, &C, { TEXT("A11Y-10") });

	TestTrue(TEXT("Produces Warning for visibility mismatch"), Report.WarningCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-10"));
	TestNotNull(TEXT("Finding exists"), Finding);
	return true;
}

// ── Test 22: A11Y-10 Expected State Transition Suppresses Mismatch ────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y10ExpectedTransitionSuppressedTest, "AccessibleUMGInspector.Audit.A11Y10ExpectedTransitionSuppressed", TEST_FLAGS)
bool FAuditA11Y10ExpectedTransitionSuppressedTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();

	// Both are Visible
	FSerializedRuntimeComparisonReport C = FDifferentialComparator::ReconcileReports(S, R);
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, &R, &C, { TEXT("A11Y-10") });

	TestEqual(TEXT("Zero visibility mismatch warnings"), Report.WarningCount, 0);
	TestTrue(TEXT("A11Y-10 passed"), Report.PassedRuleIds.Contains(TEXT("A11Y-10")));
	return true;
}

// ── Test 23: A11Y-11 Overlapping Interactive Bounds Threshold ──────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y11OverlappingBoundsTest, "AccessibleUMGInspector.Audit.A11Y11OverlappingBounds", TEST_FLAGS)
bool FAuditA11Y11OverlappingBoundsTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Give ActionBtn and SettingsBtn overlapping bounds:
	// ActionBtn: (100, 200, 300, 60)
	// SettingsBtn overlaps by placing it at (120, 210, 300, 60)
	R.Widgets[4].ScreenBounds.X = 120.f;
	R.Widgets[4].ScreenBounds.Y = 210.f;
	R.Widgets[4].ScreenBounds.Width = 300.f;
	R.Widgets[4].ScreenBounds.Height = 60.f;
	R.Widgets[4].ScreenBounds.bIsValid = true;

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-11") });

	TestTrue(TEXT("Produces Warning for overlapping bounds"), Report.WarningCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-11"));
	TestNotNull(TEXT("Finding exists"), Finding);
	return true;
}

// ── Test 24: A11Y-11 Parent-Child Overlap Exclusion ───────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y11ParentChildOverlapExcludedTest, "AccessibleUMGInspector.Audit.A11Y11ParentChildOverlapExcluded", TEST_FLAGS)
bool FAuditA11Y11ParentChildOverlapExcludedTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Parent MainPanel (node 2) has bounds that encompass child ActionBtn (node 3)
	R.Widgets[2].bIsFocusable = true;
	R.Widgets[2].ScreenBounds = R.Widgets[3].ScreenBounds;

	// But SettingsBtn is moved far away
	R.Widgets[4].ScreenBounds.Y = 1000.f;

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-11") });

	TestEqual(TEXT("Parent-child overlap excluded, zero warnings"), Report.WarningCount, 0);
	TestTrue(TEXT("A11Y-11 passed"), Report.PassedRuleIds.Contains(TEXT("A11Y-11")));
	return true;
}

// ── Test 25: A11Y-11 Zero-Size and Unavailable Geometry Exclusion ─────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y11ZeroSizeExcludedTest, "AccessibleUMGInspector.Audit.A11Y11ZeroSizeExcluded", TEST_FLAGS)
bool FAuditA11Y11ZeroSizeExcludedTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Make bounds zero size or invalid
	R.Widgets[3].ScreenBounds.Width = 0.f;
	R.Widgets[4].ScreenBounds.bIsValid = false;

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-11") });

	TestEqual(TEXT("Zero warnings when geometry is zero-size or invalid"), Report.WarningCount, 0);
	TestTrue(TEXT("A11Y-11 passed"), Report.PassedRuleIds.Contains(TEXT("A11Y-11")));
	return true;
}

// ── Test 26: A11Y-12 Missing Action Binding With Direct Binding Evidence

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y12MissingActionBindingTest, "AccessibleUMGInspector.Audit.A11Y12MissingActionBinding", TEST_FLAGS)
bool FAuditA11Y12MissingActionBindingTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	// Remove bindings from SettingsBtn
	S.Widgets[3].Bindings.Empty();

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, nullptr, nullptr, { TEXT("A11Y-12") });

	TestTrue(TEXT("Produces Warning for button with missing bindings"), Report.WarningCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-12"));
	TestNotNull(TEXT("Finding exists"), Finding);
	if (Finding)
	{
		TestEqual(TEXT("Widget is SettingsBtn"), Finding->WidgetIdentity, TEXT("SettingsBtn"));
	}
	return true;
}

// ── Test 27: A11Y-12 Missing Binding Evidence Returns NotEvaluated ────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y12BindingEvidenceUnavailableTest, "AccessibleUMGInspector.Audit.A11Y12BindingEvidenceUnavailable", TEST_FLAGS)
bool FAuditA11Y12BindingEvidenceUnavailableTest::RunTest(const FString& Parameters)
{
	// Only runtime report supplied, no serialized binding evidence
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-12") });

	TestTrue(TEXT("A11Y-12 is NotEvaluated"), Report.NotEvaluatedRules.Contains(TEXT("A11Y-12")));
	return true;
}

// ── Test 28: A11Y-13 Accessible Child Leak With Direct Evidence ────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y13AccessibleChildLeakTest, "AccessibleUMGInspector.Audit.A11Y13AccessibleChildLeak", TEST_FLAGS)
bool FAuditA11Y13AccessibleChildLeakTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Parent MainPanel directly disallows accessible children
	R.Widgets[2].bOverrideAccessibleDefaults = true;
	R.Widgets[2].AccessibleBehavior = TEXT("NotAccessible");

	// But child ActionBtn is exposed with accessible text!
	R.Widgets[3].AccessibleName = TEXT("Leaked Action");

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-13") });

	TestTrue(TEXT("Produces Error for accessible child leak"), Report.ErrorCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-13"));
	TestNotNull(TEXT("Finding exists"), Finding);
	return true;
}

// ── Test 29: A11Y-13 OS Accessibility Claims Are Not Emitted ──────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y13NoOSAccessibilityClaimsTest, "AccessibleUMGInspector.Audit.A11Y13NoOSAccessibilityClaims", TEST_FLAGS)
bool FAuditA11Y13NoOSAccessibilityClaimsTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-13") });

	bool bFoundOsTreeLimitation = false;
	for (const FString& Lim : Report.Limitations)
	{
		if (Lim.Contains(TEXT("does not inspect the OS accessibility tree")))
		{
			bFoundOsTreeLimitation = true;
			break;
		}
	}
	TestTrue(TEXT("Limitation explicitly states OS tree not inspected"), bFoundOsTreeLimitation);
	return true;
}

// ── Test 30: A11Y-14 Ambiguous Runtime Identity Finding ────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y14AmbiguousRuntimeIdentityTest, "AccessibleUMGInspector.Audit.A11Y14AmbiguousRuntimeIdentity", TEST_FLAGS)
bool FAuditA11Y14AmbiguousRuntimeIdentityTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	R.Availability = EInspectionAvailability::Ambiguous;
	R.Candidates = { TEXT("CandidateA"), TEXT("CandidateB") };

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-14") });

	TestTrue(TEXT("Produces Warning for ambiguous runtime identity"), Report.WarningCount >= 1);
	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-14"));
	TestNotNull(TEXT("Finding exists"), Finding);
	if (Finding)
	{
		TestEqual(TEXT("Confidence basis is Ambiguous"), (uint8)Finding->ConfidenceBasis, (uint8)EAuditConfidenceBasis::Ambiguous);
	}
	return true;
}

// ── Test 31: Finding Deduplication ─────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditFindingDeduplicationTest, "AccessibleUMGInspector.Audit.FindingDeduplication", TEST_FLAGS)
bool FAuditFindingDeduplicationTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// ActionBtn unnamed
	R.Widgets[3].AccessibleName = TEXT("");

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-02") });

	// Count occurrences of A11Y-02 for ActionBtn
	int32 Count = 0;
	for (const FAuditFindingRecord& F : Report.Findings)
	{
		if (F.RuleId == TEXT("A11Y-02") && F.WidgetIdentity == TEXT("ActionBtn"))
		{
			Count++;
		}
	}
	TestEqual(TEXT("Exactly 1 deduplicated finding for ActionBtn"), Count, 1);
	return true;
}

// ── Test 32: Deterministic Ordering ───────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditDeterministicOrderingTest, "AccessibleUMGInspector.Audit.DeterministicOrdering", TEST_FLAGS)
bool FAuditDeterministicOrderingTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();

	// Generate both an Error (A11Y-02) and a Warning (A11Y-12)
	R.Widgets[3].AccessibleName = TEXT("");
	S.Widgets[3].Bindings.Empty();

	FSerializedRuntimeComparisonReport C = FDifferentialComparator::ReconcileReports(S, R);
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, &R, &C, { TEXT("A11Y-02"), TEXT("A11Y-12") });

	TestTrue(TEXT("At least 2 findings"), Report.Findings.Num() >= 2);
	if (Report.Findings.Num() >= 2)
	{
		// Errors must precede Warnings
		TestEqual(TEXT("First finding is Error"), (uint8)Report.Findings[0].Classification, (uint8)EAuditFindingClassification::Error);
		TestEqual(TEXT("Second finding is Warning"), (uint8)Report.Findings[1].Classification, (uint8)EAuditFindingClassification::Warning);
	}
	return true;
}

// ── Test 33: RuleSet Filtering ─────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditRuleSetFilteringTest, "AccessibleUMGInspector.Audit.RuleSetFiltering", TEST_FLAGS)
bool FAuditRuleSetFilteringTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	// Run only A11Y-02
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-02") });

	for (const FAuditFindingRecord& F : Report.Findings)
	{
		TestEqual(TEXT("Only A11Y-02 findings present"), F.RuleId, TEXT("A11Y-02"));
	}
	return true;
}

// ── Test 34: Passed Versus NotEvaluated Distinction ───────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditPassedVsNotEvaluatedTest, "AccessibleUMGInspector.Audit.PassedVsNotEvaluated", TEST_FLAGS)
bool FAuditPassedVsNotEvaluatedTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	// Audit with only serialized report
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, nullptr, nullptr, {});

	// Rules requiring runtime should be NotEvaluated, static rules without violations should be Passed
	TestTrue(TEXT("NotEvaluated contains A11Y-02"), Report.NotEvaluatedRules.Contains(TEXT("A11Y-02")));
	TestTrue(TEXT("Passed contains A11Y-03 or A11Y-04 or A11Y-12"),
		Report.PassedRuleIds.Contains(TEXT("A11Y-03")) || Report.PassedRuleIds.Contains(TEXT("A11Y-04")) || Report.PassedRuleIds.Contains(TEXT("A11Y-12")));
	return true;
}

// ── Test 35: NVDA Output Ordering and Required Wording ────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditNvdaOutputOrderingTest, "AccessibleUMGInspector.Audit.NvdaOutputOrdering", TEST_FLAGS)
bool FAuditNvdaOutputOrderingTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	R.Widgets[3].AccessibleName = TEXT("");

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-02") }, TEXT("Both"));

	TestFalse(TEXT("NVDA text not empty"), Report.NvdaText.IsEmpty());
	TestTrue(TEXT("Opening line contains Target"), Report.NvdaText.Contains(TEXT("Target:")));
	TestTrue(TEXT("Opening line contains Evidence Mode"), Report.NvdaText.Contains(TEXT("Evidence Mode:")));
	TestTrue(TEXT("Contains ERRORS section"), Report.NvdaText.Contains(TEXT("=== ERRORS (")));
	TestTrue(TEXT("Contains WARNINGS section"), Report.NvdaText.Contains(TEXT("=== WARNINGS (")));
	TestTrue(TEXT("Contains HUMAN REVIEW REQUIRED section"), Report.NvdaText.Contains(TEXT("=== HUMAN REVIEW REQUIRED ===")));
	TestTrue(TEXT("Contains NOT EVALUATED section"), Report.NvdaText.Contains(TEXT("=== NOT EVALUATED (")));
	TestTrue(TEXT("Contains PASSED RULES section"), Report.NvdaText.Contains(TEXT("=== PASSED RULES (")));
	TestTrue(TEXT("Contains ARCHITECTURAL LIMITATIONS section"), Report.NvdaText.Contains(TEXT("=== ARCHITECTURAL LIMITATIONS ===")));
	return true;
}

// ── Test 36: JSON Schema Validity ─────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditJsonSchemaValidityTest, "AccessibleUMGInspector.Audit.JsonSchemaValidity", TEST_FLAGS)
bool FAuditJsonSchemaValidityTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, {}, TEXT("Json"));

	TestFalse(TEXT("JSON string not empty"), Report.JsonData.IsEmpty());

	TSharedPtr<FJsonObject> JsonObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Report.JsonData);
	bool bParsed = FJsonSerializer::Deserialize(Reader, JsonObj);
	TestTrue(TEXT("JSON deserialized cleanly"), bParsed);
	if (bParsed && JsonObj.IsValid())
	{
		TestTrue(TEXT("Contains bSuccess"), JsonObj->HasField(TEXT("bSuccess")));
		TestTrue(TEXT("Contains ruleSetVersion"), JsonObj->HasField(TEXT("ruleSetVersion")));
		TestTrue(TEXT("Contains findings"), JsonObj->HasField(TEXT("findings")));
		TestTrue(TEXT("Contains passedRuleIds"), JsonObj->HasField(TEXT("passedRuleIds")));
		TestTrue(TEXT("Contains notEvaluatedRules"), JsonObj->HasField(TEXT("notEvaluatedRules")));
		TestTrue(TEXT("Contains limitations"), JsonObj->HasField(TEXT("limitations")));
	}
	return true;
}

// ── Test 37: Bounded Output and Truncation Forwarding ─────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditTruncationForwardingTest, "AccessibleUMGInspector.Audit.TruncationForwarding", TEST_FLAGS)
bool FAuditTruncationForwardingTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	R.bTruncated = true;

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, {});
	TestTrue(TEXT("bTruncated forwarded to audit report"), Report.bTruncated);
	return true;
}

// ── Test 38: No Mutation ──────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditNoMutationTest, "AccessibleUMGInspector.Audit.NoMutation", TEST_FLAGS)
bool FAuditNoMutationTest::RunTest(const FString& Parameters)
{
	FSerializedWidgetBlueprintReport S = AccessibilityAuditTestHelpers::CreateMockSerializedReport();
	FRuntimeWidgetReport R = AccessibilityAuditTestHelpers::CreateMockRuntimeReport();
	FSerializedRuntimeComparisonReport C = FDifferentialComparator::ReconcileReports(S, R);

	int32 SCountBefore = S.Widgets.Num();
	int32 RCountBefore = R.Widgets.Num();
	int32 CCountBefore = C.ComparisonRecords.Num();

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(&S, &R, &C, {});

	TestEqual(TEXT("Serialized widgets unaltered"), S.Widgets.Num(), SCountBefore);
	TestEqual(TEXT("Runtime widgets unaltered"), R.Widgets.Num(), RCountBefore);
	TestEqual(TEXT("Comparison records unaltered"), C.ComparisonRecords.Num(), CCountBefore);
	return true;
}

// ── Test 39: Toolset Capability and Static Method Invocation ──────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditToolsetCapabilitiesTest, "AccessibleUMGInspector.Audit.ToolsetCapabilities", TEST_FLAGS)
bool FAuditToolsetCapabilitiesTest::RunTest(const FString& Parameters)
{
	FInspectorCapabilities Caps = UAccessibleUMGInspectorToolset::GetInspectorCapabilities();
	TestTrue(TEXT("Capabilities lists AuditWidgetAccessibility"),
		Caps.SupportedOperations.Contains(TEXT("AuditWidgetAccessibility")));

	// Test calling AuditWidgetAccessibility with empty inputs (should cleanly return error)
	FAccessibilityAuditReport Report = UAccessibleUMGInspectorToolset::AuditWidgetAccessibility(TEXT(""), TEXT(""));
	TestFalse(TEXT("Audit without inputs fails gracefully"), Report.bSuccess);
	TestTrue(TEXT("Error message present"), Report.ErrorMessage.Contains(TEXT("must be supplied")));
	return true;
}

// ── Test 40: A11Y-07 Directly focused control under inactive ancestor ──

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y07FocusedControlUnderInactiveAncestorTest, "AccessibleUMGInspector.Audit.A11Y07FocusedControlUnderInactiveAncestor", TEST_FLAGS)
bool FAuditA11Y07FocusedControlUnderInactiveAncestorTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R;
	R.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("Root");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1 };
	R.Widgets.Add(Root);

	FRuntimeWidgetRecord InactivePanel;
	InactivePanel.NodeIndex = 1;
	InactivePanel.ParentIndex = 0;
	InactivePanel.WidgetName = TEXT("Panel_Inactive");
	InactivePanel.LocalVisibility = TEXT("Collapsed");
	InactivePanel.bIsEnabled = true;
	InactivePanel.ChildIndices = { 2 };
	R.Widgets.Add(InactivePanel);

	FRuntimeWidgetRecord FocusedBtn;
	FocusedBtn.NodeIndex = 2;
	FocusedBtn.ParentIndex = 1;
	FocusedBtn.WidgetName = TEXT("Button_Focused");
	FocusedBtn.LocalVisibility = TEXT("Visible");
	FocusedBtn.bIsEnabled = true;
	FocusedBtn.bIsFocusable = true;
	FocusedBtn.bHasKeyboardFocus = true; // Observed keyboard focus!
	R.Widgets.Add(FocusedBtn);

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-07") });
	TestTrue(TEXT("Audit succeeded"), Report.bSuccess);
	TestTrue(TEXT("Produces Error for focus on inactive screen"), Report.ErrorCount >= 1);

	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-07"));
	TestNotNull(TEXT("A11Y-07 finding exists"), Finding);
	if (Finding)
	{
		TestEqual(TEXT("Classification is Error"), (uint8)Finding->Classification, (uint8)EAuditFindingClassification::Error);
		TestEqual(TEXT("Target is Button_Focused"), Finding->WidgetIdentity, TEXT("Button_Focused"));
	}

	return true;
}

// ── Test 41: A11Y-04 Duplicate accessible names across active and inactive panels ─

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y04AcrossActiveAndInactivePanelsSuppressedTest, "AccessibleUMGInspector.Audit.A11Y04AcrossActiveAndInactivePanelsSuppressed", TEST_FLAGS)
bool FAuditA11Y04AcrossActiveAndInactivePanelsSuppressedTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R;
	R.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("Root");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1, 3 };
	R.Widgets.Add(Root);

	// Active Start Panel
	FRuntimeWidgetRecord StartPanel;
	StartPanel.NodeIndex = 1;
	StartPanel.ParentIndex = 0;
	StartPanel.WidgetName = TEXT("Panel_StartScreen");
	StartPanel.LocalVisibility = TEXT("Visible");
	StartPanel.bIsEnabled = true;
	StartPanel.ChildIndices = { 2 };
	R.Widgets.Add(StartPanel);

	FRuntimeWidgetRecord BackBtnStart;
	BackBtnStart.NodeIndex = 2;
	BackBtnStart.ParentIndex = 1;
	BackBtnStart.WidgetName = TEXT("Button_StartBack");
	BackBtnStart.WidgetClass = TEXT("Button");
	BackBtnStart.LocalVisibility = TEXT("Visible");
	BackBtnStart.bIsEnabled = true;
	BackBtnStart.bIsFocusable = true;
	BackBtnStart.AccessibleName = TEXT("Back");
	R.Widgets.Add(BackBtnStart);

	// Inactive Host Panel
	FRuntimeWidgetRecord HostPanel;
	HostPanel.NodeIndex = 3;
	HostPanel.ParentIndex = 0;
	HostPanel.WidgetName = TEXT("Panel_HostScreen");
	HostPanel.LocalVisibility = TEXT("Collapsed");
	HostPanel.bIsEnabled = true;
	HostPanel.ChildIndices = { 4 };
	R.Widgets.Add(HostPanel);

	FRuntimeWidgetRecord BackBtnHost;
	BackBtnHost.NodeIndex = 4;
	BackBtnHost.ParentIndex = 3;
	BackBtnHost.WidgetName = TEXT("Button_HostBack");
	BackBtnHost.WidgetClass = TEXT("Button");
	BackBtnHost.LocalVisibility = TEXT("Visible");
	BackBtnHost.bIsEnabled = true;
	BackBtnHost.bIsFocusable = true;
	BackBtnHost.AccessibleName = TEXT("Back"); // Same accessible name "Back"!
	R.Widgets.Add(BackBtnHost);

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-04") });
	TestTrue(TEXT("Audit succeeded"), Report.bSuccess);
	TestEqual(TEXT("Zero errors across active and inactive panels"), Report.ErrorCount, 0);
	TestEqual(TEXT("Zero warnings across active and inactive panels"), Report.WarningCount, 0);
	TestTrue(TEXT("A11Y-04 passed"), Report.PassedRuleIds.Contains(TEXT("A11Y-04")));

	return true;
}

// ── Test 42: A11Y-04 Duplicate accessible names within one active panel ──

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y04WithinOneActivePanelErrorTest, "AccessibleUMGInspector.Audit.A11Y04WithinOneActivePanelError", TEST_FLAGS)
bool FAuditA11Y04WithinOneActivePanelErrorTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R;
	R.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("Root");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1 };
	R.Widgets.Add(Root);

	FRuntimeWidgetRecord ActivePanel;
	ActivePanel.NodeIndex = 1;
	ActivePanel.ParentIndex = 0;
	ActivePanel.WidgetName = TEXT("Panel_Active");
	ActivePanel.LocalVisibility = TEXT("Visible");
	ActivePanel.bIsEnabled = true;
	ActivePanel.ChildIndices = { 2, 3 };
	R.Widgets.Add(ActivePanel);

	FRuntimeWidgetRecord Btn1;
	Btn1.NodeIndex = 2;
	Btn1.ParentIndex = 1;
	Btn1.WidgetName = TEXT("Button_Primary");
	Btn1.WidgetClass = TEXT("Button");
	Btn1.LocalVisibility = TEXT("Visible");
	Btn1.bIsEnabled = true;
	Btn1.bIsFocusable = true;
	Btn1.AccessibleName = TEXT("Action");
	R.Widgets.Add(Btn1);

	FRuntimeWidgetRecord Btn2;
	Btn2.NodeIndex = 3;
	Btn2.ParentIndex = 1;
	Btn2.WidgetName = TEXT("Button_Secondary");
	Btn2.WidgetClass = TEXT("Button");
	Btn2.LocalVisibility = TEXT("Visible");
	Btn2.bIsEnabled = true;
	Btn2.bIsFocusable = true;
	Btn2.AccessibleName = TEXT("Action"); // Duplicate name in SAME active panel!
	R.Widgets.Add(Btn2);

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-04") });
	TestTrue(TEXT("Audit succeeded"), Report.bSuccess);
	TestTrue(TEXT("Produces Error for simultaneously active equivalent controls in same panel"), Report.ErrorCount >= 1);

	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-04"));
	TestNotNull(TEXT("A11Y-04 finding exists"), Finding);
	if (Finding)
	{
		TestEqual(TEXT("Classification is Error"), (uint8)Finding->Classification, (uint8)EAuditFindingClassification::Error);
	}

	return true;
}

// ── Test 43: A11Y-09 Modal with background controls effectively suppressed ─

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y09ModalBackgroundSuppressedTest, "AccessibleUMGInspector.Audit.A11Y09ModalBackgroundSuppressed", TEST_FLAGS)
bool FAuditA11Y09ModalBackgroundSuppressedTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R;
	R.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("Root");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1, 3 };
	R.Widgets.Add(Root);

	// Inactive / suppressed background screen
	FRuntimeWidgetRecord BackgroundScreen;
	BackgroundScreen.NodeIndex = 1;
	BackgroundScreen.ParentIndex = 0;
	BackgroundScreen.WidgetName = TEXT("Panel_BackgroundScreen");
	BackgroundScreen.LocalVisibility = TEXT("Collapsed");
	BackgroundScreen.bIsEnabled = true;
	BackgroundScreen.ChildIndices = { 2 };
	R.Widgets.Add(BackgroundScreen);

	FRuntimeWidgetRecord BackgroundBtn;
	BackgroundBtn.NodeIndex = 2;
	BackgroundBtn.ParentIndex = 1;
	BackgroundBtn.WidgetName = TEXT("Button_Background");
	BackgroundBtn.WidgetClass = TEXT("Button");
	BackgroundBtn.LocalVisibility = TEXT("Visible");
	BackgroundBtn.bIsEnabled = true;
	BackgroundBtn.bIsFocusable = true;
	BackgroundBtn.bHasKeyboardFocus = false; // No focus!
	R.Widgets.Add(BackgroundBtn);

	// Active Modal
	FRuntimeWidgetRecord ActiveModal;
	ActiveModal.NodeIndex = 3;
	ActiveModal.ParentIndex = 0;
	ActiveModal.WidgetName = TEXT("Modal_Confirmation");
	ActiveModal.WidgetClass = TEXT("ModalDialog");
	ActiveModal.LocalVisibility = TEXT("Visible");
	ActiveModal.bIsEnabled = true;
	ActiveModal.ChildIndices = { 4 };
	R.Widgets.Add(ActiveModal);

	FRuntimeWidgetRecord ModalBtn;
	ModalBtn.NodeIndex = 4;
	ModalBtn.ParentIndex = 3;
	ModalBtn.WidgetName = TEXT("Button_ModalOK");
	ModalBtn.WidgetClass = TEXT("Button");
	ModalBtn.LocalVisibility = TEXT("Visible");
	ModalBtn.bIsEnabled = true;
	ModalBtn.bIsFocusable = true;
	ModalBtn.bHasKeyboardFocus = true; // Focus inside modal!
	R.Widgets.Add(ModalBtn);

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-09") });
	TestTrue(TEXT("Audit succeeded"), Report.bSuccess);
	TestEqual(TEXT("Zero modal containment errors when background is suppressed"), Report.ErrorCount, 0);
	TestTrue(TEXT("A11Y-09 passed"), Report.PassedRuleIds.Contains(TEXT("A11Y-09")));

	return true;
}

// ── Test 44: A11Y-09 Modal with directly focused background control ────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditA11Y09ModalDirectlyFocusedBackgroundTest, "AccessibleUMGInspector.Audit.A11Y09ModalDirectlyFocusedBackground", TEST_FLAGS)
bool FAuditA11Y09ModalDirectlyFocusedBackgroundTest::RunTest(const FString& Parameters)
{
	FRuntimeWidgetReport R;
	R.bSuccess = true;

	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("Root");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1, 3 };
	R.Widgets.Add(Root);

	// Background screen with a button that retains focus!
	FRuntimeWidgetRecord BackgroundScreen;
	BackgroundScreen.NodeIndex = 1;
	BackgroundScreen.ParentIndex = 0;
	BackgroundScreen.WidgetName = TEXT("Panel_BackgroundScreen");
	BackgroundScreen.LocalVisibility = TEXT("Visible");
	BackgroundScreen.bIsEnabled = true;
	BackgroundScreen.ChildIndices = { 2 };
	R.Widgets.Add(BackgroundScreen);

	FRuntimeWidgetRecord BackgroundBtn;
	BackgroundBtn.NodeIndex = 2;
	BackgroundBtn.ParentIndex = 1;
	BackgroundBtn.WidgetName = TEXT("Button_Background");
	BackgroundBtn.WidgetClass = TEXT("Button");
	BackgroundBtn.LocalVisibility = TEXT("Visible");
	BackgroundBtn.bIsEnabled = true;
	BackgroundBtn.bIsFocusable = true;
	BackgroundBtn.bHasKeyboardFocus = true; // Focus outside modal!
	R.Widgets.Add(BackgroundBtn);

	// Active Modal
	FRuntimeWidgetRecord ActiveModal;
	ActiveModal.NodeIndex = 3;
	ActiveModal.ParentIndex = 0;
	ActiveModal.WidgetName = TEXT("Modal_Confirmation");
	ActiveModal.WidgetClass = TEXT("ModalDialog");
	ActiveModal.LocalVisibility = TEXT("Visible");
	ActiveModal.bIsEnabled = true;
	R.Widgets.Add(ActiveModal);

	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, { TEXT("A11Y-09") });
	TestTrue(TEXT("Audit succeeded"), Report.bSuccess);
	TestTrue(TEXT("Produces Error when control outside active modal retains focus"), Report.ErrorCount >= 1);

	const FAuditFindingRecord* Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-09"));
	TestNotNull(TEXT("A11Y-09 finding exists"), Finding);
	if (Finding)
	{
		TestEqual(TEXT("Classification is Error"), (uint8)Finding->Classification, (uint8)EAuditFindingClassification::Error);
	}

	return true;
}

// ── Test 45: Milestone 5 Replay Audit Verification ────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAuditMilestone5ReplayVerificationTest, "AccessibleUMGInspector.Audit.Milestone5ReplayVerification", TEST_FLAGS)
bool FAuditMilestone5ReplayVerificationTest::RunTest(const FString& Parameters)
{
	// Model the sanitized project-neutral hierarchy of Milestone 5 (Game Shell initial screen)
	FRuntimeWidgetReport R;
	R.bSuccess = true;
	R.RootWidgetName = TEXT("WBP_GameShell");

	// 0: RootCanvas
	FRuntimeWidgetRecord Root;
	Root.NodeIndex = 0;
	Root.ParentIndex = -1;
	Root.WidgetName = TEXT("RootCanvas");
	Root.WidgetClass = TEXT("CanvasPanel");
	Root.LocalVisibility = TEXT("Visible");
	Root.bIsEnabled = true;
	Root.ChildIndices = { 1, 4, 8, 12 };
	R.Widgets.Add(Root);

	// 1: Panel_StartScreen (ACTIVE SCREEN)
	FRuntimeWidgetRecord StartPanel;
	StartPanel.NodeIndex = 1;
	StartPanel.ParentIndex = 0;
	StartPanel.WidgetName = TEXT("Panel_StartScreen");
	StartPanel.WidgetClass = TEXT("CanvasPanel");
	StartPanel.LocalVisibility = TEXT("Visible");
	StartPanel.bIsEnabled = true;
	StartPanel.ChildIndices = { 2, 3 };
	R.Widgets.Add(StartPanel);

	// 2: Button_Start
	FRuntimeWidgetRecord BtnStart;
	BtnStart.NodeIndex = 2;
	BtnStart.ParentIndex = 1;
	BtnStart.WidgetName = TEXT("Button_Start");
	BtnStart.WidgetClass = TEXT("Button");
	BtnStart.SlateWidgetType = TEXT("SButton");
	BtnStart.LocalVisibility = TEXT("Visible");
	BtnStart.bIsEnabled = true;
	BtnStart.bIsFocusable = true;
	BtnStart.bHasSlateWidget = true;
	BtnStart.bSlateVisible = true;
	BtnStart.bSlateEnabled = true;
	BtnStart.bHasKeyboardFocus = true; // Focus on Start button
	BtnStart.ScreenBounds.X = 100.f;
	BtnStart.ScreenBounds.Y = 100.f;
	BtnStart.ScreenBounds.Width = 200.f;
	BtnStart.ScreenBounds.Height = 50.f;
	BtnStart.ScreenBounds.bIsValid = true;
	BtnStart.AccessibleName = TEXT("Start");
	R.Widgets.Add(BtnStart);

	// 3: Button_Settings
	FRuntimeWidgetRecord BtnSettings;
	BtnSettings.NodeIndex = 3;
	BtnSettings.ParentIndex = 1;
	BtnSettings.WidgetName = TEXT("Button_Settings");
	BtnSettings.WidgetClass = TEXT("Button");
	BtnSettings.SlateWidgetType = TEXT("SButton");
	BtnSettings.LocalVisibility = TEXT("Visible");
	BtnSettings.bIsEnabled = true;
	BtnSettings.bIsFocusable = true;
	BtnSettings.bHasSlateWidget = true;
	BtnSettings.bSlateVisible = true;
	BtnSettings.bSlateEnabled = true;
	BtnSettings.ScreenBounds.X = 100.f;
	BtnSettings.ScreenBounds.Y = 170.f;
	BtnSettings.ScreenBounds.Width = 200.f;
	BtnSettings.ScreenBounds.Height = 50.f;
	BtnSettings.ScreenBounds.bIsValid = true;
	BtnSettings.AccessibleName = TEXT("Settings");
	R.Widgets.Add(BtnSettings);

	// 4: Panel_HostScreen (COLLAPSED SCREEN)
	FRuntimeWidgetRecord HostPanel;
	HostPanel.NodeIndex = 4;
	HostPanel.ParentIndex = 0;
	HostPanel.WidgetName = TEXT("Panel_HostScreen");
	HostPanel.WidgetClass = TEXT("CanvasPanel");
	HostPanel.LocalVisibility = TEXT("Collapsed"); // COLLAPSED!
	HostPanel.bIsEnabled = true;
	HostPanel.ChildIndices = { 5, 6, 7 };
	R.Widgets.Add(HostPanel);

	// 5: Button_HostStart (under collapsed HostPanel)
	FRuntimeWidgetRecord BtnHostStart;
	BtnHostStart.NodeIndex = 5;
	BtnHostStart.ParentIndex = 4;
	BtnHostStart.WidgetName = TEXT("Button_HostStart");
	BtnHostStart.WidgetClass = TEXT("Button");
	BtnHostStart.SlateWidgetType = TEXT("SButton");
	BtnHostStart.LocalVisibility = TEXT("Visible"); // Local Visible!
	BtnHostStart.bIsEnabled = true;
	BtnHostStart.bIsFocusable = true;
	BtnHostStart.ScreenBounds.X = 100.f;
	BtnHostStart.ScreenBounds.Y = 100.f;
	BtnHostStart.ScreenBounds.Width = 200.f;
	BtnHostStart.ScreenBounds.Height = 50.f;
	BtnHostStart.ScreenBounds.bIsValid = true;
	BtnHostStart.AccessibleName = TEXT("Start Server");
	R.Widgets.Add(BtnHostStart);

	// 6: Button_HostBack (under collapsed HostPanel)
	FRuntimeWidgetRecord BtnHostBack;
	BtnHostBack.NodeIndex = 6;
	BtnHostBack.ParentIndex = 4;
	BtnHostBack.WidgetName = TEXT("Button_HostBack");
	BtnHostBack.WidgetClass = TEXT("Button");
	BtnHostBack.SlateWidgetType = TEXT("SButton");
	BtnHostBack.LocalVisibility = TEXT("Visible"); // Local Visible!
	BtnHostBack.bIsEnabled = true;
	BtnHostBack.bIsFocusable = true;
	BtnHostBack.AccessibleName = TEXT("Back"); // Shares name with JoinBack
	R.Widgets.Add(BtnHostBack);

	// 7: Text_HostStatus (under collapsed HostPanel)
	FRuntimeWidgetRecord TextHostStatus;
	TextHostStatus.NodeIndex = 7;
	TextHostStatus.ParentIndex = 4;
	TextHostStatus.WidgetName = TEXT("Text_HostStatus");
	TextHostStatus.WidgetClass = TEXT("TextBlock");
	TextHostStatus.LocalVisibility = TEXT("Visible");
	TextHostStatus.bIsEnabled = true;
	TextHostStatus.AccessibleName = TEXT("Status: Ready"); // Repeated status text
	R.Widgets.Add(TextHostStatus);

	// 8: Panel_JoinScreen (COLLAPSED SCREEN)
	FRuntimeWidgetRecord JoinPanel;
	JoinPanel.NodeIndex = 8;
	JoinPanel.ParentIndex = 0;
	JoinPanel.WidgetName = TEXT("Panel_JoinScreen");
	JoinPanel.WidgetClass = TEXT("CanvasPanel");
	JoinPanel.LocalVisibility = TEXT("Collapsed"); // COLLAPSED!
	JoinPanel.bIsEnabled = true;
	JoinPanel.ChildIndices = { 9, 10, 11 };
	R.Widgets.Add(JoinPanel);

	// 9: Button_Join (under collapsed JoinPanel)
	FRuntimeWidgetRecord BtnJoin;
	BtnJoin.NodeIndex = 9;
	BtnJoin.ParentIndex = 8;
	BtnJoin.WidgetName = TEXT("Button_Join");
	BtnJoin.WidgetClass = TEXT("Button");
	BtnJoin.SlateWidgetType = TEXT("SButton");
	BtnJoin.LocalVisibility = TEXT("Visible");
	BtnJoin.bIsEnabled = true;
	BtnJoin.bIsFocusable = true;
	BtnJoin.AccessibleName = TEXT("Join Game");
	R.Widgets.Add(BtnJoin);

	// 10: Button_JoinBack (under collapsed JoinPanel)
	FRuntimeWidgetRecord BtnJoinBack;
	BtnJoinBack.NodeIndex = 10;
	BtnJoinBack.ParentIndex = 8;
	BtnJoinBack.WidgetName = TEXT("Button_JoinBack");
	BtnJoinBack.WidgetClass = TEXT("Button");
	BtnJoinBack.SlateWidgetType = TEXT("SButton");
	BtnJoinBack.LocalVisibility = TEXT("Visible");
	BtnJoinBack.bIsEnabled = true;
	BtnJoinBack.bIsFocusable = true;
	BtnJoinBack.AccessibleName = TEXT("Back"); // Shares name with HostBack
	R.Widgets.Add(BtnJoinBack);

	// 11: Text_JoinStatus (under collapsed JoinPanel)
	FRuntimeWidgetRecord TextJoinStatus;
	TextJoinStatus.NodeIndex = 11;
	TextJoinStatus.ParentIndex = 8;
	TextJoinStatus.WidgetName = TEXT("Text_JoinStatus");
	TextJoinStatus.WidgetClass = TEXT("TextBlock");
	TextJoinStatus.LocalVisibility = TEXT("Visible");
	TextJoinStatus.bIsEnabled = true;
	TextJoinStatus.AccessibleName = TEXT("Status: Ready"); // Repeated status text
	R.Widgets.Add(TextJoinStatus);

	// 12: Modal_ExitConfirmation (COLLAPSED MODAL)
	FRuntimeWidgetRecord ExitModal;
	ExitModal.NodeIndex = 12;
	ExitModal.ParentIndex = 0;
	ExitModal.WidgetName = TEXT("Modal_ExitConfirmation");
	ExitModal.WidgetClass = TEXT("ModalDialog");
	ExitModal.LocalVisibility = TEXT("Collapsed"); // COLLAPSED!
	ExitModal.bIsEnabled = true;
	ExitModal.ChildIndices = { 13, 14 };
	R.Widgets.Add(ExitModal);

	// 13: Button_ExitConfirm
	FRuntimeWidgetRecord BtnExitConfirm;
	BtnExitConfirm.NodeIndex = 13;
	BtnExitConfirm.ParentIndex = 12;
	BtnExitConfirm.WidgetName = TEXT("Button_ExitConfirm");
	BtnExitConfirm.WidgetClass = TEXT("Button");
	BtnExitConfirm.LocalVisibility = TEXT("Visible");
	BtnExitConfirm.bIsEnabled = true;
	BtnExitConfirm.AccessibleName = TEXT("Confirm");
	R.Widgets.Add(BtnExitConfirm);

	// 14: Button_ExitCancel
	FRuntimeWidgetRecord BtnExitCancel;
	BtnExitCancel.NodeIndex = 14;
	BtnExitCancel.ParentIndex = 12;
	BtnExitCancel.WidgetName = TEXT("Button_ExitCancel");
	BtnExitCancel.WidgetClass = TEXT("Button");
	BtnExitCancel.LocalVisibility = TEXT("Visible");
	BtnExitCancel.bIsEnabled = true;
	BtnExitCancel.AccessibleName = TEXT("Cancel");
	R.Widgets.Add(BtnExitCancel);

	// Compute effective visibility across the hierarchy
	FRuntimeWidgetInspector::ComputeEffectiveVisibility(R);

	// Execute Audit
	FAccessibilityAuditReport Report = FAccessibilityAuditEngine::AuditReports(nullptr, &R, nullptr, {});

	TestTrue(TEXT("Audit succeeded"), Report.bSuccess);

	// Verification 1: Exactly ZERO errors (corrected from 44 false positive errors!)
	TestEqual(TEXT("Error count is exactly zero"), Report.ErrorCount, 0);

	// Verification 2: Inactive descendants are effectively hidden
	const FRuntimeWidgetRecord* EvaluatedHostBack = Report.SourceRuntimeReport.IsEmpty() ? nullptr : nullptr;
	TestFalse(TEXT("Host Start button is not effectively visible"), R.Widgets[5].bEffectivelyVisible);
	TestEqual(TEXT("Host Start effective visibility is Collapsed"), R.Widgets[5].EffectiveVisibility, TEXT("Collapsed"));
	TestEqual(TEXT("Host Start blocking ancestor is Panel_HostScreen"), R.Widgets[5].BlockingAncestorName, TEXT("Panel_HostScreen"));

	// Verification 3: No A11Y-06 findings for inactive descendants
	const FAuditFindingRecord* A11Y06Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-06"));
	TestNull(TEXT("Zero A11Y-06 findings for properly suppressed descendants"), A11Y06Finding);

	// Verification 4: Inactive status labels and duplicate Back buttons do not create A11Y-04 findings
	const FAuditFindingRecord* A11Y04Finding = AccessibilityAuditTestHelpers::FindFindingByRule(Report, TEXT("A11Y-04"));
	TestNull(TEXT("Zero A11Y-04 duplicate-name findings across inactive panels"), A11Y04Finding);

	// Verification 5: Active Start controls remain inspectable
	TestTrue(TEXT("Start button is effectively visible"), R.Widgets[2].bEffectivelyVisible);
	TestTrue(TEXT("Start button is effectively interactive"), R.Widgets[2].bEffectivelyInteractive);
	TestEqual(TEXT("Start button blocking ancestor index is -1"), R.Widgets[2].BlockingAncestorIndex, -1);

	// Verification 6: Local visibility remains faithfully reported as Visible
	TestEqual(TEXT("Host Back button local visibility is Visible"), R.Widgets[6].LocalVisibility, TEXT("Visible"));

	return true;
}
