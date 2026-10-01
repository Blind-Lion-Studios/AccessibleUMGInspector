# Milestone 4 Architectural Decisions & Record

**Repository**: `Blind-Lion-Studios/AccessibleUMGInspector`  
**Milestone**: Milestone 4 (Accessibility Audit Engine Core)  
**Status**: Accepted & Implemented  
**Date**: 2026-10-01  

---

## 1. Context and Proven Motivations

From the isolated proofs in GridFall:
1. **Authoritative Evidence Separation**: Serialized Blueprint reflection, runtime Slate/UMG tree state, and differential reconciliation provide distinct evidence domains. Audit rules must consume these verified evidence structures rather than duplicating tree traversals.
2. **Strict Read-Only Non-Mutation Policy**: The audit engine must never modify packages, dirty assets, set widget properties, construct unconstructed Slate widgets, or simulate inputs.
3. **No False Confidence or Metric Hallucinations**: Numeric confidence scores (e.g. "87% confident") are subjective and misleading in accessibility tooling. The engine enforces discrete, qualitative confidence bases (`Direct`, `Derived`, `Ambiguous`, `Unavailable`).
4. **Distinction Between Passed and NotEvaluated**: A rule where essential evidence is missing must never be marked as "Passed" or "Failed"; it must be reported as `NotEvaluated` with explicit documentation of what evidence was absent.

---

## 2. Accepted Milestone 4 Decisions

### Decision 1: Report Consumption Architecture
The audit engine (`FAccessibilityAuditEngine`) is completely decoupled from low-level tree traversal and widget matching. It accepts pre-constructed or serially executed reports:
- `FSerializedWidgetBlueprintReport` (Milestone 1)
- `FRuntimeWidgetReport` (Milestone 2)
- `FSerializedRuntimeComparisonReport` (Milestone 3)
Each rule is an independent, pure evaluation function consuming these data contracts, enabling fast, isolated unit testing without asset loading.

### Decision 2: Rule Taxonomy & Complete Implementation (A11Y-01 through A11Y-14)
- **`A11Y-01` HiddenButRuntimeExposed**: Direct Error when a serialized Hidden/Collapsed control is exposed at runtime in an inactive parent container. Suppressed when container is legitimately activated.
- **`A11Y-02` VisibleButUnnamed**: Direct Error when a visible, enabled interactive control has no effective accessible text. Decorative controls (`NotAccessible`) are explicitly excluded.
- **`A11Y-03` FocusableButUnnamed**: Direct Error when a focusable interactive control has no accessible name.
- **`A11Y-04` DuplicateAccessibleName**: Warning by default; escalated to Error when duplicate accessible names occur among simultaneously active, equivalent interactive controls. Summaries or distinct roles disambiguate.
- **`A11Y-05` InteractiveChildOutsidePanel**: Warning when an interactive child is reparented outside its expected container with reliable containment evidence. Never inferred from naming conventions alone.
- **`A11Y-06` CollapsedScreenRuntimeDescendant**: Direct Error when a descendant of a collapsed or inactive container remains visible and enabled at runtime.
- **`A11Y-07` FocusOnInactiveScreen**: Direct Error when passive runtime focus is established on a control inside an inactive screen. Never inferred from hover, bounds, or defaults.
- **`A11Y-08` NavigationTargetOutsideScreen**: Warning when an explicit navigation rule targets outside the active screen. "Navigation None" is never considered defective.
- **`A11Y-09` ModalContainmentFailure**: Direct Error when a modal is active but underlying background controls remain focusable or retain focus. Does not claim hit-testability without hit-test-grid evidence.
- **`A11Y-10` VisibilityStateMismatch**: Warning for unexplained serialized vs runtime visibility divergence. Suppressed during expected state transitions.
- **`A11Y-11` OverlappingInteractiveBounds**: Warning when two visible, enabled interactive runtime controls overlap beyond the 10% threshold. Excludes parent-child composition, zero-size geometry, and inactive controls.
- **`A11Y-12` MissingActionBinding**: Warning when an action control (e.g. Button) has zero delegate or Blueprint event bindings.
- **`A11Y-13` AccessibleChildLeak**: Direct Error when a parent disallows accessible children (`NotAccessible`) but a child is independently exposed with accessible text.
- **`A11Y-14` AmbiguousRuntimeIdentity**: Warning when ambiguous top-level candidates or comparison records prevent deterministic audit. Steers human review.

### Decision 3: Non-Numeric Confidence Bases
Findings explicitly declare their confidence basis:
- **`Direct`**: Unambiguous reflection or live runtime properties directly verified.
- **`Derived`**: Inferred from structural discrepancy correlation or screen state transitions.
- **`Ambiguous`**: Multiple competing candidates prevent singular attribution.
- **`Unavailable`**: Evidence missing; finding classified as `NotEvaluated`.

### Decision 4: Finding Deduplication & Deterministic Ordering
- **Deduplication**: Findings are deduplicated using a composite key: `RuleId:WidgetIdentity:Message`.
- **Deterministic Sort**: Findings are ordered strictly:
  1. `Error`
  2. `Warning`
  3. `Info`
  4. `NotEvaluated`
  Within each severity group, findings are sorted by `RuleId` ascending, then by `WidgetIdentity` ascending.

### Decision 5: Screen-Reader NVDA Output Architecture
Plain-text NVDA output follows strict section ordering:
1. Target identifier and Evidence Mode (`Serialized`, `Runtime`, `Differential`).
2. `=== ERRORS (X) ===`
3. `=== WARNINGS (Y) ===`
4. `=== HUMAN REVIEW REQUIRED ===`
5. `=== NOT EVALUATED (W) ===`
6. `=== PASSED RULES (P) ===`
7. `=== ARCHITECTURAL LIMITATIONS ===`
Every finding states: Rule ID & Name, Affected Control, Observed, Evidence Source, Why It Matters, Remediation Guidance, and Explicit Limitations.

---

## 3. Verification & Regression Results

- Built targeting Unreal Engine 5.8.2 via `RunUAT.bat BuildPlugin`.
- **96 of 96 Automated Tests Passed** (`ExitCode: 0`):
  - 11 Serialized Blueprint Tests (Milestone 1 baseline preserved with zero regression).
  - 20 Runtime Viewport / UserWidget Tests (Milestone 2 baseline preserved with zero regression).
  - 26 Differential Comparator Tests (Milestone 3 baseline preserved with zero regression).
  - 39 Accessibility Audit Engine Tests (Milestone 4 implementation fully validated).

---

## 4. Milestone Boundary Observance

The following items remain strictly **excluded** from Milestone 4:
- Automatic fixes or recommendations that mutate state.
- Input simulation (keyboard, mouse, gamepad).
- OS UI Automation, IAccessible2, or direct NVDA API integration.
- Hit-test-grid inspection.
- PIE lifecycle control (Inspector does not start/stop/pause PIE).
- GridFall project integration or live workspace testing (Milestone 5).
- Repository publication, remote creation, or git mutations.
