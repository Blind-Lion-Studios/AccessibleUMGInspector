# Accessible UMG Inspector — Architecture Specification

---

## 1. System Overview

Accessible UMG Inspector provides an accessibility-first inspection layer for Unreal Engine 5.8.2 UMG User Interfaces. It bridges the gap between static asset design (`UWidgetBlueprint`) and live runtime Slate composition without relying on visual Editor panels or mouse-hover interactions.

```text
       ┌────────────────────────────────────────────────────────┐
       │                 Unreal MCP Transport                   │
       │                   (127.0.0.1:8000)                     │
       └──────────────────────────┬─────────────────────────────┘
                                  │
                                  ▼
       ┌────────────────────────────────────────────────────────┐
       │             UAccessibleUMGInspectorToolset             │
       │              (Derived from UToolsetDefinition)          │
       └───────┬────────────────────────────────────────┬───────┘
               │                                        │
               ▼                                        ▼
┌──────────────────────────────┐        ┌──────────────────────────────┐
│  SerializedWidgetInspector   │        │     RuntimeWidgetInspector   │
│   (UWidgetBlueprint Tree)    │        │  (UGameViewportClient / PIE) │
│       [MILESTONE 1]          │        │        [MILESTONE 2]         │
└──────────────┬───────────────┘        └──────────────┬───────────────┘
               │                                       │
               ▼                                       ▼
┌──────────────────────────────────────────────────────────────┐
│                  Differential Comparator                     │
│        (Reconciliation: Matched, Ambiguous, Pruned)          │
│                        [MILESTONE 3]                         │
└──────────────────────────────┬───────────────────────────────┘
                               │
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                 Accessibility Audit Rules                    │
│      (A11Y-01 through A11Y-12 Violation Checks)              │
│                        [MILESTONE 4]                         │
└──────────────────────────────┬───────────────────────────────┘
                               │
               ┌───────────────┴───────────────┐
               ▼                               ▼
┌──────────────────────────────┐ ┌──────────────────────────────┐
│      NvdaTextFormatter       │ │     JsonReportFormatter      │
│  (Numbered Accessible Text)  │ │   (Full Structured JSON)     │
└──────────────────────────────┘ └──────────────────────────────┘
```

---

## 2. Serialized Record Contract (Milestone 1)

Every serialized widget node emits:
- **`WidgetName`**: Internal variable or component identifier (e.g. `Button_Start`).
- **`WidgetClass`**: C++ reflection class (e.g. `Button`, `TextBlock`, `CanvasPanel`).
- **`ObjectPath`**: Authoritative UObject path in memory.
- **`ParentName`**: Name of the parent widget in the hierarchy tree.
- **`ChildIndex`**: 0-based position in the parent container.
- **`DefaultVisibility`**: Authored visibility (`Visible`, `Collapsed`, `Hidden`, `HitTestInvisible`, `SelfHitTestInvisible`).
- **`bIsEnabled`**: Default interactive enabled state.
- **`bIsFocusable`**: Whether the control accepts keyboard focus.
- **`bIsVariable`**: Whether the widget is exposed as a Blueprint variable.
- **`bOverrideAccessibleDefaults`**: Whether custom accessibility text or rules were authored.
- **`AccessibleBehavior`**: Accessible behavior enum (`Auto`, `Custom`, `NotAccessible`, `Summary`, `ToolTip`).
- **`AccessibleName`**: Authored accessible text label.
- **`AccessibleSummary`**: Authored accessible description.
- **`Slot`**: Slot class type, parent widget, and layout properties (padding, alignment, anchors, offsets).
- **`NavigationRules`**: Directional navigation rules (`Left`, `Right`, `Up`, `Down`, `Next`, `Previous`) and explicit target widgets.
- **`Bindings`**: Blueprint delegate bindings (e.g. `OnClicked -> HandleStartClicked`).
- **`ExposedNamedSlots`**: Declared named slots if implementing `INamedSlotInterface`.
- **`Children`**: Recursive array of child records.

---

## 3. Toolset Interface & Operations

The toolset is registered with Unreal Engine via `UToolsetDefinition` and exposes two initial operations:

### `GetInspectorCapabilities()`
- Inputs: None.
- Output: `FInspectorCapabilities` (version, engine, supported operations, format types, safety design).

### `InspectSerializedWidgetBlueprint(AssetPath, Format)`
- Inputs:
  - `AssetPath` (string, required): Full asset path to a `UWidgetBlueprint`.
  - `Format` (string, optional): `"NvdaText"`, `"Json"`, or `"Both"`.
- Output: `FSerializedWidgetBlueprintReport` containing the parsed hierarchy, warning lists, package verification, and formatted report payloads.

### `InspectRuntimeUserWidget(WidgetReference, WorldContext, MaxDepth, Format)`
- Inputs:
  - `WidgetReference` (string, optional): Target widget name, object path, or class. If empty, resolves single top-level user widget.
  - `WorldContext` (string, optional): Target world name or path. If empty, resolves active PIE/Game world.
  - `MaxDepth` (int32, optional): Maximum hierarchy traversal depth (clamped to `[1, 100]`, default 32).
  - `Format` (string, optional): `"NvdaText"`, `"Json"`, or `"Both"`.
- Output: `FRuntimeWidgetReport` containing the live hierarchy, passive Slate geometry, focus states, active/hidden breakdown, and formatted report payloads.

### `CompareSerializedAndRuntime(AssetPath, RuntimeWidgetReference, WorldContext, MaxDepth, Format)`
- Inputs:
  - `AssetPath` (string, required): Full asset path to a `UWidgetBlueprint`.
  - `RuntimeWidgetReference` (string, optional): Target runtime widget name, object path, or class. If empty, resolves single top-level user widget.
  - `WorldContext` (string, optional): Target world name or path. If empty, resolves active PIE/Game world.
  - `MaxDepth` (int32, optional): Maximum hierarchy traversal depth (clamped to `[1, 100]`, default 32).
  - `Format` (string, optional): `"NvdaText"`, `"Json"`, or `"Both"`.
- Output: `FSerializedRuntimeComparisonReport` containing classification breakdown (`Matched`, `Ambiguous`, `SerializedOnly`, `RuntimeOnly`, `Unavailable`, `Unsupported`), reconciled comparison records, evidence chains, discrepancies list, and architectural limitations.

### `AuditWidgetAccessibility(AssetPath, RuntimeWidgetReference, WorldContext, RuleSet, MaxDepth, Format)`
- Inputs:
  - `AssetPath` (string, optional): Exact Widget Blueprint asset path.
  - `RuntimeWidgetReference` (string, optional): Target live `UUserWidget` object path, name, or class.
  - `WorldContext` (string, optional): Target world or PIE context.
  - `RuleSet` (array of strings, optional): Explicit list of rule IDs to evaluate. If empty, evaluates all applicable implemented rules.
  - `MaxDepth` (int32, optional): Maximum hierarchy traversal depth (clamped to `[1, 100]`, default 32).
  - `Format` (string, optional): `"NvdaText"`, `"Json"`, or `"Both"`.
- Output: `FAccessibilityAuditReport` containing finding classifications (`Error`, `Warning`, `Info`, `NotEvaluated`), explicit confidence bases (`Direct`, `Derived`, `Ambiguous`, `Unavailable`), rule breakdowns (`PassedRuleIds`, `NotEvaluatedRules`), plain-text NVDA report, and structured JSON.

---

## 4. Runtime Record Contract (Milestone 2 & Milestone 5 Correction)

Every runtime widget node emits:
- **`WidgetName`**: Runtime UWidget name (e.g. `Button_Start`).
- **`WidgetClass`**: Runtime C++ reflection class.
- **`ObjectPath`**: Authoritative UObject path in memory.
- **`ParentName`**: Name of parent widget in the hierarchy.
- **`ChildIndex`**: 0-based position in the parent container.
- **`Depth`**: Hierarchy depth relative to inspection root.
- **`RuntimeVisibility`**: Active runtime local visibility (retained for backward compatibility).
- **`LocalVisibility`**: Authoritative local visibility declared directly on this widget (`Visible`, `Collapsed`, `Hidden`, `HitTestInvisible`, `SelfHitTestInvisible`).
- **`EffectiveVisibility`**: Effective visibility computed by top-down ancestor propagation (`Visible`, `Collapsed`, `Hidden`, `HitTestInvisible`, `SelfHitTestInvisible`).
- **`BlockingAncestorIndex`**: Array index of nearest Collapsed or Hidden ancestor, or -1 if no ancestor blocks exposure.
- **`BlockingAncestorName`**: Name of nearest Collapsed or Hidden ancestor, or empty if unobstructed.
- **`bEffectivelyVisible`**: Authoritative boolean indicating whether widget is effectively visible after ancestor propagation (`EffectiveVisibility == Visible`).
- **`bEffectivelyInteractive`**: Whether widget is effectively interactive (effectively visible, enabled, self-hit-testable, and not blocked by an inactive container).
- **`bEffectivelyFocusable`**: Whether widget can effectively receive keyboard focus (locally focusable, enabled, and effectively visible).
- **`bEffectiveGeometryAvailable`**: Whether screen bounds are valid and belong to an effectively visible control.
- **`VisibilityEvidence`**: Explanatory string detailing local vs effective state and blocking ancestor identity.
- **`EffectiveStateLimitations`**: Explicit disclaimers clarifying that effective visibility does not claim OS accessibility tree exposure or hit-test grid verification.
- **`bIsEnabled`**: Active enabled state.
- **`bIsFocusable`**: Whether the control accepts keyboard focus.
- **`bHasKeyboardFocus`**: Whether the widget or its underlying Slate element has keyboard focus (inspected passively).
- **`bHasUserFocus`**: Whether the widget has user 0 focus (inspected passively).
- **`bIsHovered`**: Whether the widget or its Slate element is hovered.
- **`bOverrideAccessibleDefaults`**: Whether custom accessibility text or rules were authored.
- **`AccessibleBehavior`**: Accessible behavior enum string.
- **`AccessibleName`**: Dynamic or authored accessible text label.
- **`AccessibleSummary`**: Dynamic or authored accessible description.
- **`bHasSlateWidget`**: Whether a cached Slate representation exists via `GetCachedWidget()`.
- **`SlateWidgetType`**: Concrete Slate type string (e.g. `SButton`).
- **`bSlateVisible`**: Slate-level visibility boolean.
- **`bSlateEnabled`**: Slate-level enabled boolean.
- **`ScreenBounds`**: Absolute screen position (`X`, `Y`) and size (`Width`, `Height`) via `GetTickSpaceGeometry()`.
- **`Slot`**: Runtime slot class type and reflected layout properties.
- **`NavigationRules`**: Directional navigation rules and explicit targets.
- **`ChildIndices`**: Indices of child nodes in the report flat array.

---

## 5. Differential Comparison & Reconciliation Contract (Milestone 3)

The Differential Comparator reconciles authored `UWidgetBlueprint` declarations with live runtime Slate/UMG hierarchies.

### Classification Categories
- **`Matched`**: Exactly 1-to-1 correspondence established between serialized declaration and live runtime node.
- **`Ambiguous`**: Multiple serialized or runtime candidates exist that cannot be uniquely resolved, or class conflict occurs.
- **`SerializedOnly`**: Declared in Blueprint but absent from live runtime hierarchy (valid Collapsed/Hidden vs unexpected absent controls).
- **`RuntimeOnly`**: Present in live runtime hierarchy without a corresponding Blueprint declaration (dynamically created controls or Slate composition nodes).
- **`Unavailable`**: Input report failed to load or world/widget target could not be resolved.
- **`Unsupported`**: Target object does not support differential comparison.

### Matching Priority Rules
1. **Source Widget Name and Class Compatibility**: Exact match between `WidgetName` and compatible reflection class within parent container.
2. **Parent Container Disambiguation**: Multi-candidate widgets sharing identical names are disambiguated by matching their parent container name.
3. **Ambiguity Escalation**: If multiple serialized or runtime candidates remain ambiguous under the same parent container, all candidate records are classified as `Ambiguous`.
4. **Class Conflict Escalation**: If names match but reflection classes conflict, the pair is escalated to `Ambiguous` with explicit conflict evidence.
5. **Passive Evidence Correlation**: Accessible names, child order, slot types, and screen bounds are correlated as supporting or conflicting evidence without driving standalone identity.

### Discrepancy Detection
- **Parent/Containment Mismatches**: Serialized parent container differs from runtime parent container.
- **Child-Order Mismatches**: Relative container index shifted at runtime.
- **Visibility Discrepancies**: Authored default visibility differs from live runtime visibility.
- **State Discrepancies**: Interactive enabled state or keyboard focusability differs between serialized and runtime.
- **Accessibility Text Discrepancies**: Authored accessible name or summary label differs from dynamic runtime label.
- **Geometry Discrepancies**: Overlapping interactive bounds, zero-size interactive controls, or off-screen interactive controls.

### Deterministic Output Ordering
To ensure screen-reader efficiency and priority triage:
1. `Ambiguous` entries appear first.
2. `Matched` entries appear next.
3. `SerializedOnly` entries follow.
4. `RuntimeOnly` entries appear last.

---

## 6. Accessibility Audit Engine Contract (Milestone 4)

The Accessibility Audit Engine executes rules **A11Y-01 through A11Y-14** against serialized Blueprint trees, live runtime hierarchies, and reconciled differential reports without modifying assets or engine state.

### Finding Taxonomy
- **`Error`**: A demonstrable accessibility defect that creates a total barrier for screen-reader or keyboard users (e.g. visible unnamed interactive control, focusable unnamed control, active focus on an inactive screen, modal containment failure, accessible child leak).
- **`Warning`**: A structural, visibility, or geometry risk that degrades accessibility or creates potential ambiguity (e.g. duplicate accessible names, interactive child reparenting, unexplained visibility divergence, overlapping interactive bounds, unmapped action bindings).
- **`Info`**: Contextual or informational findings that do not represent defects.
- **`NotEvaluated`**: Required evidence is missing or cannot be authoritatively established in the current inspection mode. The engine explicitly logs missing evidence and never reports false passes or failures.

### Non-Numeric Confidence Bases
- **`Direct`**: Verified with authoritative, unambiguous serialized reflection, runtime Slate/UMG properties, or direct hierarchy matching.
- **`Derived`**: Inferred from structural discrepancy correlation, multi-candidate reconciliation, or screen activation patterns.
- **`Ambiguous`**: Multiple candidates exist that could not be uniquely resolved; finding steers human verification.
- **`Unavailable`**: Essential evidence is absent in the current inspection mode.

### Implemented Rules Summary
- **`A11Y-01` HiddenButRuntimeExposed**: Serialized hidden/collapsed control exposed at runtime in an inactive container.
- **`A11Y-02` VisibleButUnnamed**: Visible, enabled interactive runtime control with no accessible text.
- **`A11Y-03` FocusableButUnnamed**: Focusable interactive control with no accessible name.
- **`A11Y-04` DuplicateAccessibleName**: Duplicate accessible names among simultaneously active, equivalent interactive controls.
- **`A11Y-05` InteractiveChildOutsidePanel**: Interactive control reparented outside its expected container.
- **`A11Y-06` CollapsedScreenRuntimeDescendant**: Descendant of an inactive or collapsed container remaining visible at runtime.
- **`A11Y-07` FocusOnInactiveScreen**: Passive runtime focus established on a control inside an inactive screen.
- **`A11Y-08` NavigationTargetOutsideScreen**: Explicit directional navigation rule targeting outside the active screen.
- **`A11Y-09` ModalContainmentFailure**: Active modal where underlying background controls remain focusable or focused.
- **`A11Y-10` VisibilityStateMismatch**: Unexplained serialized vs runtime visibility divergence.
- **`A11Y-11` OverlappingInteractiveBounds**: Two visible, enabled interactive controls overlapping beyond threshold (> 10%).
- **`A11Y-12` MissingActionBinding**: Enabled action control with zero delegate or Blueprint event bindings.
- **`A11Y-13` AccessibleChildLeak**: Parent disallows accessible children but child is independently exposed.
- **`A11Y-14` AmbiguousRuntimeIdentity**: Multiple candidates prevent deterministic accessibility audit.

### NVDA Output Structure
1. **Headline / Opening Line**: Target identifier, Evidence Mode (`Serialized`, `Runtime`, `Differential`), summary counts.
2. **Errors**: Numbered error findings with Affected Control, Observed, Evidence Source, Why It Matters, Remediation, and Limitations.
3. **Warnings**: Numbered warning findings.
4. **Human Review Required**: Explicit callouts for ambiguous identities or duplicate names requiring human judgment.
5. **Not Evaluated**: Rules where required evidence was unavailable.
6. **Passed Rules**: Explicit list of evaluated rules without violations.
7. **Architectural Limitations**: Explicit boundaries and non-claims.


