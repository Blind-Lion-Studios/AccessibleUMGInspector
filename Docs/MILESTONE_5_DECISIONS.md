# Milestone 5 Decisions & Proving Report: Isolated GridFall Integration

## 1. Overview & Objective
Milestone 5 executed an isolated integration and live runtime proof of Accessible UMG Inspector against the copied GridFall workspace (`C:\Users\jerem\Desktop\GridFall-UMGToolSet-Probe`) and its primary game shell asset (`WBP_GameShell`), without touching, launching, or modifying the authoritative working repository (`C:\BLS\GridFall`).

All operations were executed through Epic's installed Unreal MCP endpoint (`127.0.0.1:8000`) and the native `ToolsetRegistry`.

---

## 2. Key Architecture & Registration Decisions

### 2.1 Native MCP Discovery via `UToolsetRegistry`
- **Issue**: Although `UAccessibleUMGInspectorToolset` inherited from `UToolsetDefinition`, it was not automatically listed in MCP `list_toolsets` until explicitly registered.
- **Resolution**: In `AccessibleUMGInspectorModule.cpp`, registered `UAccessibleUMGInspectorToolset::StaticClass()` via `UToolsetRegistry::RegisterToolsetClass` on `StartupModule()`, and unregistered via `UToolsetRegistry::UnregisterToolsetClass` on `ShutdownModule()`.
- **Result**: MCP server instantly discovered `AccessibleUMGInspector.AccessibleUMGInspectorToolset` alongside Epic's `UMGToolSet`, `SlateInspectorToolset`, and `EditorToolset`.

### 2.2 Strict In-Process Viewport PIE Without Mutation
- PIE was launched in-process (`bSimulate=false`, `PlayMode_InViewPort`, 1.0s warmup) using `EditorToolset.EditorAppToolset.StartPIE`.
- At no point were synthetic inputs dispatched, buttons clicked (Host, Join, Settings, Exit, Back), or transitions triggered.
- Inspection occurred exclusively on the stable initial Start screen.
- PIE was stopped cleanly via `EditorToolset.EditorAppToolset.StopPIE`.

---

## 3. Phase Verification Results

### Phase A: Serialized Proof (`InspectSerializedWidgetBlueprint`)
- **Asset**: `/Game/GridFall/UI/WBP_GameShell`
- **Result**: `bSuccess = true`
- **Root**: `CanvasPanel_0` (CanvasPanel)
- **Total Widgets**: 43 serialized widgets discovered and cataloged.
- **Controls Verified**: `Btn_HostGame`, `Btn_JoinGame`, `Btn_Settings`, `Btn_Exit`, `Panel_StartScreen`, and collapsed modal screens.
- **Package Invariance**: `bPackageWasCleanBefore = true`, `bPackageRemainsCleanAfter = true`.
- **Asset SHA-256**: Baseline hash `C85CBA10F6A60F339B9E97A56B02B0398E6740C400546BD8B10145612E719847` preserved identically.

### Phase B: Runtime Proof (`InspectRuntimeUserWidget`)
- **Target**: Single top-level user widget resolved in active PIE world (`WBP_GameShell_C_0`).
- **Result**: `bSuccess = true`
- **Root**: `WBP_GameShell_C_0` (UserWidget root, strictly avoiding Editor chrome / SWindow ancestry).
- **Control Breakdown**:
  - Total runtime nodes: 46
  - Active nodes: 38
  - Inactive / Collapsed subtrees: 8
  - Initially focused widget: `Btn_HostGame` (`[Focusable] [Focused]`, Bounds: X=797, Y=604, W=453, H=70).
- **Inactive Panels Categorized**: `Panel_HostScreen`, `Panel_JoinScreen`, `Panel_SettingsScreen`, `Modal_ExitConfirmation`, status and failure texts.

### Phase C: Differential Comparison (`CompareSerializedAndRuntime`)
- **Pairing**: `/Game/GridFall/UI/WBP_GameShell` vs `WBP_GameShell_C_0`
- **Result**: `bSuccess = true`, `Availability = Available`
- **Reconciliation Counts**:
  - `MatchedCount`: 43 (100% of serialized nodes reconciled to runtime counterpart)
  - `AmbiguousCount`: 0
  - `SerializedOnlyCount`: 0
  - `RuntimeOnlyCount`: 3 (Top-level user widget wrapper and dynamic runtime instances)
- **Discrepancies Observed**: Navigation metadata dynamically bound at runtime (4 explicit navigation rules vs 0 serialized defaults); runtime visibility shifts for panels.

### Phase D: Accessibility Audit (`AuditWidgetAccessibility`)
- **Evidence Mode**: `Differential` (evaluating serialized, runtime, and comparison evidence concurrently).
- **Rule Set**: All 14 accessibility rules (`A11Y-01` through `A11Y-14`).
- **Result**: `bSuccess = true`, `Availability = Available`, 0 unhandled crashes or exceptions.
- **Rule Breakdown**:
  - `ErrorCount`: 44
  - `WarningCount`: 12
  - `InfoCount`: 0
  - `PassedCount`: 8
  - `NotEvaluatedCount`: 0
- **Key Architectural Findings**:
  - `A11Y-06` (CollapsedScreenRuntimeDescendant): Caught major latent issue where descendant buttons (`Btn_HostCancel`, `Btn_JoinCancel`, `Btn_JoinManual`, `Btn_RefreshSessions`, `Btn_ExitCancel`, `Btn_ExitConfirm`) remain marked `Visible` inside collapsed containers (`Panel_HostScreen`, `Panel_JoinScreen`, `Modal_ExitConfirmation`).
  - `A11Y-04` (DuplicateAccessibleName): Flagged duplicate "ready" status texts across simultaneous components.
  - Zero false claims regarding hit-test grid, OS UIA tree, or crash causality.

---

## 4. Post-Execution Evidence & Git State

### Baseline Asset Invariance
- Original `C:\BLS\GridFall\Content\GridFall\UI\WBP_GameShell.uasset`:
  - Size: 180,604 bytes
  - LastWriteTime: `2026-09-26T21:57:00.6008667-04:00`
  - SHA-256: `C85CBA10F6A60F339B9E97A56B02B0398E6740C400546BD8B10145612E719847` (Exact match)
- Probe `C:\Users\jerem\Desktop\GridFall-UMGToolSet-Probe\Content\GridFall\UI\WBP_GameShell.uasset`:
  - SHA-256: `C85CBA10F6A60F339B9E97A56B02B0398E6740C400546BD8B10145612E719847` (Exact match)

### Evidence Directory
Location: `C:\Users\jerem\Desktop\GridFall-UMGToolSet-Probe-Evidence\AccessibleUMGInspector-Milestone5`
All 13 primary evidence artifacts recorded:
- `build_output.txt`
- `descriptor_before.json`, `descriptor_after.json`, `descriptor.diff`
- `plugin_manifest.txt`
- `phase_a_serialized.json`, `phase_a_serialized.txt`
- `phase_b_runtime.json`, `phase_b_runtime.txt`
- `phase_c_comparison.json`, `phase_c_comparison.txt`
- `phase_d_audit.json`, `phase_d_audit.txt`

### Plugin Automated Regression Suite
- Headless test execution of `AccessibleUMGInspectorHost`:
  - 96 of 96 automation tests passed (`100%`).
  - 0 errors, 0 failures.

---

## 5. Milestone 5 Correction: Two-Tier Visibility & Ancestor Propagation

### 5.1 Defect Analysis: The 44 False-Positive Errors
In the initial Milestone 5 audit of `WBP_GameShell`, 44 errors were reported. Investigation revealed that the audit engine conflated local widget visibility with effective runtime exposure:
- Under Epic's UMG/Slate visibility model, child controls retain their authored local `Visibility = Visible` property even when an ancestor container (such as `Panel_HostScreen`, `Panel_JoinScreen`, or `Modal_ExitConfirmation`) is set to `Collapsed` or `Hidden`.
- When an ancestor is `Collapsed`, Slate suppresses the entire subtree from layout, hit testing, rendering, and accessibility.
- Because the audit engine initially evaluated only local `Visibility == Visible`:
  1. `A11Y-06` (CollapsedScreenRuntimeDescendant) flagged every child button inside collapsed panels as an error, even though no descendant escaped suppression.
  2. `A11Y-04` (DuplicateAccessibleName) evaluated duplicate names (e.g., "Back", "Status") across inactive and active panels concurrently.
  3. `A11Y-13` (AccessibleChildLeak) flagged children under `NotAccessible` containers that were already collapsed.
  4. The Differential Comparator flagged false visibility mismatches and unavailable geometry for children whose collapsed ancestors legitimately prevent Slate layout allocation.

### 5.2 Two-Tier Visibility Architecture
To resolve this without altering authored local properties, Accessible UMG Inspector introduces an authoritative two-tier visibility model:
1. **`LocalVisibility`**: Authoritative local visibility declared directly on the widget.
2. **`EffectiveVisibility`**: Effective visibility computed via deterministic top-down ancestor propagation:
   - If any ancestor in the hierarchy chain is `Collapsed`, `EffectiveVisibility = Collapsed`.
   - Else if any ancestor is `Hidden`, `EffectiveVisibility = Hidden`.
   - Else if any ancestor is `HitTestInvisible`, `EffectiveVisibility = HitTestInvisible`.
   - Else `EffectiveVisibility = LocalVisibility`.
3. **State Fields**:
   - `BlockingAncestorIndex` & `BlockingAncestorName`: Explicit tracking of the container suppressing the control.
   - `bEffectivelyVisible`: True only if `EffectiveVisibility == Visible`.
   - `bEffectivelyInteractive`: Requires effective visibility, enabled state, and self-hit-testability.
   - `bEffectivelyFocusable`: Requires local focusability, enabled state, and effective visibility.
   - `bEffectiveGeometryAvailable`: Requires valid geometry on an effectively visible control.

### 5.3 Audit Rule & Differential Updates
- **`A11Y-06`**: Now triggers only if a descendant truly escapes container suppression (e.g. via direct keyboard focus or active Slate visibility while container Slate visibility is suppressed).
- **`A11Y-01`, A11Y-02, A11Y-03, A11Y-11, A11Y-13**: Refactored to gate evaluation on `bEffectivelyVisible` / `bEffectivelyInteractive`.
- **`A11Y-04`**: Gated to controls within the active screen context (`bEffectivelyVisible && BlockingAncestorIndex == -1`).
- **`A11Y-07`**: Requires observed passive focus plus inactive ancestor suppression.
- **Differential Comparator**: Suppresses visibility discrepancies when child retains local `Visible` under a collapsed container, and suppresses geometry discrepancies for collapsed subtrees.
- **NVDA Text Formatter**: Visualizes inactive/suppressed subtrees with `[Suppressed by: <AncestorName>]` annotations.

### 5.4 Sanitized Project-Neutral Replay Verification
- Added automated replay test (`Milestone5_Replay_GameShell_Audit_ZeroErrors`) mirroring the exact `WBP_GameShell` hierarchy (active `Panel_StartScreen` with buttons, collapsed `Panel_HostScreen`, `Panel_JoinScreen`, `Modal_ExitConfirmation` with local `Visible` children).
- Verified that the corrected audit engine produces **0 errors** on the stable start screen, successfully resolving all 44 false positives.
