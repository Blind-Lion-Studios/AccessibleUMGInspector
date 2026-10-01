// Copyright Blind Lion Studios. All Rights Reserved.

#include "AccessibilityAuditEngine.h"
#include "SerializedWidgetInspector.h"
#include "RuntimeWidgetInspector.h"
#include "DifferentialComparator.h"
#include "NvdaTextFormatter.h"
#include "JsonObjectConverter.h"

FString FAccessibilityAuditEngine::GetClassificationString(EAuditFindingClassification Classification)
{
	switch (Classification)
	{
	case EAuditFindingClassification::Error:        return TEXT("Error");
	case EAuditFindingClassification::Warning:      return TEXT("Warning");
	case EAuditFindingClassification::Info:         return TEXT("Info");
	case EAuditFindingClassification::NotEvaluated: return TEXT("NotEvaluated");
	default:                                        return TEXT("Unknown");
	}
}

FString FAccessibilityAuditEngine::GetConfidenceBasisString(EAuditConfidenceBasis Basis)
{
	switch (Basis)
	{
	case EAuditConfidenceBasis::Direct:      return TEXT("Direct");
	case EAuditConfidenceBasis::Derived:     return TEXT("Derived");
	case EAuditConfidenceBasis::Ambiguous:   return TEXT("Ambiguous");
	case EAuditConfidenceBasis::Unavailable: return TEXT("Unavailable");
	default:                                 return TEXT("Unknown");
	}
}

bool FAccessibilityAuditEngine::IsInteractiveClass(const FString& WidgetClass)
{
	if (WidgetClass.IsEmpty())
	{
		return false;
	}

	static const TArray<FString> InteractiveClasses = {
		TEXT("Button"),
		TEXT("CheckBox"),
		TEXT("Slider"),
		TEXT("EditableText"),
		TEXT("EditableTextBox"),
		TEXT("MultiLineEditableText"),
		TEXT("MultiLineEditableTextBox"),
		TEXT("ComboBox"),
		TEXT("ComboBoxKey"),
		TEXT("ComboBoxString"),
		TEXT("SpinBox")
	};

	for (const FString& IC : InteractiveClasses)
	{
		if (WidgetClass.Equals(IC, ESearchCase::IgnoreCase) || WidgetClass.Contains(IC, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

FAccessibilityAuditReport FAccessibilityAuditEngine::Audit(
	const FString& AssetPath,
	const FString& RuntimeWidgetReference,
	const FString& WorldContext,
	const TArray<FString>& RuleSet,
	int32 MaxDepth,
	const FString& Format)
{
	FAccessibilityAuditReport Report;

	// Target requirement 1: At least one of AssetPath or RuntimeWidgetReference must be supplied
	if (AssetPath.IsEmpty() && RuntimeWidgetReference.IsEmpty())
	{
		Report.bSuccess = false;
		Report.Availability = EInspectionAvailability::Unavailable;
		Report.ErrorMessage = TEXT("At least one of AssetPath or RuntimeWidgetReference must be supplied.");
		return Report;
	}

	bool bHasAssetPath = !AssetPath.IsEmpty();
	bool bHasRuntimeRef = !RuntimeWidgetReference.IsEmpty();

	TUniquePtr<FSerializedWidgetBlueprintReport> SerializedReport;
	TUniquePtr<FRuntimeWidgetReport> RuntimeReport;
	TUniquePtr<FSerializedRuntimeComparisonReport> ComparisonReport;

	// 1. Run Serialized Inspector if AssetPath supplied
	if (bHasAssetPath)
	{
		SerializedReport = MakeUnique<FSerializedWidgetBlueprintReport>(
			FSerializedWidgetInspector::InspectAsset(AssetPath, TEXT("Both")));
		if (!SerializedReport->bSuccess && !bHasRuntimeRef)
		{
			Report.bSuccess = false;
			Report.Availability = EInspectionAvailability::Unavailable;
			Report.ErrorMessage = FString::Printf(TEXT("Serialized inspection failed: %s"), *SerializedReport->ErrorMessage);
			Report.AssetPath = AssetPath;
			return Report;
		}
	}

	// 2. Run Runtime Inspector if RuntimeWidgetReference or WorldContext supplied
	if (bHasRuntimeRef || (!bHasAssetPath && !WorldContext.IsEmpty()))
	{
		RuntimeReport = MakeUnique<FRuntimeWidgetReport>(
			FRuntimeWidgetInspector::InspectRuntimeUserWidget(RuntimeWidgetReference, WorldContext, MaxDepth, TEXT("Both")));
		if (!RuntimeReport->bSuccess && !bHasAssetPath)
		{
			Report.bSuccess = false;
			Report.Availability = RuntimeReport->Availability;
			Report.ErrorMessage = FString::Printf(TEXT("Runtime inspection failed: %s"), *RuntimeReport->ErrorMessage);
			Report.RuntimeWidgetIdentity = RuntimeWidgetReference;
			Report.WorldIdentity = WorldContext;
			Report.Candidates = RuntimeReport->Candidates;
			return Report;
		}
	}

	// 3. Run Comparison if both reports are valid
	if (SerializedReport && SerializedReport->bSuccess && RuntimeReport && RuntimeReport->bSuccess)
	{
		ComparisonReport = MakeUnique<FSerializedRuntimeComparisonReport>(
			FDifferentialComparator::ReconcileReports(*SerializedReport, *RuntimeReport, TEXT("Both")));
	}

	return AuditReports(
		SerializedReport.Get(),
		RuntimeReport.Get(),
		ComparisonReport.Get(),
		RuleSet,
		Format,
		AssetPath,
		RuntimeWidgetReference,
		WorldContext);
}

FAccessibilityAuditReport FAccessibilityAuditEngine::AuditReports(
	const FSerializedWidgetBlueprintReport* SerializedReport,
	const FRuntimeWidgetReport* RuntimeReport,
	const FSerializedRuntimeComparisonReport* ComparisonReport,
	const TArray<FString>& RuleSet,
	const FString& Format,
	const FString& OverrideAssetPath,
	const FString& OverrideRuntimeWidget,
	const FString& OverrideWorld)
{
	FAccessibilityAuditReport Report;
	Report.bSuccess = true;
	Report.Availability = EInspectionAvailability::Available;
	Report.RuleSetVersion = TEXT("1.0.0");

	// Ensure runtime report has computed effective visibility
	TUniquePtr<FRuntimeWidgetReport> LocalRuntimeCopy;
	const FRuntimeWidgetReport* EffectiveRuntimeReport = RuntimeReport;
	if (RuntimeReport && RuntimeReport->bSuccess && RuntimeReport->Widgets.Num() > 0 && RuntimeReport->Widgets[0].EffectiveVisibility.IsEmpty())
	{
		LocalRuntimeCopy = MakeUnique<FRuntimeWidgetReport>(*RuntimeReport);
		FRuntimeWidgetInspector::ComputeEffectiveVisibility(*LocalRuntimeCopy);
		EffectiveRuntimeReport = LocalRuntimeCopy.Get();
	}

	// Determine evidence mode and metadata
	bool bHasSerialized = (SerializedReport != nullptr && SerializedReport->bSuccess);
	bool bHasRuntime = (EffectiveRuntimeReport != nullptr && EffectiveRuntimeReport->bSuccess);
	bool bHasComparison = (ComparisonReport != nullptr && ComparisonReport->bSuccess);

	if (bHasSerialized && bHasRuntime)
	{
		Report.EvidenceMode = TEXT("Differential");
		Report.AssetPath = !OverrideAssetPath.IsEmpty() ? OverrideAssetPath : SerializedReport->AssetPath;
		Report.RuntimeWidgetIdentity = !OverrideRuntimeWidget.IsEmpty() ? OverrideRuntimeWidget : EffectiveRuntimeReport->RootWidgetName;
		Report.WorldIdentity = !OverrideWorld.IsEmpty() ? OverrideWorld : EffectiveRuntimeReport->WorldName;
		Report.SourceSerializedReport = Report.AssetPath;
		Report.SourceRuntimeReport = Report.RuntimeWidgetIdentity;
		Report.SourceComparisonReport = TEXT("Reconciled Differential Report");
		Report.bTruncated = EffectiveRuntimeReport->bTruncated;
	}
	else if (bHasSerialized)
	{
		Report.EvidenceMode = TEXT("Serialized");
		Report.AssetPath = !OverrideAssetPath.IsEmpty() ? OverrideAssetPath : SerializedReport->AssetPath;
		Report.SourceSerializedReport = Report.AssetPath;
	}
	else if (bHasRuntime)
	{
		Report.EvidenceMode = TEXT("Runtime");
		Report.RuntimeWidgetIdentity = !OverrideRuntimeWidget.IsEmpty() ? OverrideRuntimeWidget : EffectiveRuntimeReport->RootWidgetName;
		Report.WorldIdentity = !OverrideWorld.IsEmpty() ? OverrideWorld : EffectiveRuntimeReport->WorldName;
		Report.SourceRuntimeReport = Report.RuntimeWidgetIdentity;
		Report.bTruncated = EffectiveRuntimeReport->bTruncated;
	}
	else
	{
		Report.bSuccess = false;
		Report.Availability = EInspectionAvailability::Unavailable;
		Report.ErrorMessage = TEXT("No valid serialized or runtime evidence available for audit.");
		return Report;
	}

	// Propagate candidates if ambiguous
	if (EffectiveRuntimeReport && EffectiveRuntimeReport->Availability == EInspectionAvailability::Ambiguous)
	{
		Report.Availability = EInspectionAvailability::Ambiguous;
		Report.Candidates = EffectiveRuntimeReport->Candidates;
	}

	// Helper for rule filtering
	auto ShouldRunRule = [&RuleSet](const FString& RuleId) -> bool
	{
		if (RuleSet.Num() == 0)
		{
			return true;
		}
		for (const FString& R : RuleSet)
		{
			if (R.Equals(RuleId, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	};

	TArray<FAuditFindingRecord> RawFindings;
	TArray<FString> PassedRules;
	TArray<FString> NotEvaluatedRules;

	// Evaluate rules A11Y-01 through A11Y-14
	if (ShouldRunRule(TEXT("A11Y-01"))) EvaluateA11Y01(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-02"))) EvaluateA11Y02(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-03"))) EvaluateA11Y03(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-04"))) EvaluateA11Y04(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-05"))) EvaluateA11Y05(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-06"))) EvaluateA11Y06(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-07"))) EvaluateA11Y07(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-08"))) EvaluateA11Y08(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-09"))) EvaluateA11Y09(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-10"))) EvaluateA11Y10(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-11"))) EvaluateA11Y11(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-12"))) EvaluateA11Y12(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-13"))) EvaluateA11Y13(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);
	if (ShouldRunRule(TEXT("A11Y-14"))) EvaluateA11Y14(SerializedReport, EffectiveRuntimeReport, ComparisonReport, RawFindings, PassedRules, NotEvaluatedRules);

	// Deduplication using stable evidence identity (RuleId + WidgetIdentity + Message)
	TSet<FString> SeenFindingKeys;
	for (FAuditFindingRecord& Finding : RawFindings)
	{
		Finding.ClassificationString = GetClassificationString(Finding.Classification);
		Finding.ConfidenceBasisString = GetConfidenceBasisString(Finding.ConfidenceBasis);

		FString Key = FString::Printf(TEXT("%s:%s:%s"), *Finding.RuleId, *Finding.WidgetIdentity, *Finding.Message);
		if (!SeenFindingKeys.Contains(Key))
		{
			SeenFindingKeys.Add(Key);
			Report.Findings.Add(Finding);
		}
	}

	// Deterministic sorting: Error first, Warning, Info, NotEvaluated; then RuleId; then WidgetIdentity
	Report.Findings.Sort([](const FAuditFindingRecord& A, const FAuditFindingRecord& B)
	{
		uint8 RankA = (uint8)A.Classification;
		uint8 RankB = (uint8)B.Classification;
		if (RankA != RankB)
		{
			return RankA < RankB;
		}
		if (!A.RuleId.Equals(B.RuleId, ESearchCase::IgnoreCase))
		{
			return A.RuleId < B.RuleId;
		}
		return A.WidgetIdentity < B.WidgetIdentity;
	});

	// Deduplicate and record passed / not-evaluated rules
	for (const FString& PR : PassedRules)
	{
		if (!Report.PassedRuleIds.Contains(PR))
		{
			Report.PassedRuleIds.Add(PR);
		}
	}
	for (const FString& NER : NotEvaluatedRules)
	{
		if (!Report.NotEvaluatedRules.Contains(NER))
		{
			Report.NotEvaluatedRules.Add(NER);
		}
	}

	// Count classifications
	for (const FAuditFindingRecord& F : Report.Findings)
	{
		switch (F.Classification)
		{
		case EAuditFindingClassification::Error:        Report.ErrorCount++; break;
		case EAuditFindingClassification::Warning:      Report.WarningCount++; break;
		case EAuditFindingClassification::Info:         Report.InfoCount++; break;
		case EAuditFindingClassification::NotEvaluated: Report.NotEvaluatedCount++; break;
		default: break;
		}
	}
	Report.PassedCount = Report.PassedRuleIds.Num();

	// Explicit architectural limitations
	Report.Limitations.Add(TEXT("Observation is strictly non-mutating and passive. Unconstructed Slate widgets are never spawned."));
	Report.Limitations.Add(TEXT("Accessibility audit does not inspect the OS accessibility tree (UI Automation or IAccessible2)."));
	Report.Limitations.Add(TEXT("NVDA speech/braille output and screen-reader user experience were not verified; human review is required."));
	Report.Limitations.Add(TEXT("Screen bounds and geometry do not prove pointer routing, clipping, or hit-test-grid membership."));
	Report.Limitations.Add(TEXT("Focus presence was passively inspected via FSlateApplication and does not prove focus visual cues or human keyboard accessibility."));
	Report.Limitations.Add(TEXT("Rule evaluations do not prove or establish Slate crash causes."));

	// Formatting
	bool bWantsNvda = Format.Equals(TEXT("NvdaText"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);
	bool bWantsJson = Format.Equals(TEXT("Json"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);

	if (bWantsNvda)
	{
		Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatAuditReport(Report);
	}
	if (bWantsJson)
	{
		FString JsonStr;
		FJsonObjectConverter::UStructToJsonObjectString(Report, JsonStr, 0, 0, 0, nullptr, false);
		Report.JsonData = JsonStr;
	}

	return Report;
}

// ── Rule Implementations ───────────────────────────────────────────────

void FAccessibilityAuditEngine::EvaluateA11Y01(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-01");
	const FString RuleName = TEXT("HiddenButRuntimeExposed");

	// Applicable only with serialized and runtime evidence
	if (!S || !R || !C || !C->bSuccess)
	{
		OutNotEvaluated.Add(RuleId);
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::NotEvaluated;
		Rec.Message = TEXT("A11Y-01 requires both serialized Blueprint and live runtime evidence to evaluate hidden state divergence.");
		Rec.UnavailableEvidence.Add(TEXT("Combined serialized Blueprint and live runtime comparison evidence is unavailable."));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Unavailable;
		Rec.RemediationGuidance = TEXT("Run audit with both AssetPath and live runtime context.");
		Rec.Limitations.Add(TEXT("Requires differential comparison evidence."));
		OutFindings.Add(Rec);
		return;
	}

	bool bFoundViolation = false;

	for (const FWidgetComparisonRecord& CompRec : C->ComparisonRecords)
	{
		if (CompRec.Classification != EWidgetComparisonClassification::Matched)
		{
			continue;
		}

		// Look for serialized Collapsed or Hidden
		const FSerializedWidgetRecord* SRec = S->Widgets.FindByPredicate([&](const FSerializedWidgetRecord& W){ return W.NodeIndex == CompRec.SerializedNodeIndex; });
		const FRuntimeWidgetRecord* RRec = R->Widgets.FindByPredicate([&](const FRuntimeWidgetRecord& W){ return W.NodeIndex == CompRec.RuntimeNodeIndex; });

		if (!SRec || !RRec)
		{
			continue;
		}

		bool bSerializedHidden = SRec->DefaultVisibility.Equals(TEXT("Collapsed"), ESearchCase::IgnoreCase) ||
			SRec->DefaultVisibility.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase);

		// Rule requirement: Fire ONLY when directly matched widget is EFFECTIVELY exposed at runtime
		if (bSerializedHidden && RRec->bEffectivelyVisible)
		{
			// Check active screen or containment context
			bool bParentInactive = (RRec->BlockingAncestorIndex >= 0);
			if (!bParentInactive && RRec->ParentIndex >= 0 && RRec->ParentIndex < R->Widgets.Num())
			{
				const FRuntimeWidgetRecord& RParent = R->Widgets[RRec->ParentIndex];
				bParentInactive = !RParent.bEffectivelyVisible || !RParent.bIsEnabled;
			}

			if (bParentInactive)
			{
				bFoundViolation = true;
				FAuditFindingRecord Rec;
				Rec.RuleId = RuleId;
				Rec.RuleName = RuleName;
				Rec.Classification = EAuditFindingClassification::Error;
				Rec.Message = FString::Printf(TEXT("Control '%s' is authored as '%s' but exposed at runtime in inactive container '%s'."), *SRec->WidgetName, *SRec->DefaultVisibility, *RRec->ParentName);
				Rec.WidgetIdentity = SRec->WidgetName;
				Rec.SerializedEvidence.Add(FString::Printf(TEXT("DefaultVisibility: '%s'"), *SRec->DefaultVisibility));
				Rec.RuntimeEvidence.Add(FString::Printf(TEXT("EffectiveVisibility: '%s', Parent '%s' is inactive"), *RRec->EffectiveVisibility, *RRec->ParentName));
				Rec.ComparisonEvidence.Add(TEXT("Directly matched serialized to runtime node"));
				Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
				Rec.RemediationGuidance = TEXT("Ensure inactive container properly collapses all child controls at runtime.");
				Rec.Limitations.Add(TEXT("Does not verify hit-test routing."));
				OutFindings.Add(Rec);
			}
			else
			{
				// Screen or parent is active: was this a legitimate runtime screen activation?
				bool bLegitimateActivation = (RRec->ParentName.IsEmpty() || RRec->ParentName.Equals(TEXT("None"), ESearchCase::IgnoreCase) ||
					RRec->ParentIndex == 0 || (RRec->ParentIndex > 0 && R->Widgets[RRec->ParentIndex].bEffectivelyVisible));

				if (bLegitimateActivation)
				{
					// Legitimate runtime screen activation: suppress finding!
				}
				else
				{
					// Active context cannot be reliably proven: qualify as Warning
					bFoundViolation = true;
					FAuditFindingRecord Rec;
					Rec.RuleId = RuleId;
					Rec.RuleName = RuleName;
					Rec.Classification = EAuditFindingClassification::Warning;
					Rec.Message = FString::Printf(TEXT("Serialized '%s' control '%s' is effectively exposed at runtime without active-screen verification."), *SRec->DefaultVisibility, *SRec->WidgetName);
					Rec.WidgetIdentity = SRec->WidgetName;
					Rec.SerializedEvidence.Add(FString::Printf(TEXT("DefaultVisibility: '%s'"), *SRec->DefaultVisibility));
					Rec.RuntimeEvidence.Add(FString::Printf(TEXT("EffectiveVisibility: '%s'"), *RRec->EffectiveVisibility));
					Rec.ConfidenceBasis = EAuditConfidenceBasis::Derived;
					Rec.RemediationGuidance = TEXT("Verify that control exposure was intended by runtime screen activation logic.");
					Rec.Limitations.Add(TEXT("Active-screen context could not be authoritatively established."));
					OutFindings.Add(Rec);
				}
			}
		}
	}

	if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y02(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-02");
	const FString RuleName = TEXT("VisibleButUnnamed");

	// Requires runtime evidence
	if (!R || !R->bSuccess)
	{
		OutNotEvaluated.Add(RuleId);
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::NotEvaluated;
		Rec.Message = TEXT("A11Y-02 requires live runtime hierarchy evidence.");
		Rec.UnavailableEvidence.Add(TEXT("Live runtime hierarchy evidence is unavailable."));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Unavailable;
		Rec.RemediationGuidance = TEXT("Run audit with a live runtime UserWidget reference.");
		Rec.Limitations.Add(TEXT("Requires runtime widget evidence."));
		OutFindings.Add(Rec);
		return;
	}

	bool bFoundViolation = false;

	for (const FRuntimeWidgetRecord& W : R->Widgets)
	{
		// Evaluate only effectively visible and enabled controls
		if (!W.bEffectivelyVisible || !W.bIsEnabled)
		{
			continue;
		}

		// Do not apply to decorative or explicitly NotAccessible widgets
		if (W.bOverrideAccessibleDefaults && W.AccessibleBehavior.Equals(TEXT("NotAccessible"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		// Role must be directly established or safely inferred from a supported control class
		bool bInteractive = IsInteractiveClass(W.WidgetClass) || W.bEffectivelyFocusable || W.bIsFocusable || W.SlateWidgetType.Contains(TEXT("Button"));
		if (!bInteractive)
		{
			continue;
		}

		if (W.AccessibleName.IsEmpty())
		{
			bFoundViolation = true;
			FAuditFindingRecord Rec;
			Rec.RuleId = RuleId;
			Rec.RuleName = RuleName;
			Rec.Classification = EAuditFindingClassification::Error;
			Rec.Message = FString::Printf(TEXT("Visible, enabled interactive control '%s' (%s) has no effective accessible text."), *W.WidgetName, *W.WidgetClass);
			Rec.WidgetIdentity = W.WidgetName;
			Rec.RuntimeEvidence.Add(FString::Printf(TEXT("Class: '%s', EffectiveVisibility: '%s', Local: '%s', Enabled: %s, Slate: '%s'"),
				*W.WidgetClass, *W.EffectiveVisibility, *W.LocalVisibility, W.bIsEnabled ? TEXT("true") : TEXT("false"), *W.SlateWidgetType));
			Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
			Rec.RemediationGuidance = TEXT("Provide an accessible name in UMG Accessibility properties (Accessible Name) or assign clear text content.");
			Rec.Limitations.Add(TEXT("Does not inspect OS accessibility tree or NVDA output directly."));
			OutFindings.Add(Rec);
		}
	}

	if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y03(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-03");
	const FString RuleName = TEXT("FocusableButUnnamed");

	bool bEvaluated = false;
	bool bFoundViolation = false;

	if (R && R->bSuccess)
	{
		bEvaluated = true;
		for (const FRuntimeWidgetRecord& W : R->Widgets)
		{
			// Evaluate only effectively focusable, effectively visible, enabled controls
			if (!W.bEffectivelyFocusable || !W.bEffectivelyVisible || !W.bIsEnabled)
			{
				continue;
			}

			if (W.AccessibleName.IsEmpty())
			{
				bFoundViolation = true;
				FAuditFindingRecord Rec;
				Rec.RuleId = RuleId;
				Rec.RuleName = RuleName;
				Rec.Classification = EAuditFindingClassification::Error;
				Rec.Message = FString::Printf(TEXT("Focusable interactive control '%s' (%s) has no accessible name."), *W.WidgetName, *W.WidgetClass);
				Rec.WidgetIdentity = W.WidgetName;
				Rec.RuntimeEvidence.Add(FString::Printf(TEXT("Focusable: true, EffectiveFocusable: true, Enabled: true, EffectiveVisibility: '%s'"), *W.EffectiveVisibility));
				Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
				Rec.RemediationGuidance = TEXT("Assign an accessible name to this focusable control to allow screen readers to identify it upon receiving focus.");
				Rec.Limitations.Add(TEXT("Focus inspected passively via Slate properties; does not prove focus appearance visual cues."));
				OutFindings.Add(Rec);
			}
		}
	}
	else if (S && S->bSuccess)
	{
		bEvaluated = true;
		for (const FSerializedWidgetRecord& W : S->Widgets)
		{
			if (!W.bIsFocusable || !W.bIsEnabled)
			{
				continue;
			}

			if (W.DefaultVisibility.Equals(TEXT("Collapsed"), ESearchCase::IgnoreCase) || W.DefaultVisibility.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			if (W.AccessibleName.IsEmpty())
			{
				bFoundViolation = true;
				FAuditFindingRecord Rec;
				Rec.RuleId = RuleId;
				Rec.RuleName = RuleName;
				Rec.Classification = EAuditFindingClassification::Error;
				Rec.Message = FString::Printf(TEXT("Focusable serialized control '%s' (%s) has no accessible name."), *W.WidgetName, *W.WidgetClass);
				Rec.WidgetIdentity = W.WidgetName;
				Rec.SerializedEvidence.Add(FString::Printf(TEXT("Focusable: true, Enabled: true, DefaultVisibility: '%s'"), *W.DefaultVisibility));
				Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
				Rec.RemediationGuidance = TEXT("Assign an accessible name in Widget Blueprint Designer Accessibility properties.");
				Rec.Limitations.Add(TEXT("Static Blueprint inspection only; live Slate focus state not verified."));
				OutFindings.Add(Rec);
			}
		}
	}

	if (!bEvaluated)
	{
		OutNotEvaluated.Add(RuleId);
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::NotEvaluated;
		Rec.Message = TEXT("Focusability evidence is unavailable.");
		Rec.UnavailableEvidence.Add(TEXT("No focusability metadata found in input reports."));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Unavailable;
		Rec.RemediationGuidance = TEXT("Supply serialized asset or live runtime widget containing focusable controls.");
		Rec.Limitations.Add(TEXT("Requires focusability evidence."));
		OutFindings.Add(Rec);
	}
	else if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y04(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-04");
	const FString RuleName = TEXT("DuplicateAccessibleName");

	struct FDuplicateCandidate
	{
		FString WidgetName;
		FString WidgetClass;
		FString ParentName;
		FString AccessibleSummary;
		bool bIsActive;
	};

	TMap<FString, TArray<FDuplicateCandidate>> NameGroups;

	if (R && R->bSuccess)
	{
		for (const FRuntimeWidgetRecord& W : R->Widgets)
		{
			if (W.AccessibleName.IsEmpty())
			{
				continue;
			}
			// Only include controls that are effectively visible and in the active screen context (unblocked by an inactive ancestor)
			if (!W.bEffectivelyVisible || W.BlockingAncestorIndex >= 0)
			{
				continue;
			}
			bool bActive = W.bIsEnabled;
			FDuplicateCandidate Cand;
			Cand.WidgetName = W.WidgetName;
			Cand.WidgetClass = W.WidgetClass;
			Cand.ParentName = W.ParentName;
			Cand.AccessibleSummary = W.AccessibleSummary;
			Cand.bIsActive = bActive;
			NameGroups.FindOrAdd(W.AccessibleName.ToLower()).Add(Cand);
		}
	}
	else if (S && S->bSuccess)
	{
		for (const FSerializedWidgetRecord& W : S->Widgets)
		{
			if (W.AccessibleName.IsEmpty())
			{
				continue;
			}
			bool bActive = !W.DefaultVisibility.Equals(TEXT("Collapsed"), ESearchCase::IgnoreCase) &&
				!W.DefaultVisibility.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase) && W.bIsEnabled;
			FDuplicateCandidate Cand;
			Cand.WidgetName = W.WidgetName;
			Cand.WidgetClass = W.WidgetClass;
			Cand.ParentName = W.ParentName;
			Cand.AccessibleSummary = W.AccessibleSummary;
			Cand.bIsActive = bActive;
			NameGroups.FindOrAdd(W.AccessibleName.ToLower()).Add(Cand);
		}
	}
	else
	{
		OutNotEvaluated.Add(RuleId);
		return;
	}

	bool bFoundViolation = false;

	for (auto& Pair : NameGroups)
	{
		const TArray<FDuplicateCandidate>& Group = Pair.Value;
		if (Group.Num() <= 1)
		{
			continue;
		}

		// Check if multiple active equivalent interactive controls share identical name
		int32 ActiveCount = 0;
		bool bSameRole = true;
		bool bIdenticalSummary = true;
		FString FirstClass = Group[0].WidgetClass;
		FString FirstSummary = Group[0].AccessibleSummary;

		for (const FDuplicateCandidate& Cand : Group)
		{
			if (Cand.bIsActive)
			{
				ActiveCount++;
			}
			if (!Cand.WidgetClass.Equals(FirstClass, ESearchCase::IgnoreCase))
			{
				bSameRole = false;
			}
			if (!Cand.AccessibleSummary.Equals(FirstSummary, ESearchCase::IgnoreCase))
			{
				bIdenticalSummary = false;
			}
		}

		// Summaries or distinct roles may disambiguate duplicate names
		if (!bSameRole || (!bIdenticalSummary && !FirstSummary.IsEmpty()))
		{
			// Disambiguated by role or summary -> suppressed or low severity
			continue;
		}

		bFoundViolation = true;
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.WidgetIdentity = Group[0].WidgetName;

		TArray<FString> Names;
		for (const FDuplicateCandidate& C_Cand : Group)
		{
			Names.Add(C_Cand.WidgetName);
		}

		if (ActiveCount > 1 && bSameRole && bIdenticalSummary)
		{
			// Escalate to Error only when duplicate names occur among simultaneously active, equivalent interactive controls and create demonstrated ambiguity
			Rec.Classification = EAuditFindingClassification::Error;
			Rec.Message = FString::Printf(TEXT("Duplicate accessible name \"%s\" shared by %d simultaneously active, equivalent '%s' controls [%s] creating demonstrated ambiguity."),
				*Pair.Key, ActiveCount, *FirstClass, *FString::Join(Names, TEXT(", ")));
			Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
			Rec.RuntimeEvidence.Add(FString::Printf(TEXT("Simultaneously active controls count: %d, Equivalent class: '%s', Demonstrable ambiguity: true"), ActiveCount, *FirstClass));
		}
		else
		{
			Rec.Classification = EAuditFindingClassification::Warning;
			Rec.Message = FString::Printf(TEXT("Duplicate accessible name \"%s\" shared by %d controls [%s]."),
				*Pair.Key, Group.Num(), *FString::Join(Names, TEXT(", ")));
			Rec.ConfidenceBasis = EAuditConfidenceBasis::Derived;
		}

		Rec.RemediationGuidance = TEXT("Differentiate duplicate control names by assigning distinct AccessibleName labels or descriptive AccessibleSummary attributes.");
		Rec.Limitations.Add(TEXT("Does not compare against Unreal Editor chrome."));
		OutFindings.Add(Rec);
	}

	if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y05(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-05");
	const FString RuleName = TEXT("InteractiveChildOutsidePanel");

	if (!C || !C->bSuccess || !S || !R)
	{
		OutNotEvaluated.Add(RuleId);
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::NotEvaluated;
		Rec.Message = TEXT("Containment evidence is unavailable.");
		Rec.UnavailableEvidence.Add(TEXT("Requires differential containment evidence."));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Unavailable;
		Rec.RemediationGuidance = TEXT("Provide both serialized and runtime reports to evaluate containment.");
		Rec.Limitations.Add(TEXT("Requires authoritative containment hierarchy."));
		OutFindings.Add(Rec);
		return;
	}

	bool bFoundViolation = false;

	for (const FWidgetComparisonRecord& Rec : C->ComparisonRecords)
	{
		if (Rec.Classification != EWidgetComparisonClassification::Matched)
		{
			continue;
		}

		// Only check interactive controls
		if (!IsInteractiveClass(Rec.SerializedClass))
		{
			continue;
		}

		// Reliable containment evidence check: check if parent mismatch discrepancy exists
		bool bHasParentMismatch = false;
		for (const FString& Disc : Rec.Discrepancies)
		{
			if (Disc.Contains(TEXT("parent or containment mismatch")))
			{
				bHasParentMismatch = true;
				break;
			}
		}

		// Require reliable containment evidence: both parent names must be explicit and known
		if (bHasParentMismatch && !Rec.SerializedParent.IsEmpty() && !Rec.RuntimeParent.IsEmpty() &&
			!Rec.SerializedParent.Equals(TEXT("None"), ESearchCase::IgnoreCase) && !Rec.RuntimeParent.Equals(TEXT("None"), ESearchCase::IgnoreCase))
		{
			bFoundViolation = true;
			FAuditFindingRecord Finding;
			Finding.RuleId = RuleId;
			Finding.RuleName = RuleName;
			Finding.Classification = EAuditFindingClassification::Warning;
			Finding.Message = FString::Printf(TEXT("Interactive child '%s' exists under runtime container '%s' instead of authored serialized container '%s'."),
				*Rec.SerializedWidgetName, *Rec.RuntimeParent, *Rec.SerializedParent);
			Finding.WidgetIdentity = Rec.SerializedWidgetName;
			Finding.SerializedEvidence.Add(FString::Printf(TEXT("Authored Parent: '%s'"), *Rec.SerializedParent));
			Finding.RuntimeEvidence.Add(FString::Printf(TEXT("Runtime Parent: '%s'"), *Rec.RuntimeParent));
			Finding.ComparisonEvidence.Add(TEXT("Direct parent discrepancy logged in differential comparison"));
			Finding.ConfidenceBasis = EAuditConfidenceBasis::Direct;
			Finding.RemediationGuidance = TEXT("Verify that widget reparenting preserves the intended accessible panel containment structure.");
			Finding.Limitations.Add(TEXT("Does not infer expected panels from naming conventions alone."));
			OutFindings.Add(Finding);
		}
	}

	if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y06(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-06");
	const FString RuleName = TEXT("CollapsedScreenRuntimeDescendant");

	if (!R || !R->bSuccess)
	{
		OutNotEvaluated.Add(RuleId);
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::NotEvaluated;
		Rec.Message = TEXT("Active screen evidence is unavailable.");
		Rec.UnavailableEvidence.Add(TEXT("Live runtime hierarchy evidence is required."));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Unavailable;
		Rec.RemediationGuidance = TEXT("Provide live runtime hierarchy to check for collapsed screen descendants.");
		Rec.Limitations.Add(TEXT("Requires runtime container hierarchy."));
		OutFindings.Add(Rec);
		return;
	}

	bool bFoundViolation = false;

	// Search for containers that are Collapsed or Hidden
	for (const FRuntimeWidgetRecord& Container : R->Widgets)
	{
		bool bContainerCollapsed = Container.LocalVisibility.Equals(TEXT("Collapsed"), ESearchCase::IgnoreCase) ||
			Container.LocalVisibility.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase) ||
			Container.EffectiveVisibility.Equals(TEXT("Collapsed"), ESearchCase::IgnoreCase) ||
			Container.EffectiveVisibility.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase);

		if (!bContainerCollapsed || Container.ChildIndices.Num() == 0)
		{
			continue;
		}

		// Traverse all descendants of this collapsed container
		TArray<int32> Queue = Container.ChildIndices;
		while (Queue.Num() > 0)
		{
			int32 ChildIdx = Queue.Pop();
			if (ChildIdx >= 0 && ChildIdx < R->Widgets.Num())
			{
				const FRuntimeWidgetRecord& Child = R->Widgets[ChildIdx];
				// Rule requirement: Do not fire when a child is locally Visible under an effectively Collapsed or Hidden ancestor.
				// Fire ONLY if runtime evidence establishes that the descendant escapes ancestor suppression:
				// e.g. it has active focus, or it is effectively visible, or live Slate widget is visible while container is not.
				bool bEscapesSuppression = Child.bEffectivelyVisible ||
					Child.bHasKeyboardFocus || Child.bHasUserFocus ||
					(Child.bHasSlateWidget && Child.bSlateVisible && !Container.bSlateVisible);

				if (bEscapesSuppression)
				{
					bFoundViolation = true;
					FAuditFindingRecord Rec;
					Rec.RuleId = RuleId;
					Rec.RuleName = RuleName;
					Rec.Classification = EAuditFindingClassification::Error;
					Rec.Message = FString::Printf(TEXT("Descendant '%s' of collapsed or inactive container '%s' escapes ancestor suppression at runtime."),
						*Child.WidgetName, *Container.WidgetName);
					Rec.WidgetIdentity = Child.WidgetName;
					Rec.RuntimeEvidence.Add(FString::Printf(TEXT("Container '%s' effective: '%s', Descendant '%s' effective: '%s', Focus: %s"),
						*Container.WidgetName, *Container.EffectiveVisibility, *Child.WidgetName, *Child.EffectiveVisibility,
						(Child.bHasKeyboardFocus || Child.bHasUserFocus) ? TEXT("true") : TEXT("false")));
					Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
					Rec.RemediationGuidance = TEXT("Ensure inactive or collapsed screen containers collapse their full child hierarchy.");
					Rec.Limitations.Add(TEXT("Does not verify hit-test routing."));
					OutFindings.Add(Rec);
				}
				for (int32 NextChild : Child.ChildIndices)
				{
					Queue.Add(NextChild);
				}
			}
		}
	}

	if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y07(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-07");
	const FString RuleName = TEXT("FocusOnInactiveScreen");

	if (!R || !R->bSuccess)
	{
		OutNotEvaluated.Add(RuleId);
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::NotEvaluated;
		Rec.Message = TEXT("Passive runtime focus evidence is unavailable.");
		Rec.UnavailableEvidence.Add(TEXT("Live runtime hierarchy with FSlateApplication focus is required."));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Unavailable;
		Rec.RemediationGuidance = TEXT("Run audit in active PIE or Game world.");
		Rec.Limitations.Add(TEXT("Focus is checked passively; never inferred from hover, bounds, or navigation targets."));
		OutFindings.Add(Rec);
		return;
	}

	bool bFoundViolation = false;

	// Find any widget with passive runtime focus
	for (const FRuntimeWidgetRecord& W : R->Widgets)
	{
		if (!W.bHasKeyboardFocus && !W.bHasUserFocus)
		{
			continue;
		}

		// Check if the focused widget is inside a collapsed or inactive container
		bool bInsideInactiveContainer = (W.BlockingAncestorIndex >= 0);
		FString InactiveAncestorName = W.BlockingAncestorName;

		if (!bInsideInactiveContainer)
		{
			int32 ParentIdx = W.ParentIndex;
			while (ParentIdx >= 0 && ParentIdx < R->Widgets.Num())
			{
				const FRuntimeWidgetRecord& Parent = R->Widgets[ParentIdx];
				if (!Parent.bEffectivelyVisible ||
					Parent.EffectiveVisibility.Equals(TEXT("Collapsed"), ESearchCase::IgnoreCase) ||
					Parent.EffectiveVisibility.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase) ||
					Parent.LocalVisibility.Equals(TEXT("Collapsed"), ESearchCase::IgnoreCase) ||
					Parent.LocalVisibility.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase) ||
					Parent.RuntimeVisibility.Equals(TEXT("Collapsed"), ESearchCase::IgnoreCase) ||
					Parent.RuntimeVisibility.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase))
				{
					bInsideInactiveContainer = true;
					InactiveAncestorName = Parent.WidgetName;
					break;
				}
				ParentIdx = Parent.ParentIndex;
			}
		}

		if (bInsideInactiveContainer)
		{
			bFoundViolation = true;
			FAuditFindingRecord Rec;
			Rec.RuleId = RuleId;
			Rec.RuleName = RuleName;
			Rec.Classification = EAuditFindingClassification::Error;
			Rec.Message = FString::Printf(TEXT("Passive runtime focus is directly established on control '%s' inside inactive screen '%s'."),
				*W.WidgetName, *InactiveAncestorName);
			Rec.WidgetIdentity = W.WidgetName;
			Rec.RuntimeEvidence.Add(FString::Printf(TEXT("Control '%s' has keyboard focus (%s) inside inactive ancestor '%s' (effective: '%s')"),
				*W.WidgetName, W.bHasKeyboardFocus ? TEXT("true") : TEXT("false"), *InactiveAncestorName, *W.EffectiveVisibility));
			Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
			Rec.RemediationGuidance = TEXT("Clear or redirect focus away from controls inside inactive screens when transitioning away.");
			Rec.Limitations.Add(TEXT("Do not infer focus from navigation targets, accessible text, hover, bounds, or serialized defaults."));
			OutFindings.Add(Rec);
		}
	}

	if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y08(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-08");
	const FString RuleName = TEXT("NavigationTargetOutsideScreen");

	// Inspect explicit navigation targets in serialized or runtime
	const TArray<FSerializedWidgetRecord>* SWidgets = S ? &S->Widgets : nullptr;
	if (!SWidgets || SWidgets->Num() == 0)
	{
		OutNotEvaluated.Add(RuleId);
		return;
	}

	bool bFoundViolation = false;
	bool bHasExplicitTargets = false;

	for (const FSerializedWidgetRecord& W : *SWidgets)
	{
		for (const FAccessibleNavigationRecord& Nav : W.NavigationRules)
		{
			// Navigation None or automatic is not defective
			if (Nav.Rule.Equals(TEXT("None"), ESearchCase::IgnoreCase) ||
				Nav.Rule.Equals(TEXT("Escape"), ESearchCase::IgnoreCase) ||
				Nav.Rule.Equals(TEXT("Stop"), ESearchCase::IgnoreCase) ||
				Nav.Rule.Equals(TEXT("Wrap"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			if (Nav.Rule.Equals(TEXT("Explicit"), ESearchCase::IgnoreCase) && !Nav.TargetWidget.IsEmpty())
			{
				bHasExplicitTargets = true;

				// Verify target exists in this widget tree
				bool bTargetFoundInTree = false;
				for (const FSerializedWidgetRecord& TargetW : *SWidgets)
				{
					if (TargetW.WidgetName.Equals(Nav.TargetWidget, ESearchCase::IgnoreCase))
					{
						bTargetFoundInTree = true;
						break;
					}
				}

				if (!bTargetFoundInTree)
				{
					bFoundViolation = true;
					FAuditFindingRecord Rec;
					Rec.RuleId = RuleId;
					Rec.RuleName = RuleName;
					Rec.Classification = EAuditFindingClassification::Warning;
					Rec.Message = FString::Printf(TEXT("Explicit navigation rule on '%s' targets widget '%s' located outside the active screen."),
						*W.WidgetName, *Nav.TargetWidget);
					Rec.WidgetIdentity = W.WidgetName;
					Rec.SerializedEvidence.Add(FString::Printf(TEXT("Direction: '%s', Rule: '%s', TargetWidget: '%s'"), *Nav.Direction, *Nav.Rule, *Nav.TargetWidget));
					Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
					Rec.RemediationGuidance = TEXT("Re-target directional navigation rules to controls within the active screen or use automatic navigation.");
					Rec.Limitations.Add(TEXT("Do not treat Navigation None as automatically defective."));
					OutFindings.Add(Rec);
				}
			}
		}
	}

	if (!bHasExplicitTargets)
	{
		OutPassed.Add(RuleId);
	}
	else if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y09(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-09");
	const FString RuleName = TEXT("ModalContainmentFailure");

	if (!R || !R->bSuccess)
	{
		OutNotEvaluated.Add(RuleId);
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::NotEvaluated;
		Rec.Message = TEXT("Modal state evidence is unavailable.");
		Rec.UnavailableEvidence.Add(TEXT("Live runtime hierarchy evidence is required."));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Unavailable;
		Rec.RemediationGuidance = TEXT("Provide runtime report with active modal dialog.");
		Rec.Limitations.Add(TEXT("Bounds overlap alone does not prove containment failure."));
		OutFindings.Add(Rec);
		return;
	}

	// Look for an established, effectively visible modal container
	const FRuntimeWidgetRecord* ModalWidget = nullptr;
	for (const FRuntimeWidgetRecord& W : R->Widgets)
	{
		if (W.WidgetClass.Contains(TEXT("Modal"), ESearchCase::IgnoreCase) ||
			W.WidgetName.Contains(TEXT("Modal"), ESearchCase::IgnoreCase) ||
			W.WidgetName.Contains(TEXT("Dialog"), ESearchCase::IgnoreCase))
		{
			// Modal must be effectively visible or active without blocking ancestor
			bool bModalActive = W.bEffectivelyVisible ||
				((W.RuntimeVisibility.Equals(TEXT("Visible"), ESearchCase::IgnoreCase) || W.LocalVisibility.Equals(TEXT("Visible"), ESearchCase::IgnoreCase)) && W.BlockingAncestorIndex <= 0);
			if (bModalActive)
			{
				ModalWidget = &W;
				break;
			}
		}
	}

	if (!ModalWidget)
	{
		// No active modal established -> pass
		OutPassed.Add(RuleId);
		return;
	}

	bool bFoundViolation = false;

	// Check if any non-modal control has focus or remains effectively interactive behind active modal
	for (const FRuntimeWidgetRecord& W : R->Widgets)
	{
		if (W.NodeIndex == ModalWidget->NodeIndex)
		{
			continue;
		}

		// Check if W is a descendant of the modal
		bool bIsModalDescendant = false;
		int32 ParentIdx = W.ParentIndex;
		while (ParentIdx >= 0 && ParentIdx < R->Widgets.Num())
		{
			if (ParentIdx == ModalWidget->NodeIndex)
			{
				bIsModalDescendant = true;
				break;
			}
			ParentIdx = R->Widgets[ParentIdx].ParentIndex;
		}

		if (bIsModalDescendant)
		{
			continue;
		}

		// Underlying widget outside modal: check if it has direct focus or effective interaction
		// Rule requirement: Local visibility and overlapping bounds are insufficient; require direct focus or effective interaction
		if (W.bHasKeyboardFocus || W.bHasUserFocus)
		{
			bFoundViolation = true;
			FAuditFindingRecord Rec;
			Rec.RuleId = RuleId;
			Rec.RuleName = RuleName;
			Rec.Classification = EAuditFindingClassification::Error;
			Rec.Message = FString::Printf(TEXT("Modal '%s' is active but underlying control '%s' retains active focus."),
				*ModalWidget->WidgetName, *W.WidgetName);
			Rec.WidgetIdentity = W.WidgetName;
			Rec.RuntimeEvidence.Add(FString::Printf(TEXT("Modal '%s' is effectively visible, underlying control '%s' has keyboard focus"), *ModalWidget->WidgetName, *W.WidgetName));
			Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
			Rec.RemediationGuidance = TEXT("Trap focus within the active modal and disable or collapse underlying interactive controls.");
			Rec.Limitations.Add(TEXT("Do not claim hit-testability without hit-test-grid evidence."));
			OutFindings.Add(Rec);
		}
	}

	if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y10(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-10");
	const FString RuleName = TEXT("VisibilityStateMismatch");

	if (!C || !C->bSuccess)
	{
		OutNotEvaluated.Add(RuleId);
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::NotEvaluated;
		Rec.Message = TEXT("A11Y-10 requires differential comparison evidence.");
		Rec.UnavailableEvidence.Add(TEXT("Differential comparison report is unavailable."));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Unavailable;
		Rec.RemediationGuidance = TEXT("Supply both serialized and runtime reports.");
		Rec.Limitations.Add(TEXT("Report as descriptive evidence, not crash-cause proof."));
		OutFindings.Add(Rec);
		return;
	}

	bool bFoundViolation = false;

	for (const FWidgetComparisonRecord& Rec : C->ComparisonRecords)
	{
		if (Rec.Classification != EWidgetComparisonClassification::Matched)
		{
			continue;
		}

		for (const FString& Disc : Rec.Discrepancies)
		{
			if (Disc.Contains(TEXT("visibility mismatch")))
			{
				// If runtime widget record exists and is inside an inactive container, suppress
				if (R)
				{
					const FRuntimeWidgetRecord* RRec = R->Widgets.FindByPredicate([&](const FRuntimeWidgetRecord& W){ return W.NodeIndex == Rec.RuntimeNodeIndex; });
					if (RRec && RRec->BlockingAncestorIndex >= 0)
					{
						continue;
					}
				}

				bFoundViolation = true;
				FAuditFindingRecord Finding;
				Finding.RuleId = RuleId;
				Finding.RuleName = RuleName;
				Finding.Classification = EAuditFindingClassification::Warning;
				Finding.Message = FString::Printf(TEXT("Unexplained visibility mismatch on '%s' (%s)."), *Rec.SerializedWidgetName, *Disc);
				Finding.WidgetIdentity = Rec.SerializedWidgetName;
				Finding.ComparisonEvidence.Add(Disc);
				Finding.ConfidenceBasis = EAuditConfidenceBasis::Derived;
				Finding.RemediationGuidance = TEXT("Confirm that visibility change is driven by an intentional gameplay state transition.");
				Finding.Limitations.Add(TEXT("Report as descriptive evidence, not crash-cause proof."));
				OutFindings.Add(Finding);
			}
		}
	}

	if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y11(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-11");
	const FString RuleName = TEXT("OverlappingInteractiveBounds");

	if (!R || !R->bSuccess)
	{
		OutNotEvaluated.Add(RuleId);
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::NotEvaluated;
		Rec.Message = TEXT("Geometry evidence is unavailable.");
		Rec.UnavailableEvidence.Add(TEXT("Live runtime hierarchy with Slate geometry is required."));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Unavailable;
		Rec.RemediationGuidance = TEXT("Run audit against live viewport with constructed Slate widgets.");
		Rec.Limitations.Add(TEXT("Geometry does not prove pointer routing or hit-test-grid membership."));
		OutFindings.Add(Rec);
		return;
	}

	// Filter effectively visible, enabled interactive widgets with valid non-zero geometry in the active context
	TArray<const FRuntimeWidgetRecord*> InteractiveWidgets;
	for (const FRuntimeWidgetRecord& W : R->Widgets)
	{
		if (!W.bEffectivelyVisible || !W.bIsEnabled || !W.bEffectivelyInteractive || !W.bEffectiveGeometryAvailable)
		{
			continue;
		}

		if (W.BlockingAncestorIndex >= 0)
		{
			continue;
		}

		if (!W.ScreenBounds.bIsValid || W.ScreenBounds.Width <= 0.0f || W.ScreenBounds.Height <= 0.0f)
		{
			continue;
		}

		if (IsInteractiveClass(W.WidgetClass) || W.bEffectivelyFocusable || W.bIsFocusable || W.SlateWidgetType.Contains(TEXT("Button")))
		{
			InteractiveWidgets.Add(&W);
		}
	}

	bool bFoundViolation = false;

	for (int32 i = 0; i < InteractiveWidgets.Num(); ++i)
	{
		for (int32 j = i + 1; j < InteractiveWidgets.Num(); ++j)
		{
			const FRuntimeWidgetRecord& A = *InteractiveWidgets[i];
			const FRuntimeWidgetRecord& B = *InteractiveWidgets[j];

			// Exclude parent-child composition
			if (A.NodeIndex == B.ParentIndex || B.NodeIndex == A.ParentIndex)
			{
				continue;
			}

			// Calculate intersection
			float MinX = FMath::Max(A.ScreenBounds.X, B.ScreenBounds.X);
			float MaxX = FMath::Min(A.ScreenBounds.X + A.ScreenBounds.Width, B.ScreenBounds.X + B.ScreenBounds.Width);
			float MinY = FMath::Max(A.ScreenBounds.Y, B.ScreenBounds.Y);
			float MaxY = FMath::Min(A.ScreenBounds.Y + A.ScreenBounds.Height, B.ScreenBounds.Y + B.ScreenBounds.Height);

			if (MaxX > MinX && MaxY > MinY)
			{
				float InterW = MaxX - MinX;
				float InterH = MaxY - MinY;
				float InterArea = InterW * InterH;
				float AreaA = A.ScreenBounds.Width * A.ScreenBounds.Height;
				float AreaB = B.ScreenBounds.Width * B.ScreenBounds.Height;
				float MinArea = FMath::Min(AreaA, AreaB);

				// Overlap threshold: intersection width & height > 4.0 and > 10% overlap
				if (InterW > 4.0f && InterH > 4.0f && MinArea > 0.0f && (InterArea / MinArea) > 0.10f)
				{
					bFoundViolation = true;
					FAuditFindingRecord Rec;
					Rec.RuleId = RuleId;
					Rec.RuleName = RuleName;
					Rec.Classification = EAuditFindingClassification::Warning;
					Rec.Message = FString::Printf(TEXT("Interactive controls '%s' and '%s' overlap by (%.1f, %.1f) screen units."),
						*A.WidgetName, *B.WidgetName, InterW, InterH);
					Rec.WidgetIdentity = A.WidgetName;
					Rec.RuntimeEvidence.Add(FString::Printf(TEXT("A: '%s' bounds (%.1f, %.1f, %.1f, %.1f), B: '%s' bounds (%.1f, %.1f, %.1f, %.1f)"),
						*A.WidgetName, A.ScreenBounds.X, A.ScreenBounds.Y, A.ScreenBounds.Width, A.ScreenBounds.Height,
						*B.WidgetName, B.ScreenBounds.X, B.ScreenBounds.Y, B.ScreenBounds.Width, B.ScreenBounds.Height));
					Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
					Rec.RemediationGuidance = TEXT("Adjust slot layout padding, anchors, or alignment to prevent interactive control overlap.");
					Rec.Limitations.Add(TEXT("Geometry does not prove pointer routing or hit-test-grid membership."));
					OutFindings.Add(Rec);
				}
			}
		}
	}

	if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y12(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-12");
	const FString RuleName = TEXT("MissingActionBinding");

	// Requires binding evidence directly from Serialized report
	if (!S || !S->bSuccess)
	{
		OutNotEvaluated.Add(RuleId);
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::NotEvaluated;
		Rec.Message = TEXT("Action binding evidence is unavailable.");
		Rec.UnavailableEvidence.Add(TEXT("Serialized Widget Blueprint binding introspection is required."));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Unavailable;
		Rec.RemediationGuidance = TEXT("Supply AssetPath to inspect Blueprint event bindings.");
		Rec.Limitations.Add(TEXT("Return NotEvaluated where binding evidence is incomplete."));
		OutFindings.Add(Rec);
		return;
	}

	bool bFoundViolation = false;

	for (const FSerializedWidgetRecord& W : S->Widgets)
	{
		if (!W.bIsEnabled)
		{
			continue;
		}

		// Action controls: Buttons
		if (W.WidgetClass.Equals(TEXT("Button"), ESearchCase::IgnoreCase))
		{
			if (W.Bindings.Num() == 0)
			{
				bFoundViolation = true;
				FAuditFindingRecord Rec;
				Rec.RuleId = RuleId;
				Rec.RuleName = RuleName;
				Rec.Classification = EAuditFindingClassification::Warning;
				Rec.Message = FString::Printf(TEXT("Enabled action control '%s' has no delegate or Blueprint bindings."), *W.WidgetName);
				Rec.WidgetIdentity = W.WidgetName;
				Rec.SerializedEvidence.Add(FString::Printf(TEXT("Class: '%s', Enabled: true, Bindings: 0"), *W.WidgetClass));
				Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
				Rec.RemediationGuidance = TEXT("Bind an OnClicked or action event handler to this button.");
				Rec.Limitations.Add(TEXT("Do not infer intended action solely from class or name."));
				OutFindings.Add(Rec);
			}
		}
	}

	if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y13(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-13");
	const FString RuleName = TEXT("AccessibleChildLeak");

	// Requires runtime evidence
	if (!R || !R->bSuccess)
	{
		OutNotEvaluated.Add(RuleId);
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::NotEvaluated;
		Rec.Message = TEXT("Runtime accessibility hierarchy evidence is unavailable.");
		Rec.UnavailableEvidence.Add(TEXT("Requires live runtime hierarchy evidence."));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Unavailable;
		Rec.RemediationGuidance = TEXT("Provide live runtime UserWidget reference.");
		Rec.Limitations.Add(TEXT("Do not claim equivalence with OS accessibility tree or NVDA output."));
		OutFindings.Add(Rec);
		return;
	}

	bool bFoundViolation = false;

	// Check if parent directly disallows accessible children
	for (const FRuntimeWidgetRecord& Parent : R->Widgets)
	{
		// Parent must be effectively visible to have exposed children
		if (!Parent.bEffectivelyVisible)
		{
			continue;
		}

		bool bDisallowsChildren = Parent.bOverrideAccessibleDefaults &&
			Parent.AccessibleBehavior.Equals(TEXT("NotAccessible"), ESearchCase::IgnoreCase);

		if (!bDisallowsChildren)
		{
			continue;
		}

		for (int32 ChildIdx : Parent.ChildIndices)
		{
			if (ChildIdx >= 0 && ChildIdx < R->Widgets.Num())
			{
				const FRuntimeWidgetRecord& Child = R->Widgets[ChildIdx];
				// Rule requirement: Do not claim a leak merely because a child contains accessible text.
				// Require supported runtime exposure evidence: child must be effectively visible and have a live Slate widget
				bool bChildExposed = Child.bEffectivelyVisible && Child.bHasSlateWidget &&
					(!Child.AccessibleName.IsEmpty() ||
					(Child.bOverrideAccessibleDefaults && Child.AccessibleBehavior.Equals(TEXT("Custom"), ESearchCase::IgnoreCase)));

				if (bChildExposed)
				{
					bFoundViolation = true;
					FAuditFindingRecord Rec;
					Rec.RuleId = RuleId;
					Rec.RuleName = RuleName;
					Rec.Classification = EAuditFindingClassification::Error;
					Rec.Message = FString::Printf(TEXT("Parent '%s' directly disallows accessible children, but effectively visible child '%s' is independently exposed with accessible text \"%s\"."),
						*Parent.WidgetName, *Child.WidgetName, *Child.AccessibleName);
					Rec.WidgetIdentity = Child.WidgetName;
					Rec.RuntimeEvidence.Add(FString::Printf(TEXT("Parent '%s' AccessibleBehavior: '%s', Child '%s' AccessibleName: \"%s\", Effective: '%s'"),
						*Parent.WidgetName, *Parent.AccessibleBehavior, *Child.WidgetName, *Child.AccessibleName, *Child.EffectiveVisibility));
					Rec.ConfidenceBasis = EAuditConfidenceBasis::Direct;
					Rec.RemediationGuidance = TEXT("Set child accessible behavior to NotAccessible or remove parent disallowance.");
					Rec.Limitations.Add(TEXT("Do not claim equivalence with the operating system accessibility tree or NVDA output."));
					OutFindings.Add(Rec);
				}
			}
		}
	}

	if (!bFoundViolation)
	{
		OutPassed.Add(RuleId);
	}
}

void FAccessibilityAuditEngine::EvaluateA11Y14(
	const FSerializedWidgetBlueprintReport* S,
	const FRuntimeWidgetReport* R,
	const FSerializedRuntimeComparisonReport* C,
	TArray<FAuditFindingRecord>& OutFindings,
	TArray<FString>& OutPassed,
	TArray<FString>& OutNotEvaluated)
{
	const FString RuleId = TEXT("A11Y-14");
	const FString RuleName = TEXT("AmbiguousRuntimeIdentity");

	bool bHasAmbiguity = false;

	if (C && C->AmbiguousCount > 0)
	{
		bHasAmbiguity = true;
		for (const FWidgetComparisonRecord& CompRec : C->ComparisonRecords)
		{
			if (CompRec.Classification == EWidgetComparisonClassification::Ambiguous)
			{
				FAuditFindingRecord Rec;
				Rec.RuleId = RuleId;
				Rec.RuleName = RuleName;
				Rec.Classification = EAuditFindingClassification::Warning;
				Rec.Message = FString::Printf(TEXT("Ambiguous runtime identity for widget '%s'; multiple candidates could not be uniquely resolved."),
					*CompRec.SerializedWidgetName);
				Rec.WidgetIdentity = CompRec.SerializedWidgetName;
				Rec.ComparisonEvidence.Add(TEXT("Ambiguous candidate classification in differential report"));
				Rec.ConfidenceBasis = EAuditConfidenceBasis::Ambiguous;
				Rec.RemediationGuidance = TEXT("Assign unique widget identifiers or parent container scopes to allow deterministic accessibility audit.");
				Rec.Limitations.Add(TEXT("This rule should steer human review, not accuse the asset of a structural defect."));
				OutFindings.Add(Rec);
			}
		}
	}
	else if (R && (R->Availability == EInspectionAvailability::Ambiguous || R->Candidates.Num() > 1))
	{
		bHasAmbiguity = true;
		FAuditFindingRecord Rec;
		Rec.RuleId = RuleId;
		Rec.RuleName = RuleName;
		Rec.Classification = EAuditFindingClassification::Warning;
		Rec.Message = FString::Printf(TEXT("Ambiguous top-level runtime widget selection; %d candidate widgets found."), R->Candidates.Num());
		Rec.WidgetIdentity = R->RootWidgetName;
		Rec.RuntimeEvidence.Add(FString::Printf(TEXT("Candidates: [%s]"), *FString::Join(R->Candidates, TEXT(", "))));
		Rec.ConfidenceBasis = EAuditConfidenceBasis::Ambiguous;
		Rec.RemediationGuidance = TEXT("Specify an explicit RuntimeWidgetReference object path to target a specific user widget.");
		Rec.Limitations.Add(TEXT("This rule should steer human review, not accuse the asset of a structural defect."));
		OutFindings.Add(Rec);
	}

	if (!bHasAmbiguity)
	{
		OutPassed.Add(RuleId);
	}
}
