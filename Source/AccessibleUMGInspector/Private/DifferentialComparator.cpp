// Copyright Blind Lion Studios. All Rights Reserved.

#include "DifferentialComparator.h"
#include "SerializedWidgetInspector.h"
#include "RuntimeWidgetInspector.h"
#include "NvdaTextFormatter.h"
#include "AccessibleUMGInspectorTypes.h"
#include "JsonObjectConverter.h"

FSerializedRuntimeComparisonReport FDifferentialComparator::Compare(
	const FString& AssetPath,
	const FString& RuntimeWidgetReference,
	const FString& WorldContext,
	int32 MaxDepth,
	const FString& Format)
{
	FSerializedRuntimeComparisonReport Report;

	if (AssetPath.IsEmpty())
	{
		Report.bSuccess = false;
		Report.Availability = EInspectionAvailability::Unavailable;
		Report.ErrorMessage = TEXT("AssetPath must not be empty.");
		return Report;
	}

	// 1. Run serialized inspector against AssetPath
	FSerializedWidgetBlueprintReport SerializedReport = FSerializedWidgetInspector::InspectAsset(AssetPath, TEXT("Both"));
	if (!SerializedReport.bSuccess)
	{
		Report.bSuccess = false;
		Report.Availability = EInspectionAvailability::Unavailable;
		Report.ErrorMessage = FString::Printf(TEXT("Serialized inspection failed: %s"), *SerializedReport.ErrorMessage);
		Report.AssetPath = AssetPath;

		bool bWantsNvda = Format.Equals(TEXT("NvdaText"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);
		bool bWantsJson = Format.Equals(TEXT("Json"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);

		if (bWantsNvda)
		{
			Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatComparisonReport(Report);
		}
		if (bWantsJson)
		{
			FString JsonStr;
			FJsonObjectConverter::UStructToJsonObjectString(Report, JsonStr, 0, 0, 0, nullptr, false);
			Report.JsonData = JsonStr;
		}

		return Report;
	}

	// 2. Run runtime inspector against RuntimeWidgetReference and WorldContext
	FRuntimeWidgetReport RuntimeReport = FRuntimeWidgetInspector::InspectRuntimeUserWidget(RuntimeWidgetReference, WorldContext, MaxDepth, TEXT("Both"));
	if (!RuntimeReport.bSuccess)
	{
		Report.bSuccess = false;
		Report.Availability = RuntimeReport.Availability;
		Report.ErrorMessage = FString::Printf(TEXT("Runtime inspection failed: %s"), *RuntimeReport.ErrorMessage);
		Report.AssetPath = AssetPath;
		Report.RuntimeWidgetIdentity = RuntimeWidgetReference;
		Report.WorldIdentity = RuntimeReport.WorldName;
		Report.Candidates = RuntimeReport.Candidates;

		bool bWantsNvda = Format.Equals(TEXT("NvdaText"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);
		bool bWantsJson = Format.Equals(TEXT("Json"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);

		if (bWantsNvda)
		{
			Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatComparisonReport(Report);
		}
		if (bWantsJson)
		{
			FString JsonStr;
			FJsonObjectConverter::UStructToJsonObjectString(Report, JsonStr, 0, 0, 0, nullptr, false);
			Report.JsonData = JsonStr;
		}

		return Report;
	}

	// 3. Reconcile both valid reports
	return ReconcileReports(SerializedReport, RuntimeReport, Format);
}

FSerializedRuntimeComparisonReport FDifferentialComparator::ReconcileReports(
	const FSerializedWidgetBlueprintReport& SerializedReport,
	const FRuntimeWidgetReport& InRuntimeReport,
	const FString& Format)
{
	FRuntimeWidgetReport RuntimeReport = InRuntimeReport;
	if (RuntimeReport.Widgets.Num() > 0 && RuntimeReport.Widgets[0].EffectiveVisibility.IsEmpty())
	{
		FRuntimeWidgetInspector::ComputeEffectiveVisibility(RuntimeReport);
	}
	FSerializedRuntimeComparisonReport Report;

	// Forward failure if either input report failed
	if (!SerializedReport.bSuccess)
	{
		Report.bSuccess = false;
		Report.Availability = EInspectionAvailability::Unavailable;
		Report.ErrorMessage = FString::Printf(TEXT("Serialized inspection failed: %s"), *SerializedReport.ErrorMessage);
		Report.AssetPath = SerializedReport.AssetPath;
		return Report;
	}

	if (!RuntimeReport.bSuccess)
	{
		Report.bSuccess = false;
		Report.Availability = RuntimeReport.Availability;
		Report.ErrorMessage = FString::Printf(TEXT("Runtime inspection failed: %s"), *RuntimeReport.ErrorMessage);
		Report.AssetPath = SerializedReport.AssetPath;
		Report.RuntimeWidgetIdentity = RuntimeReport.RootWidgetName;
		Report.WorldIdentity = RuntimeReport.WorldName;
		Report.Candidates = RuntimeReport.Candidates;
		return Report;
	}

	Report.bSuccess = true;
	Report.Availability = EInspectionAvailability::Available;
	Report.AssetPath = SerializedReport.AssetPath;
	Report.RuntimeWidgetIdentity = RuntimeReport.RootWidgetName;
	Report.bTruncated = RuntimeReport.bTruncated;

	// Core matching & discrepancy detection
	MatchNodes(SerializedReport, RuntimeReport, Report.ComparisonRecords, Report.Discrepancies);

	// Detect global runtime discrepancies (e.g. overlapping bounds, detached nodes)
	DetectGlobalRuntimeDiscrepancies(RuntimeReport, Report.ComparisonRecords, Report.Discrepancies);

	// Count classifications
	for (const FWidgetComparisonRecord& Rec : Report.ComparisonRecords)
	{
		switch (Rec.Classification)
		{
		case EWidgetComparisonClassification::Matched:        Report.MatchedCount++; break;
		case EWidgetComparisonClassification::Ambiguous:      Report.AmbiguousCount++; break;
		case EWidgetComparisonClassification::SerializedOnly: Report.SerializedOnlyCount++; break;
		case EWidgetComparisonClassification::RuntimeOnly:    Report.RuntimeOnlyCount++; break;
		case EWidgetComparisonClassification::Unavailable:    Report.UnavailableCount++; break;
		case EWidgetComparisonClassification::Unsupported:    Report.UnsupportedCount++; break;
		default: break;
		}
	}

	// Architectural limitations documentation
	Report.Limitations.Add(TEXT("Observation is strictly non-mutating and passive. Unconstructed Slate widgets are never spawned."));
	Report.Limitations.Add(TEXT("Accessible names and screen bounds are used strictly as supporting evidence and discrepancy indicators, never standalone identity proofs."));
	Report.Limitations.Add(TEXT("Serialized Collapsed or Hidden controls may validly appear as SerializedOnly when no live Slate node was constructed."));
	Report.Limitations.Add(TEXT("Dynamically instantiated runtime controls are reported as RuntimeOnly unless direct Blueprint identity is proven."));

	// Format outputs
	bool bWantsNvda = Format.Equals(TEXT("NvdaText"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);
	bool bWantsJson = Format.Equals(TEXT("Json"), ESearchCase::IgnoreCase) || Format.Equals(TEXT("Both"), ESearchCase::IgnoreCase);

	if (bWantsNvda)
	{
		Report.NvdaText = FAccessibleUMGNvdaTextFormatter::FormatComparisonReport(Report);
	}

	if (bWantsJson)
	{
		FString JsonStr;
		FJsonObjectConverter::UStructToJsonObjectString(Report, JsonStr, 0, 0, 0, nullptr, false);
		Report.JsonData = JsonStr;
	}

	return Report;
}

void FDifferentialComparator::MatchNodes(
	const FSerializedWidgetBlueprintReport& SerializedReport,
	const FRuntimeWidgetReport& RuntimeReport,
	TArray<FWidgetComparisonRecord>& OutRecords,
	TArray<FString>& OutAllDiscrepancies)
{
	int32 SNum = SerializedReport.Widgets.Num();
	int32 RNum = RuntimeReport.Widgets.Num();

	TSet<int32> MatchedS;
	TSet<int32> MatchedR;
	TSet<int32> AmbiguousS;
	TSet<int32> AmbiguousR;

	// Candidate map: for each serialized index, list of compatible runtime indices
	TMap<int32, TArray<int32>> SCandidates;
	// Candidate map: for each runtime index, list of compatible serialized indices
	TMap<int32, TArray<int32>> RCandidates;

	// Track class mismatches when names match (for explicit conflict reporting)
	TMap<int32, TArray<int32>> ClassMismatchMap;

	for (int32 s = 0; s < SNum; ++s)
	{
		const FSerializedWidgetRecord& S = SerializedReport.Widgets[s];

		for (int32 r = 0; r < RNum; ++r)
		{
			const FRuntimeWidgetRecord& R = RuntimeReport.Widgets[r];

			// Name match check
			if (S.WidgetName.Equals(R.WidgetName, ESearchCase::IgnoreCase))
			{
				bool bClassCompatible = S.WidgetClass.Equals(R.WidgetClass, ESearchCase::IgnoreCase);

				if (bClassCompatible)
				{
					SCandidates.FindOrAdd(s).Add(r);
					RCandidates.FindOrAdd(r).Add(s);
				}
				else
				{
					// Class mismatch prevents match!
					ClassMismatchMap.FindOrAdd(s).Add(r);
				}
			}
		}
	}

	// Parent match helper lambda
	auto CheckParentMatches = [&RuntimeReport](const FSerializedWidgetRecord& S, const FRuntimeWidgetRecord& R) -> bool
	{
		if (!S.ParentName.IsEmpty() && !R.ParentName.IsEmpty() && S.ParentName.Equals(R.ParentName, ESearchCase::IgnoreCase))
		{
			return true;
		}

		bool bSIsRoot = S.ParentName.IsEmpty() || S.ParentName.Equals(TEXT("None"), ESearchCase::IgnoreCase);
		bool bRIsRoot = R.ParentName.IsEmpty() || R.ParentName.Equals(TEXT("None"), ESearchCase::IgnoreCase) ||
			(!RuntimeReport.RootWidgetName.IsEmpty() && R.ParentName.Equals(RuntimeReport.RootWidgetName, ESearchCase::IgnoreCase)) ||
			R.ParentIndex == 0 ||
			R.ParentIndex == -1;

		return bSIsRoot && bRIsRoot;
	};

	// First pass: Resolve unambiguous 1-to-1 candidates
	for (int32 s = 0; s < SNum; ++s)
	{
		const FSerializedWidgetRecord& S = SerializedReport.Widgets[s];
		const TArray<int32>* CandR = SCandidates.Find(s);

		if (!CandR || CandR->Num() == 0)
		{
			continue;
		}

		if (CandR->Num() == 1)
		{
			int32 r = (*CandR)[0];
			const TArray<int32>* CandS = RCandidates.Find(r);

			if (CandS && CandS->Num() == 1)
			{
				// Exactly 1-to-1 match by name and class
				const FRuntimeWidgetRecord& R = RuntimeReport.Widgets[r];

				FWidgetComparisonRecord Rec;
				Rec.Classification = EWidgetComparisonClassification::Matched;
				Rec.ClassificationString = GetClassificationString(Rec.Classification);
				Rec.SerializedNodeIndex = S.NodeIndex;
				Rec.RuntimeNodeIndex = R.NodeIndex;
				Rec.SerializedObjectPath = S.ObjectPath;
				Rec.RuntimeObjectPath = R.ObjectPath;
				Rec.SerializedWidgetName = S.WidgetName;
				Rec.RuntimeWidgetName = R.WidgetName;
				Rec.SerializedClass = S.WidgetClass;
				Rec.RuntimeWidgetClass = R.WidgetClass;
				Rec.RuntimeSlateType = R.SlateWidgetType;
				Rec.SerializedParent = S.ParentName;
				Rec.RuntimeParent = R.ParentName;
				Rec.SerializedChildOrder = S.ChildIndex;
				Rec.RuntimeChildOrder = R.ChildIndex;

				Rec.Evidence.Add(FString::Printf(TEXT("Exact source widget name: '%s'"), *S.WidgetName));
				Rec.Evidence.Add(FString::Printf(TEXT("Compatible widget class: '%s'"), *S.WidgetClass));

				// Parent chain evidence
				bool bParentMatches = CheckParentMatches(S, R);

				if (bParentMatches)
				{
					Rec.Evidence.Add(FString::Printf(TEXT("Matching parent container: '%s'"), *S.ParentName));
				}
				else
				{
					Rec.ConflictingEvidence.Add(FString::Printf(TEXT("Parent mismatch: Serialized is '%s', Runtime is '%s'"), *S.ParentName, *R.ParentName));
					FString Disc = FString::Printf(TEXT("parent or containment mismatch: '%s' (Serialized parent '%s' vs Runtime parent '%s')"), *S.WidgetName, *S.ParentName, *R.ParentName);
					Rec.Discrepancies.Add(Disc);
					OutAllDiscrepancies.Add(Disc);
				}

				// Child order evidence
				if (S.ChildIndex >= 0 && R.ChildIndex >= 0)
				{
					if (S.ChildIndex == R.ChildIndex)
					{
						Rec.Evidence.Add(FString::Printf(TEXT("Matching child container order: %d"), S.ChildIndex));
					}
					else
					{
						Rec.ConflictingEvidence.Add(FString::Printf(TEXT("Child order mismatch: Serialized index %d vs Runtime index %d"), S.ChildIndex, R.ChildIndex));
						FString Disc = FString::Printf(TEXT("child-order mismatch: '%s' (Serialized index %d vs Runtime index %d)"), *S.WidgetName, S.ChildIndex, R.ChildIndex);
						Rec.Discrepancies.Add(Disc);
						OutAllDiscrepancies.Add(Disc);
					}
				}

				// Supporting evidence: slot type
				if (S.bHasSlot && R.bHasSlot && S.Slot.SlotType.Equals(R.Slot.SlotType, ESearchCase::IgnoreCase))
				{
					Rec.Evidence.Add(FString::Printf(TEXT("Supporting evidence: matching slot type '%s'"), *S.Slot.SlotType));
				}

				// Supporting evidence: accessible text
				if (!S.AccessibleName.IsEmpty() && S.AccessibleName.Equals(R.AccessibleName, ESearchCase::IgnoreCase))
				{
					Rec.Evidence.Add(FString::Printf(TEXT("Supporting evidence: matching accessible name \"%s\""), *S.AccessibleName));
				}

				// Check property discrepancies
				DetectDiscrepancies(S, R, Rec, OutAllDiscrepancies);

				OutRecords.Add(Rec);
				MatchedS.Add(s);
				MatchedR.Add(r);
			}
		}
	}

	// Second pass: Disambiguation by parent for multi-candidate matches
	for (int32 s = 0; s < SNum; ++s)
	{
		if (MatchedS.Contains(s) || AmbiguousS.Contains(s))
		{
			continue;
		}

		const FSerializedWidgetRecord& S = SerializedReport.Widgets[s];
		const TArray<int32>* CandR = SCandidates.Find(s);

		if (!CandR || CandR->Num() == 0)
		{
			continue;
		}

		// Filter available runtime candidates by matching parent
		TArray<int32> ParentFilteredCandidates;
		for (int32 r : *CandR)
		{
			if (MatchedR.Contains(r))
			{
				continue;
			}

			const FRuntimeWidgetRecord& R = RuntimeReport.Widgets[r];
			bool bParentMatches = CheckParentMatches(S, R);

			if (bParentMatches)
			{
				ParentFilteredCandidates.Add(r);
			}
		}

		if (ParentFilteredCandidates.Num() == 1)
		{
			int32 r = ParentFilteredCandidates[0];
			const FRuntimeWidgetRecord& R = RuntimeReport.Widgets[r];

			// Check if R also has multiple serialized candidates with matching parent
			TArray<int32> ParentFilteredSerialized;
			const TArray<int32>* CandS = RCandidates.Find(r);
			if (CandS)
			{
				for (int32 otherS : *CandS)
				{
					if (MatchedS.Contains(otherS))
					{
						continue;
					}
					if (CheckParentMatches(SerializedReport.Widgets[otherS], R))
					{
						ParentFilteredSerialized.Add(otherS);
					}
				}
			}

			if (ParentFilteredSerialized.Num() <= 1)
			{
				FWidgetComparisonRecord Rec;
				Rec.Classification = EWidgetComparisonClassification::Matched;
				Rec.ClassificationString = GetClassificationString(Rec.Classification);
				Rec.SerializedNodeIndex = S.NodeIndex;
				Rec.RuntimeNodeIndex = R.NodeIndex;
				Rec.SerializedObjectPath = S.ObjectPath;
				Rec.RuntimeObjectPath = R.ObjectPath;
				Rec.SerializedWidgetName = S.WidgetName;
				Rec.RuntimeWidgetName = R.WidgetName;
				Rec.SerializedClass = S.WidgetClass;
				Rec.RuntimeWidgetClass = R.WidgetClass;
				Rec.RuntimeSlateType = R.SlateWidgetType;
				Rec.SerializedParent = S.ParentName;
				Rec.RuntimeParent = R.ParentName;
				Rec.SerializedChildOrder = S.ChildIndex;
				Rec.RuntimeChildOrder = R.ChildIndex;

				Rec.Evidence.Add(FString::Printf(TEXT("Exact source widget name: '%s'"), *S.WidgetName));
				Rec.Evidence.Add(FString::Printf(TEXT("Compatible widget class: '%s'"), *S.WidgetClass));
				Rec.Evidence.Add(FString::Printf(TEXT("Disambiguated by matching parent container: '%s'"), *S.ParentName));

				DetectDiscrepancies(S, R, Rec, OutAllDiscrepancies);

				OutRecords.Add(Rec);
				MatchedS.Add(s);
				MatchedR.Add(r);
			}
			else
			{
				// Multiple serialized nodes match this runtime node by parent
				for (int32 ambigS : ParentFilteredSerialized)
				{
					if (AmbiguousS.Contains(ambigS))
					{
						continue;
					}
					const FSerializedWidgetRecord& SRec = SerializedReport.Widgets[ambigS];
					FWidgetComparisonRecord Rec;
					Rec.Classification = EWidgetComparisonClassification::Ambiguous;
					Rec.ClassificationString = GetClassificationString(Rec.Classification);
					Rec.SerializedNodeIndex = SRec.NodeIndex;
					Rec.SerializedObjectPath = SRec.ObjectPath;
					Rec.SerializedWidgetName = SRec.WidgetName;
					Rec.SerializedClass = SRec.WidgetClass;
					Rec.SerializedParent = SRec.ParentName;
					Rec.SerializedChildOrder = SRec.ChildIndex;

					Rec.ConflictingEvidence.Add(FString::Printf(TEXT("Multiple serialized widgets match runtime candidate '%s' (%s)"), *R.WidgetName, *R.ObjectPath));
					FString Disc = FString::Printf(TEXT("ambiguous identity: multiple serialized widgets compete for runtime widget '%s'"), *SRec.WidgetName);
					Rec.Discrepancies.Add(Disc);
					OutAllDiscrepancies.Add(Disc);

					OutRecords.Add(Rec);
					AmbiguousS.Add(ambigS);
				}
				AmbiguousR.Add(r);
			}
		}
		else if (ParentFilteredCandidates.Num() > 1 || CandR->Num() > 1)
		{
			// Ambiguous: multiple runtime candidates exist and cannot be uniquely disambiguated
			FWidgetComparisonRecord Rec;
			Rec.Classification = EWidgetComparisonClassification::Ambiguous;
			Rec.ClassificationString = GetClassificationString(Rec.Classification);
			Rec.SerializedNodeIndex = S.NodeIndex;
			Rec.SerializedObjectPath = S.ObjectPath;
			Rec.SerializedWidgetName = S.WidgetName;
			Rec.SerializedClass = S.WidgetClass;
			Rec.SerializedParent = S.ParentName;
			Rec.SerializedChildOrder = S.ChildIndex;

			TArray<int32> AllCands = (ParentFilteredCandidates.Num() > 1) ? ParentFilteredCandidates : *CandR;
			TArray<FString> CandPaths;
			for (int32 CandRIdx : AllCands)
			{
				CandPaths.Add(RuntimeReport.Widgets[CandRIdx].ObjectPath);
				AmbiguousR.Add(CandRIdx);
			}

			Rec.ConflictingEvidence.Add(FString::Printf(TEXT("Ambiguous runtime candidates for '%s': [%s]"), *S.WidgetName, *FString::Join(CandPaths, TEXT(", "))));
			FString Disc = FString::Printf(TEXT("ambiguous identity: multiple runtime candidates found for serialized widget '%s'"), *S.WidgetName);
			Rec.Discrepancies.Add(Disc);
			OutAllDiscrepancies.Add(Disc);

			OutRecords.Add(Rec);
			AmbiguousS.Add(s);
		}
	}

	// Third pass: Class mismatch conflicts
	for (auto& Pair : ClassMismatchMap)
	{
		int32 s = Pair.Key;
		if (MatchedS.Contains(s) || AmbiguousS.Contains(s))
		{
			continue;
		}

		const FSerializedWidgetRecord& S = SerializedReport.Widgets[s];
		for (int32 r : Pair.Value)
		{
			if (MatchedR.Contains(r))
			{
				continue;
			}
			const FRuntimeWidgetRecord& R = RuntimeReport.Widgets[r];

			FWidgetComparisonRecord Rec;
			Rec.Classification = EWidgetComparisonClassification::Ambiguous;
			Rec.ClassificationString = GetClassificationString(Rec.Classification);
			Rec.SerializedNodeIndex = S.NodeIndex;
			Rec.RuntimeNodeIndex = R.NodeIndex;
			Rec.SerializedWidgetName = S.WidgetName;
			Rec.RuntimeWidgetName = R.WidgetName;
			Rec.SerializedClass = S.WidgetClass;
			Rec.RuntimeWidgetClass = R.WidgetClass;
			Rec.SerializedParent = S.ParentName;
			Rec.RuntimeParent = R.ParentName;

			Rec.ConflictingEvidence.Add(FString::Printf(TEXT("Class mismatch prevents exact match: Serialized '%s' is class '%s', Runtime '%s' is class '%s'"),
				*S.WidgetName, *S.WidgetClass, *R.WidgetName, *R.WidgetClass));
			FString Disc = FString::Printf(TEXT("ambiguous identity: class mismatch between '%s' (%s) and runtime (%s)"), *S.WidgetName, *S.WidgetClass, *R.WidgetClass);
			Rec.Discrepancies.Add(Disc);
			OutAllDiscrepancies.Add(Disc);

			OutRecords.Add(Rec);
			AmbiguousS.Add(s);
			AmbiguousR.Add(r);
		}
	}

	// Fourth pass: Unmatched serialized nodes -> SerializedOnly
	for (int32 s = 0; s < SNum; ++s)
	{
		if (MatchedS.Contains(s) || AmbiguousS.Contains(s))
		{
			continue;
		}

		const FSerializedWidgetRecord& S = SerializedReport.Widgets[s];

		FWidgetComparisonRecord Rec;
		Rec.Classification = EWidgetComparisonClassification::SerializedOnly;
		Rec.ClassificationString = GetClassificationString(Rec.Classification);
		Rec.SerializedNodeIndex = S.NodeIndex;
		Rec.SerializedObjectPath = S.ObjectPath;
		Rec.SerializedWidgetName = S.WidgetName;
		Rec.SerializedClass = S.WidgetClass;
		Rec.SerializedParent = S.ParentName;
		Rec.SerializedChildOrder = S.ChildIndex;

		if (S.DefaultVisibility.Equals(TEXT("Collapsed"), ESearchCase::IgnoreCase) || S.DefaultVisibility.Equals(TEXT("Hidden"), ESearchCase::IgnoreCase))
		{
			Rec.Evidence.Add(FString::Printf(TEXT("Valid SerializedOnly: authored with default visibility '%s' and unconstructed in live viewport"), *S.DefaultVisibility));
		}
		else
		{
			Rec.Evidence.Add(TEXT("Declared in Widget Blueprint asset"));
			Rec.UnavailableEvidence.Add(TEXT("No live runtime counterpart instantiated"));
			FString Disc = FString::Printf(TEXT("serialized-only widget: '%s' (%s) is declared in blueprint but absent in runtime viewport"), *S.WidgetName, *S.WidgetClass);
			Rec.Discrepancies.Add(Disc);
			OutAllDiscrepancies.Add(Disc);
		}

		OutRecords.Add(Rec);
	}

	// Fifth pass: Unmatched runtime nodes -> RuntimeOnly
	for (int32 r = 0; r < RNum; ++r)
	{
		if (MatchedR.Contains(r) || AmbiguousR.Contains(r))
		{
			continue;
		}

		const FRuntimeWidgetRecord& R = RuntimeReport.Widgets[r];

		FWidgetComparisonRecord Rec;
		Rec.Classification = EWidgetComparisonClassification::RuntimeOnly;
		Rec.ClassificationString = GetClassificationString(Rec.Classification);
		Rec.RuntimeNodeIndex = R.NodeIndex;
		Rec.RuntimeObjectPath = R.ObjectPath;
		Rec.RuntimeWidgetName = R.WidgetName;
		Rec.RuntimeWidgetClass = R.WidgetClass;
		Rec.RuntimeSlateType = R.SlateWidgetType;
		Rec.RuntimeParent = R.ParentName;
		Rec.RuntimeChildOrder = R.ChildIndex;

		Rec.Evidence.Add(TEXT("Present in live runtime hierarchy"));
		Rec.UnavailableEvidence.Add(TEXT("No corresponding Widget Blueprint asset declaration located"));

		FString Disc = FString::Printf(TEXT("runtime-only widget: '%s' (%s) is dynamically created or an internal Slate composition node"), *R.WidgetName, *R.WidgetClass);
		Rec.Discrepancies.Add(Disc);
		OutAllDiscrepancies.Add(Disc);

		OutRecords.Add(Rec);
	}

	// Deterministic sorting: Ambiguous first, then Matched, then SerializedOnly, then RuntimeOnly
	OutRecords.Sort([](const FWidgetComparisonRecord& A, const FWidgetComparisonRecord& B)
	{
		auto GetRank = [](EWidgetComparisonClassification C) -> int32
		{
			switch (C)
			{
			case EWidgetComparisonClassification::Ambiguous:      return 0;
			case EWidgetComparisonClassification::Matched:        return 1;
			case EWidgetComparisonClassification::SerializedOnly: return 2;
			case EWidgetComparisonClassification::RuntimeOnly:    return 3;
			case EWidgetComparisonClassification::Unavailable:    return 4;
			case EWidgetComparisonClassification::Unsupported:    return 5;
			default: return 6;
			}
		};

		int32 RankA = GetRank(A.Classification);
		int32 RankB = GetRank(B.Classification);

		if (RankA != RankB)
		{
			return RankA < RankB;
		}

		if (A.SerializedNodeIndex >= 0 && B.SerializedNodeIndex >= 0)
		{
			return A.SerializedNodeIndex < B.SerializedNodeIndex;
		}

		if (A.RuntimeNodeIndex >= 0 && B.RuntimeNodeIndex >= 0)
		{
			return A.RuntimeNodeIndex < B.RuntimeNodeIndex;
		}

		return A.SerializedWidgetName < B.SerializedWidgetName;
	});
}

void FDifferentialComparator::DetectDiscrepancies(
	const FSerializedWidgetRecord& S,
	const FRuntimeWidgetRecord& R,
	FWidgetComparisonRecord& OutRecord,
	TArray<FString>& OutAllDiscrepancies)
{
	// Visibility mismatch
	FString CompVis = !R.LocalVisibility.IsEmpty() ? R.LocalVisibility : R.RuntimeVisibility;
	if (!R.RuntimeVisibility.IsEmpty() && !R.RuntimeVisibility.Equals(R.LocalVisibility, ESearchCase::IgnoreCase))
	{
		CompVis = R.RuntimeVisibility;
	}
	if (CompVis.IsEmpty())
	{
		CompVis = TEXT("Visible");
	}

	bool bReportVisibilityMismatch = true;
	if (!S.DefaultVisibility.Equals(CompVis, ESearchCase::IgnoreCase))
	{
		// Rule 3: Do not report visibility mismatch when child is locally Visible but ancestor is effectively Collapsed or Hidden
		if (!R.bEffectivelyVisible && CompVis.Equals(TEXT("Visible"), ESearchCase::IgnoreCase))
		{
			bReportVisibilityMismatch = false;
		}

		if (bReportVisibilityMismatch)
		{
			FString Disc = FString::Printf(TEXT("visibility mismatch: '%s' (Serialized default '%s' vs Runtime '%s')"), *S.WidgetName, *S.DefaultVisibility, *CompVis);
			OutRecord.Discrepancies.Add(Disc);
			OutAllDiscrepancies.Add(Disc);
		}
	}

	// Add effective runtime visibility evidence separately
	OutRecord.Evidence.Add(FString::Printf(TEXT("Effective runtime visibility: '%s' (Local: '%s'%s)"),
		*R.EffectiveVisibility,
		*R.LocalVisibility,
		R.BlockingAncestorIndex >= 0 ? *FString::Printf(TEXT(", blocked by '%s'"), *R.BlockingAncestorName) : TEXT("")));

	// Enabled state mismatch
	if (S.bIsEnabled != R.bIsEnabled)
	{
		FString Disc = FString::Printf(TEXT("enabled-state mismatch: '%s' (Serialized %s vs Runtime %s)"),
			*S.WidgetName,
			S.bIsEnabled ? TEXT("Enabled") : TEXT("Disabled"),
			R.bIsEnabled ? TEXT("Enabled") : TEXT("Disabled"));
		OutRecord.Discrepancies.Add(Disc);
		OutAllDiscrepancies.Add(Disc);
	}

	// Focusability mismatch
	if (S.bIsFocusable != R.bIsFocusable)
	{
		FString Disc = FString::Printf(TEXT("focusability mismatch: '%s' (Serialized %s vs Runtime %s)"),
			*S.WidgetName,
			S.bIsFocusable ? TEXT("Focusable") : TEXT("Not Focusable"),
			R.bIsFocusable ? TEXT("Focusable") : TEXT("Not Focusable"));
		OutRecord.Discrepancies.Add(Disc);
		OutAllDiscrepancies.Add(Disc);
	}

	// Accessible name mismatch
	if (!S.AccessibleName.IsEmpty() && !S.AccessibleName.Equals(R.AccessibleName, ESearchCase::IgnoreCase))
	{
		FString Disc = FString::Printf(TEXT("accessible-name mismatch: '%s' (Serialized \"%s\" vs Runtime \"%s\")"),
			*S.WidgetName, *S.AccessibleName, *R.AccessibleName);
		OutRecord.Discrepancies.Add(Disc);
		OutAllDiscrepancies.Add(Disc);
	}

	// Accessible summary mismatch
	if (!S.AccessibleSummary.IsEmpty() && !S.AccessibleSummary.Equals(R.AccessibleSummary, ESearchCase::IgnoreCase))
	{
		FString Disc = FString::Printf(TEXT("accessible-summary mismatch: '%s' (Serialized \"%s\" vs Runtime \"%s\")"),
			*S.WidgetName, *S.AccessibleSummary, *R.AccessibleSummary);
		OutRecord.Discrepancies.Add(Disc);
		OutAllDiscrepancies.Add(Disc);
	}

	// Navigation rules mismatch
	if (S.NavigationRules.Num() != R.NavigationRules.Num())
	{
		FString Disc = FString::Printf(TEXT("navigation mismatch: '%s' has %d serialized rules vs %d runtime rules"),
			*S.WidgetName, S.NavigationRules.Num(), R.NavigationRules.Num());
		OutRecord.Discrepancies.Add(Disc);
		OutAllDiscrepancies.Add(Disc);
	}
	else
	{
		for (int32 i = 0; i < S.NavigationRules.Num(); ++i)
		{
			const FAccessibleNavigationRecord& SNav = S.NavigationRules[i];
			const FAccessibleNavigationRecord& RNav = R.NavigationRules[i];
			if (!SNav.Direction.Equals(RNav.Direction, ESearchCase::IgnoreCase) ||
				!SNav.Rule.Equals(RNav.Rule, ESearchCase::IgnoreCase) ||
				!SNav.TargetWidget.Equals(RNav.TargetWidget, ESearchCase::IgnoreCase))
			{
				FString Disc = FString::Printf(TEXT("navigation mismatch: '%s' rule for '%s' differs (Serialized: %s->%s vs Runtime: %s->%s)"),
					*S.WidgetName, *SNav.Direction, *SNav.Rule, *SNav.TargetWidget, *RNav.Rule, *RNav.TargetWidget);
				OutRecord.Discrepancies.Add(Disc);
				OutAllDiscrepancies.Add(Disc);
				break;
			}
		}
	}

	// Unavailable geometry: only report for effectively visible controls that lack valid screen bounds
	if (R.bEffectivelyVisible && R.bHasSlateWidget && (!R.bEffectiveGeometryAvailable || !R.ScreenBounds.bIsValid))
	{
		FString Disc = FString::Printf(TEXT("unavailable geometry: '%s' is effectively visible with cached Slate widget but zero or uncalculated screen bounds"), *R.WidgetName);
		OutRecord.Discrepancies.Add(Disc);
		OutAllDiscrepancies.Add(Disc);
		OutRecord.UnavailableEvidence.Add(TEXT("Screen-space bounds unavailable for effectively visible widget"));
	}
}

void FDifferentialComparator::DetectGlobalRuntimeDiscrepancies(
	const FRuntimeWidgetReport& RuntimeReport,
	TArray<FWidgetComparisonRecord>& ComparisonRecords,
	TArray<FString>& OutAllDiscrepancies)
{
	// 1. Check for overlapping interactive runtime bounds: only evaluate effectively visible, interactive, unblocked controls
	TArray<const FRuntimeWidgetRecord*> InteractiveRecords;
	for (const FRuntimeWidgetRecord& R : RuntimeReport.Widgets)
	{
		if (R.bEffectivelyVisible && R.bEffectivelyInteractive && R.bEffectiveGeometryAvailable &&
			R.BlockingAncestorIndex == -1 && R.ScreenBounds.bIsValid && R.ScreenBounds.Width > 0.0f && R.ScreenBounds.Height > 0.0f)
		{
			InteractiveRecords.Add(&R);
		}
	}

	for (int32 i = 0; i < InteractiveRecords.Num(); ++i)
	{
		for (int32 j = i + 1; j < InteractiveRecords.Num(); ++j)
		{
			const FRuntimeWidgetRecord* A = InteractiveRecords[i];
			const FRuntimeWidgetRecord* B = InteractiveRecords[j];

			// Exclude parent-child containment
			if (A->NodeIndex == B->ParentIndex || B->NodeIndex == A->ParentIndex)
			{
				continue;
			}

			// Simple AABB overlap check with threshold
			float MinX = FMath::Max(A->ScreenBounds.X, B->ScreenBounds.X);
			float MaxX = FMath::Min(A->ScreenBounds.X + A->ScreenBounds.Width, B->ScreenBounds.X + B->ScreenBounds.Width);
			float MinY = FMath::Max(A->ScreenBounds.Y, B->ScreenBounds.Y);
			float MaxY = FMath::Min(A->ScreenBounds.Y + A->ScreenBounds.Height, B->ScreenBounds.Y + B->ScreenBounds.Height);

			if (MaxX > MinX && MaxY > MinY)
			{
				float InterW = MaxX - MinX;
				float InterH = MaxY - MinY;
				float InterArea = InterW * InterH;
				float AreaA = A->ScreenBounds.Width * A->ScreenBounds.Height;
				float AreaB = B->ScreenBounds.Width * B->ScreenBounds.Height;
				float MinArea = FMath::Min(AreaA, AreaB);

				if (InterW > 4.0f && InterH > 4.0f && MinArea > 0.0f && (InterArea / MinArea) > 0.10f)
				{
					FString Disc = FString::Printf(TEXT("overlapping runtime bounds: interactive widgets '%s' and '%s' occupy overlapping screen space (%.1f, %.1f)"),
						*A->WidgetName, *B->WidgetName, InterW, InterH);
					OutAllDiscrepancies.Add(Disc);

					// Add to corresponding comparison records if present
					for (FWidgetComparisonRecord& Rec : ComparisonRecords)
					{
						if (Rec.RuntimeNodeIndex == A->NodeIndex || Rec.RuntimeNodeIndex == B->NodeIndex)
						{
							Rec.Discrepancies.Add(Disc);
						}
					}
				}
			}
		}
	}

	// 2. Check for stale, invalid, or detached runtime records
	for (const FRuntimeWidgetRecord& R : RuntimeReport.Widgets)
	{
		if (R.ObjectPath.IsEmpty())
		{
			FString Disc = FString::Printf(TEXT("stale, invalid, or detached runtime record: widget '%s' has an empty or detached object path"), *R.WidgetName);
			OutAllDiscrepancies.Add(Disc);
			for (FWidgetComparisonRecord& Rec : ComparisonRecords)
			{
				if (Rec.RuntimeNodeIndex == R.NodeIndex)
				{
					Rec.Discrepancies.Add(Disc);
				}
			}
		}
	}
}

FString FDifferentialComparator::GetClassificationString(EWidgetComparisonClassification Classification)
{
	switch (Classification)
	{
	case EWidgetComparisonClassification::Matched:        return TEXT("Matched");
	case EWidgetComparisonClassification::Ambiguous:      return TEXT("Ambiguous");
	case EWidgetComparisonClassification::SerializedOnly: return TEXT("SerializedOnly");
	case EWidgetComparisonClassification::RuntimeOnly:    return TEXT("RuntimeOnly");
	case EWidgetComparisonClassification::Unavailable:    return TEXT("Unavailable");
	case EWidgetComparisonClassification::Unsupported:    return TEXT("Unsupported");
	default:                                              return TEXT("Unknown");
	}
}
