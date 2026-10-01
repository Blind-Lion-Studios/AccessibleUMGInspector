// Copyright Blind Lion Studios. All Rights Reserved.

#include "NvdaTextFormatter.h"

FString FAccessibleUMGNvdaTextFormatter::FormatSerializedReport(const FSerializedWidgetBlueprintReport& Report)
{
	TStringBuilder<4096> SB;

	if (!Report.bSuccess)
	{
		SB.Appendf(TEXT("Accessible UMG Inspector Error: %s\n"), *Report.ErrorMessage);
		SB.Appendf(TEXT("Target Asset: %s\n"), *Report.AssetPath);
		return SB.ToString();
	}

	int32 TotalWarnings = Report.DuplicateWidgetNames.Num() + Report.DuplicateAccessibleNames.Num();

	// Summary headline
	SB.Appendf(TEXT("Accessible UMG Inspector: %s [Serialized Asset]\n"), *Report.AssetPath);
	SB.Appendf(TEXT("Parent Class: %s | Total Widgets: %d | Named Slots: %d | Package Clean: %s | Warnings: %d\n"),
		*Report.ParentClass,
		Report.TotalWidgets,
		Report.NamedSlotCount,
		(Report.bPackageWasCleanBefore && Report.bPackageRemainsCleanAfter) ? TEXT("Yes") : TEXT("No"),
		TotalWarnings);

	// Actionable warnings first
	if (TotalWarnings > 0)
	{
		SB.Append(TEXT("\nActionable Warnings:\n"));
		for (const FString& DupWidget : Report.DuplicateWidgetNames)
		{
			SB.Appendf(TEXT("- [WARN-DUPLICATE-WIDGET-NAME] Multiple widgets share the internal name '%s'\n"), *DupWidget);
		}
		for (const FString& DupA11y : Report.DuplicateAccessibleNames)
		{
			SB.Appendf(TEXT("- [WARN-DUPLICATE-ACCESSIBLE-NAME] Multiple controls share the accessible name '%s'\n"), *DupA11y);
		}
	}

	// Hierarchy tree
	SB.Append(TEXT("\nSerialized Widget Hierarchy:\n"));
	if (Report.RootWidgetIndex >= 0 && Report.Widgets.IsValidIndex(Report.RootWidgetIndex))
	{
		FormatWidgetNode(SB, Report, Report.RootWidgetIndex, TEXT("1"));
	}
	else
	{
		SB.Append(TEXT("  (Tree is empty - No root widget declared)\n"));
	}

	return SB.ToString();
}

void FAccessibleUMGNvdaTextFormatter::FormatWidgetNode(FStringBuilderBase& OutBuilder, const FSerializedWidgetBlueprintReport& Report, int32 NodeIndex, const FString& Prefix)
{
	if (!Report.Widgets.IsValidIndex(NodeIndex))
	{
		return;
	}

	const FSerializedWidgetRecord& Node = Report.Widgets[NodeIndex];

	// Main node declaration line: Prefix. WidgetName (WidgetClass) [Visibility] [Enabled/Disabled] [Focusable]
	OutBuilder.Appendf(TEXT("%s. %s (%s) [%s]"), *Prefix, *Node.WidgetName, *Node.WidgetClass, *Node.DefaultVisibility);

	if (!Node.bIsEnabled)
	{
		OutBuilder.Append(TEXT(" [Disabled]"));
	}
	else
	{
		OutBuilder.Append(TEXT(" [Enabled]"));
	}

	if (Node.bIsFocusable)
	{
		OutBuilder.Append(TEXT(" [Focusable]"));
	}

	if (Node.bIsVariable)
	{
		OutBuilder.Append(TEXT(" [Variable]"));
	}

	OutBuilder.Append(TEXT("\n"));

	// Sub-line indentation
	FString SubIndent = TEXT("   ");

	// Accessible text / summary
	if (!Node.AccessibleName.IsEmpty())
	{
		OutBuilder.Appendf(TEXT("%sAccessible Name: \"%s\"\n"), *SubIndent, *Node.AccessibleName);
	}
	else if (Node.bOverrideAccessibleDefaults)
	{
		OutBuilder.Appendf(TEXT("%sAccessible Name: (Explicitly Empty)\n"), *SubIndent);
	}

	if (!Node.AccessibleSummary.IsEmpty())
	{
		OutBuilder.Appendf(TEXT("%sAccessible Summary: \"%s\"\n"), *SubIndent, *Node.AccessibleSummary);
	}

	if (!Node.AccessibleBehavior.IsEmpty() && Node.AccessibleBehavior != TEXT("Auto"))
	{
		OutBuilder.Appendf(TEXT("%sAccessible Behavior: %s\n"), *SubIndent, *Node.AccessibleBehavior);
	}

	// Slot information
	if (Node.bHasSlot)
	{
		OutBuilder.Appendf(TEXT("%sSlot: %s in %s"), *SubIndent, *Node.Slot.SlotType, *Node.Slot.ParentWidgetName);
		if (Node.Slot.Properties.Num() > 0)
		{
			OutBuilder.Append(TEXT(" ["));
			bool bFirst = true;
			for (const auto& Prop : Node.Slot.Properties)
			{
				if (!bFirst) OutBuilder.Append(TEXT(", "));
				OutBuilder.Appendf(TEXT("%s: %s"), *Prop.Key, *Prop.Value);
				bFirst = false;
			}
			OutBuilder.Append(TEXT("]"));
		}
		OutBuilder.Append(TEXT("\n"));
	}

	// Navigation rules
	for (const FAccessibleNavigationRecord& Nav : Node.NavigationRules)
	{
		OutBuilder.Appendf(TEXT("%sNav %s: %s -> %s\n"), *SubIndent, *Nav.Direction, *Nav.Rule, *Nav.TargetWidget);
	}

	// Bindings
	for (const FAccessibleWidgetBindingRecord& Binding : Node.Bindings)
	{
		OutBuilder.Appendf(TEXT("%sBinding: %s -> %s (%s)\n"), *SubIndent, *Binding.PropertyName, *Binding.FunctionName, *Binding.BindingKind);
	}

	// Named slots
	if (Node.bIsNamedSlotHost && Node.ExposedNamedSlots.Num() > 0)
	{
		OutBuilder.Appendf(TEXT("%sExposed Named Slots: %s\n"), *SubIndent, *FString::Join(Node.ExposedNamedSlots, TEXT(", ")));
	}

	// Children
	for (int32 ChildIdx = 0; ChildIdx < Node.ChildIndices.Num(); ++ChildIdx)
	{
		int32 ChildNodeIndex = Node.ChildIndices[ChildIdx];
		FString ChildPrefix = FString::Printf(TEXT("%s.%d"), *Prefix, ChildIdx + 1);
		FormatWidgetNode(OutBuilder, Report, ChildNodeIndex, ChildPrefix);
	}
}

FString FAccessibleUMGNvdaTextFormatter::FormatRuntimeReport(const FRuntimeWidgetReport& Report)
{
	TStringBuilder<4096> SB;

	if (!Report.bSuccess)
	{
		SB.Appendf(TEXT("Accessible UMG Inspector Error: %s\n"), *Report.ErrorMessage);
		SB.Appendf(TEXT("Target Reference: %s\n"), *Report.RootWidgetName);
		if (Report.Candidates.Num() > 0)
		{
			SB.Append(TEXT("Candidates:\n"));
			for (const FString& Cand : Report.Candidates)
			{
				SB.Appendf(TEXT("- %s\n"), *Cand);
			}
		}
		return SB.ToString();
	}

	// Headline
	SB.Appendf(TEXT("Accessible UMG Inspector: %s [Runtime Viewport / UserWidget]\n"), *Report.RootWidgetName);
	SB.Appendf(TEXT("World: %s | Total: %d | Active: %d | Inactive/Hidden: %d | Focused Widget: %s\n"),
		*Report.WorldName,
		Report.TotalWidgets,
		Report.ActiveWidgetCount,
		Report.HiddenOrCollapsedCount,
		Report.bHasFocus ? *Report.FocusedWidgetName : TEXT("None"));

	if (Report.bTruncated)
	{
		SB.Append(TEXT("Warning: Inspection truncated (exceeded depth or node limit).\n"));
	}

	// Active and visible hierarchy
	SB.Append(TEXT("\nActive Controls Hierarchy:\n"));
	if (Report.RootWidgetIndex >= 0 && Report.Widgets.IsValidIndex(Report.RootWidgetIndex))
	{
		FormatRuntimeWidgetNode(SB, Report, Report.RootWidgetIndex, TEXT("1"), true);
	}
	else
	{
		SB.Append(TEXT("  (No active controls)\n"));
	}

	// Inactive, collapsed, or hidden controls section
	if (Report.HiddenOrCollapsedCount > 0)
	{
		SB.Append(TEXT("\nInactive / Collapsed / Hidden Subtrees:\n"));
		int32 HiddenIndex = 1;
		for (const FRuntimeWidgetRecord& Rec : Report.Widgets)
		{
			if (!Rec.bEffectivelyVisible || Rec.EffectiveVisibility == TEXT("Collapsed") || Rec.EffectiveVisibility == TEXT("Hidden") ||
				Rec.LocalVisibility == TEXT("Collapsed") || Rec.LocalVisibility == TEXT("Hidden"))
			{
				if (Rec.BlockingAncestorIndex >= 0)
				{
					SB.Appendf(TEXT("[%d] %s (%s) [Local: %s, Effective: %s, Suppressed by: %s] - Path: %s\n"),
						HiddenIndex++,
						*Rec.WidgetName,
						*Rec.WidgetClass,
						*Rec.LocalVisibility,
						*Rec.EffectiveVisibility,
						*Rec.BlockingAncestorName,
						*Rec.ObjectPath);
				}
				else
				{
					SB.Appendf(TEXT("[%d] %s (%s) [%s] - Path: %s\n"),
						HiddenIndex++,
						*Rec.WidgetName,
						*Rec.WidgetClass,
						!Rec.EffectiveVisibility.IsEmpty() ? *Rec.EffectiveVisibility : *Rec.RuntimeVisibility,
						*Rec.ObjectPath);
				}
			}
		}
	}

	return SB.ToString();
}

void FAccessibleUMGNvdaTextFormatter::FormatRuntimeWidgetNode(
	FStringBuilderBase& OutBuilder,
	const FRuntimeWidgetReport& Report,
	int32 NodeIndex,
	const FString& Prefix,
	bool bOnlyActive)
{
	if (!Report.Widgets.IsValidIndex(NodeIndex))
	{
		return;
	}

	const FRuntimeWidgetRecord& Node = Report.Widgets[NodeIndex];

	// If filtering for active and this node is not effectively visible (or collapsed/hidden), skip formatting its subtree in active section
	if (bOnlyActive && (!Node.bEffectivelyVisible || Node.EffectiveVisibility == TEXT("Collapsed") || Node.EffectiveVisibility == TEXT("Hidden") ||
		Node.RuntimeVisibility == TEXT("Collapsed") || Node.RuntimeVisibility == TEXT("Hidden")))
	{
		return;
	}

	// Prefix. WidgetName (WidgetClass) [EffectiveVisibility] [Enabled/Disabled] [Focusable] [Focused] [Hovered]
	OutBuilder.Appendf(TEXT("%s. %s (%s) [%s]"),
		*Prefix,
		*Node.WidgetName,
		*Node.WidgetClass,
		!Node.EffectiveVisibility.IsEmpty() ? *Node.EffectiveVisibility : *Node.RuntimeVisibility);

	if (!Node.bIsEnabled)
	{
		OutBuilder.Append(TEXT(" [Disabled]"));
	}
	else
	{
		OutBuilder.Append(TEXT(" [Enabled]"));
	}

	if (Node.bIsFocusable)
	{
		OutBuilder.Append(TEXT(" [Focusable]"));
	}

	if (Node.bHasKeyboardFocus || Node.bHasUserFocus)
	{
		OutBuilder.Append(TEXT(" [Focused]"));
	}

	if (Node.bIsHovered)
	{
		OutBuilder.Append(TEXT(" [Hovered]"));
	}

	OutBuilder.Append(TEXT("\n"));

	// Sub-line indentation
	FString SubIndent = TEXT("   ");

	// Screen Bounds if valid
	if (Node.ScreenBounds.bIsValid)
	{
		OutBuilder.Appendf(TEXT("%sBounds: (X=%.0f, Y=%.0f, W=%.0f, H=%.0f)\n"),
			*SubIndent,
			Node.ScreenBounds.X,
			Node.ScreenBounds.Y,
			Node.ScreenBounds.Width,
			Node.ScreenBounds.Height);
	}

	// Accessible Name / Summary
	if (!Node.AccessibleName.IsEmpty())
	{
		OutBuilder.Appendf(TEXT("%sAccessible Name: \"%s\"\n"), *SubIndent, *Node.AccessibleName);
	}
	else if (Node.bOverrideAccessibleDefaults)
	{
		OutBuilder.Appendf(TEXT("%sAccessible Name: (Explicitly Empty)\n"), *SubIndent);
	}

	if (!Node.AccessibleSummary.IsEmpty())
	{
		OutBuilder.Appendf(TEXT("%sAccessible Summary: \"%s\"\n"), *SubIndent, *Node.AccessibleSummary);
	}

	if (Node.bHasSlateWidget)
	{
		OutBuilder.Appendf(TEXT("%sSlate Type: %s [Slate Visible: %s, Slate Enabled: %s]\n"),
			*SubIndent,
			*Node.SlateWidgetType,
			Node.bSlateVisible ? TEXT("Yes") : TEXT("No"),
			Node.bSlateEnabled ? TEXT("Yes") : TEXT("No"));
	}

	// Slot
	if (Node.bHasSlot)
	{
		OutBuilder.Appendf(TEXT("%sSlot: %s (Parent: %s)\n"), *SubIndent, *Node.Slot.SlotType, *Node.Slot.ParentWidgetName);
	}

	// Navigation rules
	for (const FAccessibleNavigationRecord& Nav : Node.NavigationRules)
	{
		OutBuilder.Appendf(TEXT("%sNav %s: %s -> %s\n"), *SubIndent, *Nav.Direction, *Nav.Rule, *Nav.TargetWidget);
	}

	// Format active children
	int32 SubIndex = 1;
	for (int32 ChildIdx : Node.ChildIndices)
	{
		if (Report.Widgets.IsValidIndex(ChildIdx))
		{
			const FRuntimeWidgetRecord& ChildRecord = Report.Widgets[ChildIdx];
			if (!bOnlyActive || (ChildRecord.bEffectivelyVisible && ChildRecord.EffectiveVisibility != TEXT("Collapsed") && ChildRecord.EffectiveVisibility != TEXT("Hidden")))
			{
				FString ChildPrefix = FString::Printf(TEXT("%s.%d"), *Prefix, SubIndex++);
				FormatRuntimeWidgetNode(OutBuilder, Report, ChildIdx, ChildPrefix, bOnlyActive);
			}
		}
	}
}

FString FAccessibleUMGNvdaTextFormatter::FormatComparisonReport(const FSerializedRuntimeComparisonReport& Report)
{
	TStringBuilder<4096> SB;

	if (!Report.bSuccess)
	{
		SB.Appendf(TEXT("Accessible UMG Inspector Error: %s\n"), *Report.ErrorMessage);
		SB.Appendf(TEXT("Target Asset: %s\n"), *Report.AssetPath);
		if (!Report.RuntimeWidgetIdentity.IsEmpty())
		{
			SB.Appendf(TEXT("Runtime Reference: %s\n"), *Report.RuntimeWidgetIdentity);
		}
		if (Report.Candidates.Num() > 0)
		{
			SB.Append(TEXT("Candidates:\n"));
			for (const FString& Cand : Report.Candidates)
			{
				SB.Appendf(TEXT("- %s\n"), *Cand);
			}
		}
		return SB.ToString();
	}

	// 1. One-line result summary
	SB.Appendf(TEXT("Accessible UMG Inspector: Reconciliation for %s vs %s [Matched: %d | Ambiguous: %d | SerializedOnly: %d | RuntimeOnly: %d | Unavailable: %d | Unsupported: %d]\n"),
		*Report.AssetPath,
		*Report.RuntimeWidgetIdentity,
		Report.MatchedCount,
		Report.AmbiguousCount,
		Report.SerializedOnlyCount,
		Report.RuntimeOnlyCount,
		Report.UnavailableCount,
		Report.UnsupportedCount);

	// 2. Selected asset, runtime widget, and world
	SB.Appendf(TEXT("Asset: %s | Runtime Widget: %s | World: %s\n"),
		*Report.AssetPath,
		*Report.RuntimeWidgetIdentity,
		*Report.WorldIdentity);

	if (Report.bTruncated)
	{
		SB.Append(TEXT("Warning: Reconciliation truncated due to input report limits.\n"));
	}

	// 3. Ambiguous and conflicting entries FIRST
	if (Report.AmbiguousCount > 0)
	{
		SB.Append(TEXT("\nAmbiguous / Conflicting Entries (Attention Required):\n"));
		int32 AmbIdx = 1;
		for (const FWidgetComparisonRecord& Rec : Report.ComparisonRecords)
		{
			if (Rec.Classification == EWidgetComparisonClassification::Ambiguous)
			{
				SB.Appendf(TEXT("[%d] [AMBIGUOUS] Serialized: '%s' (%s) | Runtime: '%s' (%s)\n"),
					AmbIdx++,
					*Rec.SerializedWidgetName,
					*Rec.SerializedClass,
					*Rec.RuntimeWidgetName,
					*Rec.RuntimeWidgetClass);

				for (const FString& Conf : Rec.ConflictingEvidence)
				{
					SB.Appendf(TEXT("    - Conflict: %s\n"), *Conf);
				}
				for (const FString& Disc : Rec.Discrepancies)
				{
					SB.Appendf(TEXT("    - Discrepancy: %s\n"), *Disc);
				}
			}
		}
	}

	// 4. Numbered comparison entries
	SB.Append(TEXT("\nReconciled Widget Comparison Entries:\n"));
	int32 EntryIdx = 1;
	for (const FWidgetComparisonRecord& Rec : Report.ComparisonRecords)
	{
		FString ClassTag = Rec.ClassificationString.ToUpper();
		SB.Appendf(TEXT("%d. [%s] %s"),
			EntryIdx++,
			*ClassTag,
			!Rec.SerializedWidgetName.IsEmpty() ? *Rec.SerializedWidgetName : *Rec.RuntimeWidgetName);

		if (!Rec.SerializedClass.IsEmpty())
		{
			SB.Appendf(TEXT(" (%s)"), *Rec.SerializedClass);
		}
		else if (!Rec.RuntimeWidgetClass.IsEmpty())
		{
			SB.Appendf(TEXT(" (%s)"), *Rec.RuntimeWidgetClass);
		}

		if (Rec.Classification == EWidgetComparisonClassification::Matched)
		{
			SB.Appendf(TEXT(" - Serialized: %s | Runtime: %s\n"), *Rec.SerializedWidgetName, *Rec.RuntimeWidgetName);
			SB.Appendf(TEXT("   Parent: %s | Child Order: %d\n"), *Rec.SerializedParent, Rec.SerializedChildOrder);
		}
		else if (Rec.Classification == EWidgetComparisonClassification::SerializedOnly)
		{
			SB.Appendf(TEXT(" - Present in Blueprint, absent in runtime\n"));
			SB.Appendf(TEXT("   Parent: %s | Child Order: %d\n"), *Rec.SerializedParent, Rec.SerializedChildOrder);
		}
		else if (Rec.Classification == EWidgetComparisonClassification::RuntimeOnly)
		{
			SB.Appendf(TEXT(" - Dynamically created or internal Slate node\n"));
			SB.Appendf(TEXT("   Runtime Parent: %s | Slate Type: %s\n"), *Rec.RuntimeParent, *Rec.RuntimeSlateType);
		}
		else
		{
			SB.Append(TEXT("\n"));
		}

		// Evidence
		for (const FString& Ev : Rec.Evidence)
		{
			SB.Appendf(TEXT("   Evidence: %s\n"), *Ev);
		}

		// Discrepancies on this record
		for (const FString& Disc : Rec.Discrepancies)
		{
			SB.Appendf(TEXT("   Discrepancy: %s\n"), *Disc);
		}
	}

	// 5. Descriptive discrepancies summary
	if (Report.Discrepancies.Num() > 0)
	{
		SB.Appendf(TEXT("\nDescriptive Discrepancies (%d total):\n"), Report.Discrepancies.Num());
		for (const FString& Disc : Report.Discrepancies)
		{
			SB.Appendf(TEXT("- %s\n"), *Disc);
		}
	}

		// Explicit limitations
		if (Report.Limitations.Num() > 0)
		{
			SB.Append(TEXT("\nReconciliation Limitations:\n"));
			for (const FString& Lim : Report.Limitations)
			{
				SB.Appendf(TEXT("- %s\n"), *Lim);
			}
		}

		return SB.ToString();
	}

FString FAccessibleUMGNvdaTextFormatter::FormatAuditReport(const FAccessibilityAuditReport& Report)
{
	TStringBuilder<4096> SB;

	if (!Report.bSuccess)
	{
		SB.Appendf(TEXT("Accessible UMG Inspector Audit Error: %s\n"), *Report.ErrorMessage);
		if (!Report.AssetPath.IsEmpty())
		{
			SB.Appendf(TEXT("Target Asset: %s\n"), *Report.AssetPath);
		}
		if (!Report.RuntimeWidgetIdentity.IsEmpty())
		{
			SB.Appendf(TEXT("Runtime Reference: %s\n"), *Report.RuntimeWidgetIdentity);
		}
		if (Report.Candidates.Num() > 0)
		{
			SB.Append(TEXT("Candidates:\n"));
			for (const FString& Cand : Report.Candidates)
			{
				SB.Appendf(TEXT("- %s\n"), *Cand);
			}
		}
		return SB.ToString();
	}

	FString TargetName = !Report.AssetPath.IsEmpty() ? Report.AssetPath : Report.RuntimeWidgetIdentity;

	// 1. Opening line
	SB.Appendf(TEXT("Target: %s | Evidence Mode: %s | Errors: %d, Warnings: %d, Info: %d, Not Evaluated: %d, Passed: %d\n"),
		*TargetName,
		*Report.EvidenceMode,
		Report.ErrorCount,
		Report.WarningCount,
		Report.InfoCount,
		Report.NotEvaluatedCount,
		Report.PassedCount);

	int32 FindingNumber = 1;

	// Helper to format finding
	auto FormatFinding = [&](const FAuditFindingRecord& F)
	{
		SB.Appendf(TEXT("\nFinding %d: [%s] %s\n"), FindingNumber++, *F.RuleId, *F.RuleName);
		if (!F.WidgetIdentity.IsEmpty())
		{
			SB.Appendf(TEXT("  Affected Control: %s\n"), *F.WidgetIdentity);
		}
		SB.Appendf(TEXT("  Classification: %s\n"), *F.ClassificationString);
		SB.Appendf(TEXT("  Observed: %s\n"), *F.Message);
		SB.Appendf(TEXT("  Evidence Source: %s Confidence\n"), *F.ConfidenceBasisString);

		for (const FString& Ev : F.SerializedEvidence)
		{
			SB.Appendf(TEXT("    Serialized: %s\n"), *Ev);
		}
		for (const FString& Ev : F.RuntimeEvidence)
		{
			SB.Appendf(TEXT("    Runtime: %s\n"), *Ev);
		}
		for (const FString& Ev : F.ComparisonEvidence)
		{
			SB.Appendf(TEXT("    Comparison: %s\n"), *Ev);
		}
		for (const FString& Ev : F.UnavailableEvidence)
		{
			SB.Appendf(TEXT("    Unavailable: %s\n"), *Ev);
		}

		// Why it matters
		FString WhyItMatters;
		if (F.RuleId == TEXT("A11Y-01")) WhyItMatters = TEXT("Controls unexpectedly exposed at runtime in inactive containers can confuse screen readers.");
		else if (F.RuleId == TEXT("A11Y-02")) WhyItMatters = TEXT("Screen readers announce unnamed controls as generic unlabelled elements, preventing blind users from understanding their purpose.");
		else if (F.RuleId == TEXT("A11Y-03")) WhyItMatters = TEXT("Keyboard-focusable controls with no accessible name cannot be identified when receiving focus.");
		else if (F.RuleId == TEXT("A11Y-04")) WhyItMatters = TEXT("Identical accessible names create ambiguity for screen-reader users distinguishing between controls.");
		else if (F.RuleId == TEXT("A11Y-05")) WhyItMatters = TEXT("Interactive controls outside intended panels disrupt logical tab order and reading hierarchy.");
		else if (F.RuleId == TEXT("A11Y-06")) WhyItMatters = TEXT("Descendants of collapsed screens accepting focus create ghost controls that confuse users.");
		else if (F.RuleId == TEXT("A11Y-07")) WhyItMatters = TEXT("Focus residing on inactive screens causes keyboard input to be dispatched to invisible interface layers.");
		else if (F.RuleId == TEXT("A11Y-08")) WhyItMatters = TEXT("Navigation targets pointing outside the active screen break keyboard focus containment.");
		else if (F.RuleId == TEXT("A11Y-09")) WhyItMatters = TEXT("Active modals must trap focus; allowing underlying controls to receive focus breaks modal accessibility.");
		else if (F.RuleId == TEXT("A11Y-10")) WhyItMatters = TEXT("Unexplained visibility divergence indicates potential desynchronization between UI state and live gameplay.");
		else if (F.RuleId == TEXT("A11Y-11")) WhyItMatters = TEXT("Overlapping interactive controls can cause touch or pointer hit-test conflicts.");
		else if (F.RuleId == TEXT("A11Y-12")) WhyItMatters = TEXT("Action controls without event bindings cannot execute user actions when triggered.");
		else if (F.RuleId == TEXT("A11Y-13")) WhyItMatters = TEXT("Accessible child leak violates parent container accessibility scoping.");
		else if (F.RuleId == TEXT("A11Y-14")) WhyItMatters = TEXT("Ambiguous identity prevents deterministic accessibility audit.");
		else WhyItMatters = TEXT("Accessibility violations impact screen-reader and keyboard users.");

		SB.Appendf(TEXT("  Why It Matters: %s\n"), *WhyItMatters);

		if (!F.RemediationGuidance.IsEmpty())
		{
			SB.Appendf(TEXT("  Remediation Guidance: %s\n"), *F.RemediationGuidance);
		}

		for (const FString& Lim : F.Limitations)
		{
			SB.Appendf(TEXT("  Limitation: %s\n"), *Lim);
		}
	};

	// 2. Section: Errors
	SB.Appendf(TEXT("\n=== ERRORS (%d) ===\n"), Report.ErrorCount);
	for (const FAuditFindingRecord& F : Report.Findings)
	{
		if (F.Classification == EAuditFindingClassification::Error)
		{
			FormatFinding(F);
		}
	}

	// 3. Section: Warnings
	SB.Appendf(TEXT("\n=== WARNINGS (%d) ===\n"), Report.WarningCount);
	for (const FAuditFindingRecord& F : Report.Findings)
	{
		if (F.Classification == EAuditFindingClassification::Warning)
		{
			FormatFinding(F);
		}
	}

	// 4. Section: Human Review Required
	SB.Append(TEXT("\n=== HUMAN REVIEW REQUIRED ===\n"));
	bool bHasHumanReview = false;
	for (const FAuditFindingRecord& F : Report.Findings)
	{
		if (F.RuleId == TEXT("A11Y-04") || F.RuleId == TEXT("A11Y-14") || F.ConfidenceBasis == EAuditConfidenceBasis::Ambiguous)
		{
			bHasHumanReview = true;
			SB.Appendf(TEXT("- [%s] %s: Human verification required for '%s'. %s\n"),
				*F.RuleId, *F.RuleName, *F.WidgetIdentity, *F.Message);
		}
	}
	if (!bHasHumanReview)
	{
		SB.Append(TEXT("None. Automated rule evaluation completed with high confidence evidence.\n"));
	}

	// 5. Section: Not Evaluated
	SB.Appendf(TEXT("\n=== NOT EVALUATED (%d) ===\n"), Report.NotEvaluatedCount);
	for (const FAuditFindingRecord& F : Report.Findings)
	{
		if (F.Classification == EAuditFindingClassification::NotEvaluated)
		{
			FormatFinding(F);
		}
	}
	for (const FString& RuleId : Report.NotEvaluatedRules)
	{
		// If not already in findings as a record
		bool bInFindings = false;
		for (const FAuditFindingRecord& F : Report.Findings)
		{
			if (F.RuleId == RuleId && F.Classification == EAuditFindingClassification::NotEvaluated)
			{
				bInFindings = true;
				break;
			}
		}
		if (!bInFindings)
		{
			SB.Appendf(TEXT("- [%s] Required evidence not available in current inspection mode.\n"), *RuleId);
		}
	}

	// 6. Section: Passed Rules
	SB.Appendf(TEXT("\n=== PASSED RULES (%d) ===\n"), Report.PassedCount);
	if (Report.PassedRuleIds.Num() > 0)
	{
		SB.Appendf(TEXT("Passed: %s\n"), *FString::Join(Report.PassedRuleIds, TEXT(", ")));
	}
	else
	{
		SB.Append(TEXT("None.\n"));
	}

	// 7. Section: Architectural Limitations
	SB.Append(TEXT("\n=== ARCHITECTURAL LIMITATIONS ===\n"));
	for (const FString& Lim : Report.Limitations)
	{
		SB.Appendf(TEXT("- %s\n"), *Lim);
	}

	return SB.ToString();
}


