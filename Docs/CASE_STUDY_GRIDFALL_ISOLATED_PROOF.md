# Case Study: Isolated GridFall Integration & Proving Proof

## 1. Executive Summary
This case study documents the end-to-end integration and proving proof conducted during Milestone 5 of Accessible UMG Inspector. The integration was executed in an isolated workspace against a complete multi-screen game shell (`WBP_GameShell`), proving that the plugin operates as an authoritative, read-only inspection layer in a production Unreal Engine 5.8.2 game environment.

---

## 2. Isolated Proving Environment
To ensure strict safety and preserve project boundaries:
- **Authoritative Working Repository**: `C:\BLS\GridFall` was completely isolated and never accessed, launched, built, configured, or modified.
- **Dedicated Proving Copy**: `C:\Users\jerem\Desktop\GridFall-UMGToolSet-Probe` served as the single authorized integration environment.
- **Transport**: Native Unreal MCP endpoint at `127.0.0.1:8000` via `ToolsetRegistry`.
- **Target Asset**: `/Game/GridFall/UI/WBP_GameShell.uasset` (Size: 180,604 bytes; SHA-256: `C85CBA10F6A60F339B9E97A56B02B0398E6740C400546BD8B10145612E719847`).

---

## 3. Source-Registration Correction
During initial deployment, `UAccessibleUMGInspectorToolset` inherited from `UToolsetDefinition`, but was not automatically discovered by MCP `list_toolsets`.
- **Root Cause**: Toolset classes require explicit registration with Epic's `UToolsetRegistry`.
- **Correction**: In `AccessibleUMGInspectorModule.cpp`, registered `UAccessibleUMGInspectorToolset::StaticClass()` in `StartupModule()` and unregistered it in `ShutdownModule()`.
- **Result**: MCP server instantly discovered `AccessibleUMGInspector.AccessibleUMGInspectorToolset` alongside Epic's built-in toolsets without requiring engine modifications.

---

## 4. Phase Verification Evidence

### Phase A: Serialized Inspection (`InspectSerializedWidgetBlueprint`)
- **Target**: `/Game/GridFall/UI/WBP_GameShell`
- **Result**: `bSuccess = true`, `Availability = Available`.
- **Discovery**: 43 serialized widgets cataloged, rooted at `CanvasPanel_0`.
- **Package Invariance**: `bPackageWasCleanBefore = true`, `bPackageRemainsCleanAfter = true`.
- **Asset Integrity**: Asset SHA-256 remained identical post-inspection.

### Phase B: Runtime Inspection (`InspectRuntimeUserWidget`)
- **Execution Mode**: In-process Viewport PIE (`bSimulate=false`, `PlayMode_InViewPort`, 1.0s warmup).
- **Target**: Single top-level user widget resolved in active PIE world (`WBP_GameShell_C_0`).
- **Result**: `bSuccess = true`, `Availability = Available`.
- **Root**: Resolved directly at `WBP_GameShell_C_0`, strictly avoiding Editor chrome / SWindow ancestry.
- **Control Breakdown**:
  - Total runtime nodes: 46.
  - Active nodes: 38.
  - Inactive / Collapsed subtrees: 8.
  - Passive Focus: `Btn_HostGame` (`[Focusable] [Focused]`, Bounds: X=797, Y=604, W=453, H=70).
  - Collapsed Containers Identified: `Panel_HostScreen`, `Panel_JoinScreen`, `Panel_SettingsScreen`, `Modal_ExitConfirmation`.

### Phase C: Differential Comparison (`CompareSerializedAndRuntime`)
- **Pairing**: `/Game/GridFall/UI/WBP_GameShell` vs `WBP_GameShell_C_0`.
- **Reconciliation Counts**:
  - `MatchedCount`: 43 (100% of serialized nodes reconciled to runtime counterpart).
  - `AmbiguousCount`: 0.
  - `SerializedOnlyCount`: 0.
  - `RuntimeOnlyCount`: 3 (Top-level user widget instance wrapper and internal composition nodes).
- **Discrepancies Observed**: Navigation metadata dynamically bound at runtime (4 explicit navigation rules vs 0 serialized defaults).

---

## 5. The 44 False-Positive Audit Errors & Root Cause Analysis

### The Initial Audit
In Phase D of the initial Milestone 5 proof, `AuditWidgetAccessibility` reported **44 errors** on the stable start screen.

### Root Cause: Local vs Effective Visibility Conflation
Detailed investigation revealed that the audit engine conflated local widget visibility with effective runtime exposure:
1. In Epic's UMG/Slate architecture, child controls retain their authored local `Visibility = Visible` property even when an ancestor panel (`Panel_HostScreen`, `Panel_JoinScreen`, `Modal_ExitConfirmation`) is `Collapsed`.
2. When an ancestor is `Collapsed`, Slate suppresses the entire subtree from rendering, layout allocation, hit-testing, pointer routing, and accessibility tree inclusion.
3. Because the initial audit rules checked only local `Visibility == Visible`:
   - `A11Y-06` flagged every descendant button inside collapsed panels as an error.
   - `A11Y-04` compared child labels across collapsed inactive screens against active controls.
   - `A11Y-13` flagged children under `NotAccessible` containers that were already collapsed.
   - `DifferentialComparator` generated false visibility and geometry mismatches.

---

## 6. The Two-Tier Visibility Solution & Replay Verification

### Two-Tier Model Implementation
Accessible UMG Inspector was updated with an authoritative two-tier visibility architecture:
- `LocalVisibility`: Direct local property declared on the widget.
- `EffectiveVisibility`: Derived state computed via deterministic top-down ancestor propagation (`ComputeEffectiveVisibility`).
- State flags: `bEffectivelyVisible`, `bEffectivelyInteractive`, `bEffectivelyFocusable`, `BlockingAncestorIndex`.

### Synthetic Replay Verification
An automated synthetic replay test (`Milestone5_Replay_GameShell_Audit_ZeroErrors`) modeled the exact structural hierarchy of `WBP_GameShell`:
- Active `Panel_StartScreen` controls.
- Collapsed `Panel_HostScreen`, `Panel_JoinScreen`, and `Modal_ExitConfirmation` containers with local `Visible` children.
- **Result**: The corrected audit engine produced **exactly 0 errors** on the stable start screen, successfully resolving all 44 false positives while preserving local authored intent.

---

## 7. Explicit Architectural Limitations & Non-Claims

1. **No Screen Transition Testing**:
   - The Milestone 5 proof was strictly authorized for the stable initial start screen.
   - **No Host, Join, Settings, Exit, or Back transitions** have been inspected or evaluated with the corrected inspector.
2. **No Crash-Cause Diagnosis**:
   - During separate gameplay testing in the original project, three PIE sessions experienced crashes during screen transitions.
   - Accessible UMG Inspector makes **no claim** that those crashes have been diagnosed, explained, or resolved by this tooling.
   - Audit findings provide structural, visibility, and property evidence; they do not diagnose engine-level memory faults or Slate crash causality.
