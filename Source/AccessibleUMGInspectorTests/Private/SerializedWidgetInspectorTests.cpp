// Copyright Blind Lion Studios. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "SerializedWidgetInspector.h"
#include "NvdaTextFormatter.h"
#include "AccessibleUMGInspectorTypes.h"

#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetNavigation.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#define TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace AccessibleUMGInspectorTestHelpers
{
	/** Creates a synthetic, project-neutral Widget Blueprint in memory for unit testing */
	static UWidgetBlueprint* CreateSyntheticTestBlueprint(UObject* Outer = nullptr)
	{
		if (!Outer)
		{
			Outer = GetTransientPackage();
		}

		UWidgetBlueprint* BP = NewObject<UWidgetBlueprint>(Outer, *FString::Printf(TEXT("WBP_SyntheticFixture_%s"), *FGuid::NewGuid().ToString()));
		BP->WidgetTree = NewObject<UWidgetTree>(BP);

		// Root Canvas
		UCanvasPanel* RootCanvas = BP->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		BP->WidgetTree->RootWidget = RootCanvas;

		// Main Vertical Box
		UVerticalBox* VertBox = BP->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainVerticalBox"));
		UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(VertBox);
		CanvasSlot->SetOffsets(FMargin(10.f, 20.f, 300.f, 400.f));

		// Button Primary
		UButton* ButtonPrimary = BP->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Primary"));
		ButtonPrimary->SetIsEnabled(true);
		ButtonPrimary->SetVisibility(ESlateVisibility::Visible);
		if (FBoolProperty* FocusProp = CastField<FBoolProperty>(ButtonPrimary->GetClass()->FindPropertyByName(TEXT("IsFocusable"))))
		{
			FocusProp->SetPropertyValue_InContainer(ButtonPrimary, true);
		}
		ButtonPrimary->bIsVariable = true;
#if WITH_EDITORONLY_DATA
		ButtonPrimary->bOverrideAccessibleDefaults = true;
		ButtonPrimary->AccessibleText = FText::FromString(TEXT("Primary Action"));
		ButtonPrimary->AccessibleSummaryText = FText::FromString(TEXT("Triggers main operation"));
		ButtonPrimary->AccessibleBehavior = ESlateAccessibleBehavior::Custom;
#endif
		// Navigation rule on ButtonPrimary: Down -> Explicit to Button_Secondary
		ButtonPrimary->Navigation = NewObject<UWidgetNavigation>(ButtonPrimary);
		FWidgetNavigationData& DownData = ButtonPrimary->Navigation->GetNavigationData(EUINavigation::Down);
		DownData.Rule = EUINavigationRule::Explicit;
		DownData.WidgetToFocus = TEXT("Button_Secondary");

		UVerticalBoxSlot* VertSlot1 = VertBox->AddChildToVerticalBox(ButtonPrimary);
		VertSlot1->SetPadding(FMargin(0.f, 4.f, 0.f, 4.f));

		// Button Secondary
		UButton* ButtonSecondary = BP->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Secondary"));
		ButtonSecondary->SetIsEnabled(false);
		ButtonSecondary->SetVisibility(ESlateVisibility::Visible);
		if (FBoolProperty* FocusProp = CastField<FBoolProperty>(ButtonSecondary->GetClass()->FindPropertyByName(TEXT("IsFocusable"))))
		{
			FocusProp->SetPropertyValue_InContainer(ButtonSecondary, true);
		}
#if WITH_EDITORONLY_DATA
		ButtonSecondary->bOverrideAccessibleDefaults = true;
		ButtonSecondary->AccessibleText = FText::FromString(TEXT("Secondary Action"));
		ButtonSecondary->AccessibleBehavior = ESlateAccessibleBehavior::Custom;
#endif
		UVerticalBoxSlot* VertSlot2 = VertBox->AddChildToVerticalBox(ButtonSecondary);
		VertSlot2->SetPadding(FMargin(0.f, 4.f, 0.f, 4.f));

		// TextBlock
		UTextBlock* TextStatus = BP->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Text_Status"));
		TextStatus->SetVisibility(ESlateVisibility::Collapsed);
		TextStatus->SetText(FText::FromString(TEXT("System Ready")));
		VertBox->AddChildToVerticalBox(TextStatus);

		// Add a delegate binding
		FDelegateEditorBinding Binding;
		Binding.ObjectName = TEXT("Button_Primary");
		Binding.PropertyName = TEXT("OnClicked");
		Binding.FunctionName = TEXT("HandlePrimaryClicked");
		Binding.Kind = EBindingKind::Function;
		BP->Bindings.Add(Binding);

		return BP;
	}

	/** Creates a synthetic blueprint with duplicate names for validation */
	static UWidgetBlueprint* CreateDuplicateNameFixture(UObject* Outer = nullptr)
	{
		if (!Outer)
		{
			Outer = GetTransientPackage();
		}

		UWidgetBlueprint* BP = NewObject<UWidgetBlueprint>(Outer, *FString::Printf(TEXT("WBP_DuplicateFixture_%s"), *FGuid::NewGuid().ToString()));
		BP->WidgetTree = NewObject<UWidgetTree>(BP);

		UVerticalBox* VertBox = BP->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DuplicateRoot"));
		BP->WidgetTree->RootWidget = VertBox;

		UButton* Btn1 = BP->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Action"));
#if WITH_EDITORONLY_DATA
		Btn1->bOverrideAccessibleDefaults = true;
		Btn1->AccessibleText = FText::FromString(TEXT("Perform Operation"));
#endif
		VertBox->AddChildToVerticalBox(Btn1);

		// Intentionally assign identical accessible text to another button
		UButton* Btn2 = BP->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Action_Copy"));
#if WITH_EDITORONLY_DATA
		Btn2->bOverrideAccessibleDefaults = true;
		Btn2->AccessibleText = FText::FromString(TEXT("Perform Operation"));
#endif
		VertBox->AddChildToVerticalBox(Btn2);

		return BP;
	}

	static const FSerializedWidgetRecord* FindWidgetByName(const FSerializedWidgetBlueprintReport& Report, const FString& Name)
	{
		for (const FSerializedWidgetRecord& W : Report.Widgets)
		{
			if (W.WidgetName == Name)
			{
				return &W;
			}
		}
		return nullptr;
	}
}

// ── Test 1: Capability Query ─────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccessibleUMGInspectorCapabilitiesTest, "AccessibleUMGInspector.Serialized.Capabilities", TEST_FLAGS)
bool FAccessibleUMGInspectorCapabilitiesTest::RunTest(const FString& Parameters)
{
	FInspectorCapabilities Caps;
	TestEqual(TEXT("PluginName should match"), Caps.PluginName, TEXT("AccessibleUMGInspector"));
	TestEqual(TEXT("PluginVersion should match"), Caps.PluginVersion, TEXT("0.1.0-rc.1"));
	TestEqual(TEXT("EngineVersion should match"), Caps.SupportedEngineVersion, TEXT("5.8.2"));
	TestTrue(TEXT("ReadOnlyByDesign should be true"), Caps.bReadOnlyByDesign);
	TestTrue(TEXT("RequiresExplicitAssetPath should be true"), Caps.bRequiresExplicitAssetPath);
	TestTrue(TEXT("ZeroSyntheticInput should be true"), Caps.bZeroSyntheticInput);
	TestTrue(TEXT("ZeroPackageMutation should be true"), Caps.bZeroPackageMutation);
	return true;
}

// ── Test 2: Ordered Serialized Traversal & Hierarchy ─────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccessibleUMGInspectorTraversalTest, "AccessibleUMGInspector.Serialized.OrderedTraversal", TEST_FLAGS)
bool FAccessibleUMGInspectorTraversalTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* BP = AccessibleUMGInspectorTestHelpers::CreateSyntheticTestBlueprint();
	TestNotNull(TEXT("Synthetic blueprint should be created"), BP);

	FSerializedWidgetBlueprintReport Report = FSerializedWidgetInspector::InspectBlueprint(BP, TEXT("Both"));
	TestTrue(TEXT("Report should succeed"), Report.bSuccess);
	TestEqual(TEXT("Total widget count should be 5"), Report.TotalWidgets, 5);
	TestEqual(TEXT("Report Widgets array count should be 5"), Report.Widgets.Num(), 5);

	// Root should be RootCanvas
	TestTrue(TEXT("Root index should be valid"), Report.RootWidgetIndex >= 0);
	const FSerializedWidgetRecord& RootRecord = Report.Widgets[Report.RootWidgetIndex];
	TestEqual(TEXT("Root widget name"), RootRecord.WidgetName, TEXT("RootCanvas"));
	TestEqual(TEXT("Root widget class"), RootRecord.WidgetClass, TEXT("CanvasPanel"));
	TestEqual(TEXT("Root should have 1 child"), RootRecord.ChildIndices.Num(), 1);

	// Level 1 child should be MainVerticalBox
	int32 VertBoxIndex = RootRecord.ChildIndices[0];
	const FSerializedWidgetRecord& VertBoxRecord = Report.Widgets[VertBoxIndex];
	TestEqual(TEXT("Level 1 child name"), VertBoxRecord.WidgetName, TEXT("MainVerticalBox"));
	TestEqual(TEXT("Level 1 parent name"), VertBoxRecord.ParentName, TEXT("RootCanvas"));
	TestEqual(TEXT("Level 1 child index"), VertBoxRecord.ChildIndex, 0);
	TestEqual(TEXT("Level 1 children count"), VertBoxRecord.ChildIndices.Num(), 3);

	// Level 2 children
	int32 BtnPrimaryIdx = VertBoxRecord.ChildIndices[0];
	int32 BtnSecondaryIdx = VertBoxRecord.ChildIndices[1];
	int32 TxtStatusIdx = VertBoxRecord.ChildIndices[2];

	TestEqual(TEXT("Child 0 should be Button_Primary"), Report.Widgets[BtnPrimaryIdx].WidgetName, TEXT("Button_Primary"));
	TestEqual(TEXT("Child 1 should be Button_Secondary"), Report.Widgets[BtnSecondaryIdx].WidgetName, TEXT("Button_Secondary"));
	TestEqual(TEXT("Child 2 should be Text_Status"), Report.Widgets[TxtStatusIdx].WidgetName, TEXT("Text_Status"));

	return true;
}

// ── Test 3: Panel Slots & Layout Metadata ────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccessibleUMGInspectorPanelSlotsTest, "AccessibleUMGInspector.Serialized.PanelSlots", TEST_FLAGS)
bool FAccessibleUMGInspectorPanelSlotsTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* BP = AccessibleUMGInspectorTestHelpers::CreateSyntheticTestBlueprint();
	FSerializedWidgetBlueprintReport Report = FSerializedWidgetInspector::InspectBlueprint(BP, TEXT("Both"));

	const FSerializedWidgetRecord* VertBoxRecord = AccessibleUMGInspectorTestHelpers::FindWidgetByName(Report, TEXT("MainVerticalBox"));
	TestNotNull(TEXT("MainVerticalBox should exist"), VertBoxRecord);
	if (VertBoxRecord)
	{
		TestTrue(TEXT("MainVerticalBox should have slot"), VertBoxRecord->bHasSlot);
		TestEqual(TEXT("Slot type should be CanvasPanelSlot"), VertBoxRecord->Slot.SlotType, TEXT("CanvasPanelSlot"));
		TestEqual(TEXT("Slot parent should be RootCanvas"), VertBoxRecord->Slot.ParentWidgetName, TEXT("RootCanvas"));
	}

	const FSerializedWidgetRecord* BtnRecord = AccessibleUMGInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Primary"));
	TestNotNull(TEXT("Button_Primary should exist"), BtnRecord);
	if (BtnRecord)
	{
		TestTrue(TEXT("Button_Primary should have slot"), BtnRecord->bHasSlot);
		TestEqual(TEXT("Slot type should be VerticalBoxSlot"), BtnRecord->Slot.SlotType, TEXT("VerticalBoxSlot"));
		TestEqual(TEXT("Slot parent should be MainVerticalBox"), BtnRecord->Slot.ParentWidgetName, TEXT("MainVerticalBox"));
	}

	return true;
}

// ── Test 4: Visibility, Enabled State, & Focusability ────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccessibleUMGInspectorWidgetStateTest, "AccessibleUMGInspector.Serialized.WidgetState", TEST_FLAGS)
bool FAccessibleUMGInspectorWidgetStateTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* BP = AccessibleUMGInspectorTestHelpers::CreateSyntheticTestBlueprint();
	FSerializedWidgetBlueprintReport Report = FSerializedWidgetInspector::InspectBlueprint(BP, TEXT("Both"));

	const FSerializedWidgetRecord* BtnPrimary = AccessibleUMGInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Primary"));
	const FSerializedWidgetRecord* BtnSecondary = AccessibleUMGInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Secondary"));
	const FSerializedWidgetRecord* TxtStatus = AccessibleUMGInspectorTestHelpers::FindWidgetByName(Report, TEXT("Text_Status"));

	TestNotNull(TEXT("BtnPrimary exists"), BtnPrimary);
	TestNotNull(TEXT("BtnSecondary exists"), BtnSecondary);
	TestNotNull(TEXT("TxtStatus exists"), TxtStatus);

	if (BtnPrimary && BtnSecondary && TxtStatus)
	{
		// Button_Primary: Visible, Enabled, Focusable, Variable
		TestEqual(TEXT("BtnPrimary visibility"), BtnPrimary->DefaultVisibility, TEXT("Visible"));
		TestTrue(TEXT("BtnPrimary enabled"), BtnPrimary->bIsEnabled);
		TestTrue(TEXT("BtnPrimary focusable"), BtnPrimary->bIsFocusable);
		TestTrue(TEXT("BtnPrimary variable"), BtnPrimary->bIsVariable);

		// Button_Secondary: Visible, Disabled, Focusable
		TestEqual(TEXT("BtnSecondary visibility"), BtnSecondary->DefaultVisibility, TEXT("Visible"));
		TestFalse(TEXT("BtnSecondary disabled"), BtnSecondary->bIsEnabled);
		TestTrue(TEXT("BtnSecondary focusable"), BtnSecondary->bIsFocusable);

		// Text_Status: Collapsed, Not Focusable
		TestEqual(TEXT("TxtStatus visibility"), TxtStatus->DefaultVisibility, TEXT("Collapsed"));
		TestFalse(TEXT("TxtStatus not focusable"), TxtStatus->bIsFocusable);
	}

	return true;
}

// ── Test 5: Accessibility Metadata Reflection ────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccessibleUMGInspectorAccessibilityDataTest, "AccessibleUMGInspector.Serialized.AccessibilityData", TEST_FLAGS)
bool FAccessibleUMGInspectorAccessibilityDataTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* BP = AccessibleUMGInspectorTestHelpers::CreateSyntheticTestBlueprint();
	FSerializedWidgetBlueprintReport Report = FSerializedWidgetInspector::InspectBlueprint(BP, TEXT("Both"));

	const FSerializedWidgetRecord* BtnPrimary = AccessibleUMGInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Primary"));
	TestNotNull(TEXT("BtnPrimary exists"), BtnPrimary);

	if (BtnPrimary)
	{
		TestTrue(TEXT("bOverrideAccessibleDefaults should be true"), BtnPrimary->bOverrideAccessibleDefaults);
		TestEqual(TEXT("AccessibleName should be 'Primary Action'"), BtnPrimary->AccessibleName, TEXT("Primary Action"));
		TestEqual(TEXT("AccessibleSummary should be 'Triggers main operation'"), BtnPrimary->AccessibleSummary, TEXT("Triggers main operation"));
		TestEqual(TEXT("AccessibleBehavior should be 'Custom'"), BtnPrimary->AccessibleBehavior, TEXT("Custom"));
	}

	return true;
}

// ── Test 6: Navigation Metadata ──────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccessibleUMGInspectorNavigationTest, "AccessibleUMGInspector.Serialized.NavigationMetadata", TEST_FLAGS)
bool FAccessibleUMGInspectorNavigationTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* BP = AccessibleUMGInspectorTestHelpers::CreateSyntheticTestBlueprint();
	FSerializedWidgetBlueprintReport Report = FSerializedWidgetInspector::InspectBlueprint(BP, TEXT("Both"));

	const FSerializedWidgetRecord* BtnPrimary = AccessibleUMGInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Primary"));
	TestNotNull(TEXT("BtnPrimary exists"), BtnPrimary);

	if (BtnPrimary)
	{
		TestEqual(TEXT("BtnPrimary should have 1 explicit nav rule"), BtnPrimary->NavigationRules.Num(), 1);

		if (BtnPrimary->NavigationRules.Num() > 0)
		{
			TestEqual(TEXT("Nav direction should be Down"), BtnPrimary->NavigationRules[0].Direction, TEXT("Down"));
			TestEqual(TEXT("Nav rule should be Explicit"), BtnPrimary->NavigationRules[0].Rule, TEXT("Explicit"));
			TestEqual(TEXT("Nav target should be Button_Secondary"), BtnPrimary->NavigationRules[0].TargetWidget, TEXT("Button_Secondary"));
		}
	}

	return true;
}

// ── Test 7: Bindings Reflection ──────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccessibleUMGInspectorBindingsTest, "AccessibleUMGInspector.Serialized.Bindings", TEST_FLAGS)
bool FAccessibleUMGInspectorBindingsTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* BP = AccessibleUMGInspectorTestHelpers::CreateSyntheticTestBlueprint();
	FSerializedWidgetBlueprintReport Report = FSerializedWidgetInspector::InspectBlueprint(BP, TEXT("Both"));

	const FSerializedWidgetRecord* BtnPrimary = AccessibleUMGInspectorTestHelpers::FindWidgetByName(Report, TEXT("Button_Primary"));
	TestNotNull(TEXT("BtnPrimary exists"), BtnPrimary);

	if (BtnPrimary)
	{
		TestEqual(TEXT("BtnPrimary should have 1 binding"), BtnPrimary->Bindings.Num(), 1);

		if (BtnPrimary->Bindings.Num() > 0)
		{
			TestEqual(TEXT("Binding property should be OnClicked"), BtnPrimary->Bindings[0].PropertyName, TEXT("OnClicked"));
			TestEqual(TEXT("Binding function should be HandlePrimaryClicked"), BtnPrimary->Bindings[0].FunctionName, TEXT("HandlePrimaryClicked"));
			TestEqual(TEXT("Binding kind should be Function"), BtnPrimary->Bindings[0].BindingKind, TEXT("Function"));
		}
	}

	return true;
}

// ── Test 8: Duplicate Name Detection ─────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccessibleUMGInspectorDuplicateNamesTest, "AccessibleUMGInspector.Serialized.DuplicateNames", TEST_FLAGS)
bool FAccessibleUMGInspectorDuplicateNamesTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* BP = AccessibleUMGInspectorTestHelpers::CreateDuplicateNameFixture();
	FSerializedWidgetBlueprintReport Report = FSerializedWidgetInspector::InspectBlueprint(BP, TEXT("Both"));

	TestTrue(TEXT("Report should complete"), Report.bSuccess);
	TestTrue(TEXT("Duplicate accessible name 'Perform Operation' should be detected"), Report.DuplicateAccessibleNames.Contains(TEXT("Perform Operation")));

	return true;
}

// ── Test 9: Read-Only Dirty State Invariance ─────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccessibleUMGInspectorDirtyStateInvarianceTest, "AccessibleUMGInspector.Serialized.DirtyStateInvariance", TEST_FLAGS)
bool FAccessibleUMGInspectorDirtyStateInvarianceTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* BP = AccessibleUMGInspectorTestHelpers::CreateSyntheticTestBlueprint();
	UPackage* Package = BP->GetOutermost();
	Package->SetDirtyFlag(false);
	TestFalse(TEXT("Package should start clean"), Package->IsDirty());

	FSerializedWidgetBlueprintReport Report = FSerializedWidgetInspector::InspectBlueprint(BP, TEXT("Both"));
	TestTrue(TEXT("Package was clean before"), Report.bPackageWasCleanBefore);
	TestTrue(TEXT("Package remains clean after"), Report.bPackageRemainsCleanAfter);
	TestFalse(TEXT("Package is not dirty after inspection"), Package->IsDirty());

	return true;
}

// ── Test 10: NVDA Plain Text Formatting ──────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccessibleUMGInspectorNvdaFormattingTest, "AccessibleUMGInspector.Serialized.NvdaFormatting", TEST_FLAGS)
bool FAccessibleUMGInspectorNvdaFormattingTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* BP = AccessibleUMGInspectorTestHelpers::CreateSyntheticTestBlueprint();
	FSerializedWidgetBlueprintReport Report = FSerializedWidgetInspector::InspectBlueprint(BP, TEXT("NvdaText"));

	TestFalse(TEXT("NVDA text should not be empty"), Report.NvdaText.IsEmpty());
	TestTrue(TEXT("NVDA text should contain headline"), Report.NvdaText.Contains(TEXT("Accessible UMG Inspector:")));
	TestTrue(TEXT("NVDA text should contain numbered root"), Report.NvdaText.Contains(TEXT("1. RootCanvas (CanvasPanel)")));
	TestTrue(TEXT("NVDA text should contain sub-numbered vertical box"), Report.NvdaText.Contains(TEXT("1.1. MainVerticalBox (VerticalBox)")));
	TestTrue(TEXT("NVDA text should contain sub-sub-numbered button"), Report.NvdaText.Contains(TEXT("1.1.1. Button_Primary (Button)")));
	TestTrue(TEXT("NVDA text should contain accessible name label"), Report.NvdaText.Contains(TEXT("Accessible Name: \"Primary Action\"")));
	TestTrue(TEXT("NVDA text should contain navigation target"), Report.NvdaText.Contains(TEXT("Nav Down: Explicit -> Button_Secondary")));
	TestTrue(TEXT("NVDA text should contain binding info"), Report.NvdaText.Contains(TEXT("Binding: OnClicked -> HandlePrimaryClicked")));

	return true;
}

// ── Test 11: Structured JSON Schema & Parsing Validity ───────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAccessibleUMGInspectorJsonValidityTest, "AccessibleUMGInspector.Serialized.JsonValidity", TEST_FLAGS)
bool FAccessibleUMGInspectorJsonValidityTest::RunTest(const FString& Parameters)
{
	UWidgetBlueprint* BP = AccessibleUMGInspectorTestHelpers::CreateSyntheticTestBlueprint();
	FSerializedWidgetBlueprintReport Report = FSerializedWidgetInspector::InspectBlueprint(BP, TEXT("Json"));

	TestFalse(TEXT("JSON string should not be empty"), Report.JsonData.IsEmpty());

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Report.JsonData);
	bool bParseSuccess = FJsonSerializer::Deserialize(Reader, JsonObject);

	TestTrue(TEXT("JSON should deserialize successfully"), bParseSuccess);
	TestNotNull(TEXT("Root JSON object should not be null"), JsonObject.Get());

	if (JsonObject.IsValid())
	{
		TestTrue(TEXT("JSON should have bSuccess"), JsonObject->HasField(TEXT("bSuccess")));
		TestTrue(TEXT("JSON should have totalWidgets"), JsonObject->HasField(TEXT("totalWidgets")));
		TestEqual(TEXT("JSON totalWidgets should be 5"), (int32)JsonObject->GetNumberField(TEXT("totalWidgets")), 5);
		TestTrue(TEXT("JSON should have widgets array"), JsonObject->HasField(TEXT("widgets")));
	}

	return true;
}

#undef TEST_FLAGS
