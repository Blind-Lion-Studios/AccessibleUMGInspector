# Accessible UMG Inspector — Audit Rule Reference

**Repository**: `Blind-Lion-Studios/AccessibleUMGInspector`  
**Rule Set Version**: `1.0.0`  
**Target Engine**: Unreal Engine 5.8.2  

---

> [!IMPORTANT]
> **Automated Inspection Notice**: Automated accessibility inspection is a diagnostics accelerator, **not a replacement** for manual keyboard testing, NVDA screen-reader verification, controller validation, sighted design review, or gameplay playtesting. Automated rules detect objective structural, property, and geometry discrepancies; they cannot evaluate visual readability, speech pacing, or gameplay accessibility.

---

## Rule Reference Table

| Rule ID | Rule Name | Default Severity | Required Evidence | Primary Confidence Basis |
| :--- | :--- | :--- | :--- | :--- |
| **A11Y-01** | HiddenButRuntimeExposed | Error | Differential (Serialized + Runtime) | Direct |
| **A11Y-02** | VisibleButUnnamed | Error | Runtime | Direct |
| **A11Y-03** | FocusableButUnnamed | Error | Runtime or Serialized | Direct |
| **A11Y-04** | DuplicateAccessibleName | Warning / Error | Serialized or Runtime | Direct / Derived |
| **A11Y-05** | InteractiveChildOutsidePanel | Warning | Differential | Direct |
| **A11Y-06** | CollapsedScreenRuntimeDescendant | Error | Runtime | Direct |
| **A11Y-07** | FocusOnInactiveScreen | Error | Runtime | Direct |
| **A11Y-08** | NavigationTargetOutsideScreen | Warning | Serialized or Runtime | Direct |
| **A11Y-09** | ModalContainmentFailure | Error | Runtime | Direct |
| **A11Y-10** | VisibilityStateMismatch | Warning | Differential | Derived |
| **A11Y-11** | OverlappingInteractiveBounds | Warning | Runtime (with Geometry) | Direct |
| **A11Y-12** | MissingActionBinding | Warning | Serialized | Direct |
| **A11Y-13** | AccessibleChildLeak | Error | Runtime | Direct |
| **A11Y-14** | AmbiguousRuntimeIdentity | Warning | Runtime or Differential | Ambiguous |

---

## Core Visibility and Exposure Concepts

The audit engine operates on an authoritative two-tier visibility model:

1. **Local Visibility (`LocalVisibility`)**: The direct property authored on the UMG `UWidget` (e.g. `Visible`, `Collapsed`, `Hidden`, `HitTestInvisible`, `SelfHitTestInvisible`). In Slate/UMG, child widgets retain their authored local visibility even when an enclosing ancestor panel is collapsed.
2. **Effective Visibility (`EffectiveVisibility`)**: The true rendered and layout-participating state of the widget after walking the complete runtime ancestor chain.
   - If the widget or any ancestor is `Collapsed`, `EffectiveVisibility = Collapsed`, `bEffectivelyVisible = false`, `bEffectivelyInteractive = false`, `bEffectivelyFocusable = false`, and the nearest blocking ancestor is recorded.
   - If the widget or any ancestor is `Hidden`, `EffectiveVisibility = Hidden`, `bEffectivelyVisible = false`, `bEffectivelyInteractive = false`, `bEffectivelyFocusable = false`.
   - `HitTestInvisible` propagates down: descendants remain visible but are not pointer hit-testable.
   - `SelfHitTestInvisible` affects only the widget itself; descendants remain interactive.
   - Parent disabled state propagates: a disabled ancestor makes descendants effectively disabled (`bEffectivelyInteractive = false`).
   - Cached geometry under an effectively collapsed or hidden subtree does not establish visibility, interaction, or screen overlap (`bEffectiveGeometryAvailable = false`).

---

## Detailed Rule Specifications

### A11Y-01: HiddenButRuntimeExposed
- **Description**: Detects when a control authored as Hidden or Collapsed in the Widget Blueprint is unexpectedly exposed at runtime.
- **Applicability**: Differential mode only (requires both Serialized Blueprint asset and live Runtime hierarchy).
- **Required Evidence**: Serialized `DefaultVisibility` (`Collapsed` or `Hidden`), Runtime `EffectiveVisibility` and `bEffectivelyVisible == true`.
- **Direct vs Derived Evidence**:
  - *Direct*: Direct property comparison between matched serialized node and live runtime node where control is effectively exposed (`bEffectivelyVisible == true`) inside an inactive parent container.
  - *Derived*: Screen activation inference when parent container is not explicitly collapsed.
- **Suppression Conditions**: Suppressed when the control is effectively suppressed by a collapsed/hidden ancestor (`!bEffectivelyVisible`), or when the containing parent panel or screen is legitimately activated to `Visible` by runtime UI logic (intentional screen activation).
- **NotEvaluated Conditions**: Returns `NotEvaluated` when either Serialized Blueprint or Runtime evidence is unavailable, or when active context cannot be established.
- **Severity**: `Error` when effectively exposed inside an inactive container; `Warning` when exposure cannot be verified against active-screen context.
- **Limitations**: Does not verify pointer hit-test routing or clipping.
- **Human-Review Requirement**: Verify whether the runtime visibility activation was intentional or an inadvertent leak of an inactive screen element.
- **Remediation Guidance**: Ensure inactive screen containers properly collapse all child controls when hidden.

---

### A11Y-02: VisibleButUnnamed
- **Description**: Detects effectively visible, enabled interactive controls that provide no effective accessible text label for screen readers.
- **Applicability**: Runtime mode or Differential mode.
- **Required Evidence**: Runtime `bEffectivelyVisible == true`, `bIsEnabled == true`, interactive class/role, and empty `AccessibleName`.
- **Direct vs Derived Evidence**:
  - *Direct*: Authoritative reflection of `UWidget::AccessibleName` and Slate widget cache for effectively visible controls.
- **Suppression Conditions**: Suppressed when the control is inside a collapsed or hidden ancestor (`!bEffectivelyVisible`), or for decorative or non-interactive widgets where `bOverrideAccessibleDefaults && AccessibleBehavior == "NotAccessible"`.
- **NotEvaluated Conditions**: Returns `NotEvaluated` when live runtime hierarchy evidence is unavailable.
- **Severity**: `Error`.
- **Limitations**: Does not inspect the operating system accessibility tree (UI Automation/IAccessible2) or live NVDA output.
- **Human-Review Requirement**: Confirm that text provided in visual children (e.g. nested TextBlocks) is properly synthesized or exposed as the parent button's accessible name.
- **Remediation Guidance**: Provide an accessible name in UMG Accessibility properties (`AccessibleName`) or assign clear text content.

---

### A11Y-03: FocusableButUnnamed
- **Description**: Detects controls that accept keyboard or controller focus (`bIsFocusable == true` and `bEffectivelyFocusable == true`) but have no accessible name.
- **Applicability**: Serialized, Runtime, or Differential mode.
- **Required Evidence**: `bEffectivelyFocusable == true` (or serialized `bIsFocusable == true`), `bIsEnabled == true`, effectively visible state, and empty `AccessibleName`.
- **Direct vs Derived Evidence**:
  - *Direct*: Authoritative reflection of `bIsFocusable` and `AccessibleName`.
- **Suppression Conditions**: Suppressed when control or any ancestor is Collapsed or Hidden (`!bEffectivelyVisible` or `!bEffectivelyFocusable`).
- **NotEvaluated Conditions**: Returns `NotEvaluated` when focusability evidence cannot be retrieved from serialized reflection or live Slate widgets.
- **Severity**: `Error`.
- **Limitations**: Focusability inspected passively via Slate properties; does not prove focus appearance visual cues.
- **Human-Review Requirement**: Confirm that focused state renders an accessible visual focus indicator.
- **Remediation Guidance**: Assign an accessible name to this focusable control to allow screen readers to announce it upon receiving focus.

---

### A11Y-04: DuplicateAccessibleName
- **Description**: Detects multiple interactive controls sharing identical accessible names within the active screen context.
- **Applicability**: Serialized, Runtime, or Differential mode.
- **Required Evidence**: Multiple effectively active controls (`bEffectivelyVisible == true`, `bIsEnabled == true`, unblocked by inactive ancestors `BlockingAncestorIndex == -1`) with identical non-empty `AccessibleName` (case-insensitive).
- **Direct vs Derived Evidence**:
  - *Direct*: Exact string comparison across simultaneously active controls in the same active panel.
  - *Derived*: Contextual equivalence determination.
- **Suppression Conditions**: Controls on inactive or collapsed screens (`BlockingAncestorIndex >= 0`) are excluded and do not collide with active screen controls. Suppressed when controls have distinct roles (e.g. Button vs CheckBox) or distinct `AccessibleSummary` descriptions that disambiguate their purpose.
- **NotEvaluated Conditions**: Returns `NotEvaluated` when no controls with accessible names are present.
- **Severity**: `Warning` by default. Escalated to `Error` only when duplicate names occur among simultaneously active, equivalent interactive controls and create demonstrated ambiguity.
- **Limitations**: Does not compare against Unreal Editor chrome.
- **Human-Review Requirement**: Sighted or screen-reader review to ensure users can differentiate between controls performing similar actions.
- **Remediation Guidance**: Differentiate duplicate control names by assigning distinct `AccessibleName` labels or descriptive `AccessibleSummary` attributes.

---

### A11Y-05: InteractiveChildOutsidePanel
- **Description**: Detects when an interactive control is parented outside its authored container hierarchy at runtime.
- **Applicability**: Differential mode.
- **Required Evidence**: Matched interactive control where serialized `ParentName` authoritatively differs from runtime `ParentName`.
- **Direct vs Derived Evidence**:
  - *Direct*: Direct comparison of authoritative parent object references. Never inferred from naming conventions.
- **Suppression Conditions**: Suppressed when parent references match or when widget is non-interactive.
- **NotEvaluated Conditions**: Returns `NotEvaluated` when differential containment evidence is incomplete or unavailable.
- **Severity**: `Warning`.
- **Limitations**: Does not infer expected panels from naming conventions alone.
- **Human-Review Requirement**: Verify whether runtime reparenting preserves the intended logical tab order and screen-reader reading hierarchy.
- **Remediation Guidance**: Verify that widget reparenting preserves the intended accessible panel containment structure.

---

### A11Y-06: CollapsedScreenRuntimeDescendant
- **Description**: Detects when a descendant of a collapsed or inactive container escapes ancestor suppression at runtime.
- **Applicability**: Runtime mode or Differential mode.
- **Required Evidence**: An ancestor container with `EffectiveVisibility == "Collapsed"` or `"Hidden"` having a descendant that escapes suppression (e.g. `bEffectivelyVisible == true`, `bHasKeyboardFocus == true`, `bHasUserFocus == true`, or live Slate widget is visible while container is not).
- **Direct vs Derived Evidence**:
  - *Direct*: Authoritative tree traversal of live runtime widget records and focus inspection.
- **Suppression Conditions**: Suppressed when a descendant is properly collapsed/hidden by its ancestor. In Epic UMG, child controls normally retain local `Visible` while their container is `Collapsed`; this rule does NOT fire unless runtime evidence proves the child escaped suppression.
- **NotEvaluated Conditions**: Returns `NotEvaluated` when live runtime hierarchy evidence is unavailable.
- **Severity**: `Error`.
- **Limitations**: Does not verify pointer hit-test routing.
- **Human-Review Requirement**: Confirm whether the descendant control can accept keyboard focus or pointer clicks.
- **Remediation Guidance**: Ensure inactive or collapsed screen containers collapse their full child hierarchy.

---

### A11Y-07: FocusOnInactiveScreen
- **Description**: Detects when passive runtime keyboard or user focus resides on a control located inside an inactive or collapsed screen container.
- **Applicability**: Runtime mode.
- **Required Evidence**: Live runtime control with `bHasKeyboardFocus == true` or `bHasUserFocus == true` residing inside an ancestor with `BlockingAncestorIndex >= 0` or collapsed/hidden effective visibility.
- **Direct vs Derived Evidence**:
  - *Direct*: Passive inspection of `FSlateApplication::Get().GetKeyboardFocusedWidget()` and `GetUserFocusedWidget(0)`.
- **Suppression Conditions**: Suppressed when focused control's entire ancestor chain is Visible.
- **NotEvaluated Conditions**: Returns `NotEvaluated` when live focus evidence is unavailable.
- **Severity**: `Error`.
- **Limitations**: Never inferred from navigation targets, accessible text, hover, bounds, or serialized defaults. Focus must be directly observed.
- **Human-Review Requirement**: Verify that keyboard navigation cannot navigate into hidden screen layers.
- **Remediation Guidance**: Clear or redirect focus away from controls inside inactive screens when transitioning away.

---

### A11Y-08: NavigationTargetOutsideScreen
- **Description**: Detects explicit directional navigation rules that point to target widgets outside the active screen or widget tree.
- **Applicability**: Serialized or Differential mode.
- **Required Evidence**: An explicit `FAccessibleNavigationRecord` with `Rule == "Explicit"` where `TargetWidget` is not found within the active widget tree.
- **Direct vs Derived Evidence**:
  - *Direct*: Authoritative reflection of `UWidget::Navigation` rules.
- **Suppression Conditions**: Suppressed when navigation rules are `None`, `Escape`, `Stop`, or `Wrap`. "Navigation None" is never considered defective.
- **NotEvaluated Conditions**: Returns `NotEvaluated` when navigation rules are absent or automatic.
- **Severity**: `Warning`.
- **Limitations**: Does not simulate keyboard or gamepad navigation input.
- **Human-Review Requirement**: Test controller D-pad and arrow key navigation across all screen boundaries.
- **Remediation Guidance**: Re-target directional navigation rules to controls within the active screen or use automatic navigation.

---

### A11Y-09: ModalContainmentFailure
- **Description**: Detects when a modal dialog is established as active, but underlying background controls retain keyboard focus or remain interactively exposed.
- **Applicability**: Runtime mode or Differential mode.
- **Required Evidence**: An active modal container (`bEffectivelyVisible == true`), and an underlying non-modal widget with direct focus (`bHasKeyboardFocus || bHasUserFocus`) or effective interaction (`bEffectivelyInteractive == true`).
- **Direct vs Derived Evidence**:
  - *Direct*: Direct inspection of active modal hierarchy, effective exposure, and passive Slate focus.
- **Suppression Conditions**: Suppressed when no modal is active, when the modal container itself is suppressed by an ancestor, or when all underlying background controls are suppressed under collapsed/hidden ancestors (`bEffectivelyVisible == false`) or disabled.
- **NotEvaluated Conditions**: Returns `NotEvaluated` when modal state cannot be established.
- **Severity**: `Error`.
- **Limitations**: Bounds overlap alone does not prove containment failure; does not claim hit-testability without hit-test-grid evidence.
- **Human-Review Requirement**: Confirm that Tab and Shift+Tab navigation wrap strictly within the modal dialog.
- **Remediation Guidance**: Trap focus within the active modal and collapse or disable underlying interactive controls.

---

### A11Y-10: VisibilityStateMismatch
- **Description**: Detects unexplained divergence between authored default visibility and live runtime visibility.
- **Applicability**: Differential mode.
- **Required Evidence**: Matched node where serialized `DefaultVisibility` differs from runtime `LocalVisibility` or `EffectiveVisibility`.
- **Direct vs Derived Evidence**:
  - *Derived*: Correlation between authored asset defaults and live Slate state.
- **Suppression Conditions**: Suppressed when divergence corresponds to an expected screen activation transition, or when a child control retains authored local `Visible` while residing inside a collapsed or hidden ancestor container (`BlockingAncestorIndex >= 0`).
- **NotEvaluated Conditions**: Returns `NotEvaluated` when differential comparison evidence is unavailable.
- **Severity**: `Warning`.
- **Limitations**: Reported as descriptive evidence, not crash-cause proof.
- **Human-Review Requirement**: Verify that visibility divergence aligns with intended gameplay state machines.
- **Remediation Guidance**: Confirm that visibility change is driven by an intentional gameplay state transition.

---

### A11Y-11: OverlappingInteractiveBounds
- **Description**: Detects two visible, enabled interactive runtime controls whose screen bounds overlap beyond the 10% threshold.
- **Applicability**: Runtime mode with valid Slate geometry.
- **Required Evidence**: Two distinct interactive widgets with valid non-zero screen bounds where intersection area / min(AreaA, AreaB) > 0.10 and dimensions > 4.0 units, both actively exposed (`bEffectivelyVisible == true`, `bEffectivelyInteractive == true`, `BlockingAncestorIndex == -1`).
- **Direct vs Derived Evidence**:
  - *Direct*: Screen positions and sizes read via `SWidget::GetTickSpaceGeometry()`.
- **Suppression Conditions**: Excludes parent-child composition (e.g. Button containing TextBlock), zero-size bounds, invalid geometry, and controls suppressed under collapsed or hidden ancestor containers (`bEffectivelyVisible == false`).
- **NotEvaluated Conditions**: Returns `NotEvaluated` when runtime geometry evidence is unavailable.
- **Severity**: `Warning`.
- **Limitations**: Geometry does not prove pointer routing, clipping, or hit-test-grid membership.
- **Human-Review Requirement**: Sighted and touch/pointer inspection to ensure controls are not visually occluded or difficult to tap/click.
- **Remediation Guidance**: Adjust slot layout padding, anchors, or alignment to prevent interactive control overlap.

---

### A11Y-12: MissingActionBinding
- **Description**: Detects enabled action controls (e.g. Buttons) that have zero delegate or Blueprint event bindings.
- **Applicability**: Serialized mode or Differential mode.
- **Required Evidence**: An enabled `Button` with `Bindings.Num() == 0`.
- **Direct vs Derived Evidence**:
  - *Direct*: Direct inspection of `UWidgetBlueprint` delegate bindings.
- **Suppression Conditions**: Suppressed for disabled controls or non-action widget classes.
- **NotEvaluated Conditions**: Returns `NotEvaluated` when serialized binding introspection evidence is unavailable.
- **Severity**: `Warning`.
- **Limitations**: Does not infer intended action solely from class or name.
- **Human-Review Requirement**: Confirm whether button actions are bound via C++ or external event brokers.
- **Remediation Guidance**: Bind an `OnClicked` or action event handler to this button.

---

### A11Y-13: AccessibleChildLeak
- **Description**: Detects when a parent container directly disallows accessible children (`AccessibleBehavior == "NotAccessible"`), but a child widget is independently exposed with accessible text.
- **Applicability**: Runtime mode or Differential mode.
- **Required Evidence**: Parent widget with `bOverrideAccessibleDefaults && AccessibleBehavior == "NotAccessible"`, and an effectively exposed child widget (`bEffectivelyVisible == true`, `bHasSlateWidget == true`) with non-empty `AccessibleName` or `AccessibleBehavior == "Custom"`.
- **Direct vs Derived Evidence**:
  - *Direct*: Authoritative reflection of parent and child accessible properties.
- **Suppression Conditions**: Suppressed when parent allows accessible children, or when the child widget is suppressed under a collapsed or hidden ancestor container (`bEffectivelyVisible == false`).
- **NotEvaluated Conditions**: Returns `NotEvaluated` when runtime accessibility hierarchy evidence is unavailable.
- **Severity**: `Error`.
- **Limitations**: Does not claim equivalence with the operating system accessibility tree or NVDA output.
- **Human-Review Requirement**: Review container accessibility scoping to ensure screen readers do not read internal sub-elements unexpectedly.
- **Remediation Guidance**: Set child accessible behavior to NotAccessible or remove parent disallowance.

---

### A11Y-14: AmbiguousRuntimeIdentity
- **Description**: Detects when an accessibility result cannot be tied uniquely to one serialized or runtime control.
- **Applicability**: Runtime mode or Differential mode.
- **Required Evidence**: Runtime report with `Availability == Ambiguous` or `Candidates.Num() > 1`, or differential comparison report with `AmbiguousCount > 0`.
- **Direct vs Derived Evidence**:
  - *Ambiguous*: Multiple competing candidates prevent singular attribution.
- **Suppression Conditions**: Suppressed when all serialized and runtime nodes are uniquely resolved 1-to-1.
- **NotEvaluated Conditions**: Returns `NotEvaluated` when input reports are unavailable.
- **Severity**: `Warning`.
- **Limitations**: This rule steers human review; it does not accuse the asset of a structural defect.
- **Human-Review Requirement**: Human verification to identify which candidate runtime widget corresponds to the intended inspection target.
- **Remediation Guidance**: Assign unique widget identifiers or parent container scopes to allow deterministic accessibility audit.
