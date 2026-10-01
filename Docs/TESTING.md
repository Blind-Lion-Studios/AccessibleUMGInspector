# Automation Testing Guide: Accessible UMG Inspector

## Overview
Accessible UMG Inspector includes a comprehensive automated test suite consisting of **112 automation tests** executed through Unreal Engine's headless test framework.

All tests are project-neutral:
- They operate on transient, in-memory synthetic test fixtures (`NewObject<UWidgetBlueprint>`, `TestConcreteUserWidget`, `FRuntimeWidgetReport`).
- Zero dependencies on proprietary project content or asset paths.
- Zero disk mutations or persistent package changes.

---

## Running Automation Tests

### Command Line (Headless Execution)
To run all Accessible UMG Inspector automation tests headlessly via `UnrealEditor-Cmd.exe`:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
    "C:\YourProject\YourProject.uproject" `
    -ExecCmds="Automation RunTests AccessibleUMGInspector; Quit" `
    -nullrhi -unattended -stdout -NoLoadingScreen
```

### Exit Codes
- **`0`**: All tests passed with zero errors or unhandled warnings.
- **`1` / non-zero**: One or more tests failed or logged critical errors.

---

## Test Suite Architecture (112 Total Tests)

### 1. Serialized Widget Inspector Tests (`AccessibleUMGInspector.Serialized.*` — 11 Tests)
Implemented in [`SerializedWidgetInspectorTests.cpp`](file:///C:/BLS/AccessibleUMGInspector/Source/AccessibleUMGInspectorTests/Private/SerializedWidgetInspectorTests.cpp):
1. `Capabilities`: Verifies plugin version (`0.1.0-rc.1`), engine version (`5.8.2`), safety flags.
2. `WidgetState`: Verifies serialized visibility, enabled state, focusability, and variable exposure.
3. `PanelSlots`: Introspects `CanvasPanelSlot` and `VerticalBoxSlot` layout properties.
4. `AccessibilityData`: Validates accessible name, summary, and behavior enum reflection.
5. `NavigationMetadata`: Verifies explicit directional navigation rules.
6. `Bindings`: Inspects Blueprint delegate bindings (`OnClicked`).
7. `OrderedTraversal`: Validates top-down child index ordering.
8. `DuplicateNames`: Verifies duplicate name detection and warning generation.
9. `DirtyStateInvariance`: Confirms inspection leaves `UPackage::IsDirty() == false`.
10. `NvdaFormatting`: Validates hierarchical plain-text output formatting.
11. `JsonValidity`: Confirms valid structured JSON serialization.

### 2. Runtime Widget Inspector Tests (`AccessibleUMGInspector.Runtime.*` — 20 Tests)
Implemented in [`RuntimeWidgetInspectorTests.cpp`](file:///C:/BLS/AccessibleUMGInspector/Source/AccessibleUMGInspectorTests/Private/RuntimeWidgetInspectorTests.cpp):
12. `NoEligibleWidget`: Handles empty viewport scenarios cleanly.
13. `AmbiguousCandidate`: Detects and surfaces multiple candidate widgets without choosing.
14. `SingleTopLevelResolution`: Resolves single top-level user widget when reference is omitted.
15. `VisibilityStates`: Reflects local visibility flags accurately.
16. `PassiveGeometryExtraction`: Reads screen bounds passively without invoking `TakeWidget()`.
17. `PassiveFocusExtraction`: Queries Slate keyboard and user focus passively.
18. `HoveredDetection`: Reads hover state passively.
19. `SlotProperties`: Extracts runtime slot class and layout properties.
20. `NavigationRules`: Extracts runtime navigation rules.
21. `DepthLimit`: Honors `MaxDepth` traversal clamp (`[1, 100]`).
22. `NodeCountLimit`: Caps traversal at 5,000 nodes to prevent memory exhaustion.
23. `NoTakeWidget`: Proves `GetCachedWidget()` is null after inspecting unconstructed widgets.
24. `NvdaFormatting`: Validates runtime plain-text output.
25. `JsonValidity`: Confirms valid structured JSON output.
26. `VisibleChildUnderCollapsedParent`: Verifies effective visibility is `Collapsed` when parent is collapsed.
27. `VisibleChildUnderHiddenParent`: Verifies effective visibility is `Hidden` when parent is hidden.
28. `VisibleGrandchildUnderCollapsedGrandparent`: Verifies multi-level ancestor propagation.
29. `DisabledAncestorPropagation`: Verifies disabled parent disables descendant effective interactivity.
30. `HitTestInvisiblePropagation`: Verifies `HitTestInvisible` propagates and suppresses pointer interaction.
31. `SelfHitTestInvisibleChildBehavior`: Verifies `SelfHitTestInvisible` allows child widgets to remain interactive.

### 3. Differential Comparator Tests (`AccessibleUMGInspector.Differential.*` — 26 Tests)
Implemented in [`DifferentialComparatorTests.cpp`](file:///C:/BLS/AccessibleUMGInspector/Source/AccessibleUMGInspectorTests/Private/DifferentialComparatorTests.cpp):
32–57: Comprehensive matching and reconciliation tests:
- Exact 1-to-1 matching by name and class.
- Disambiguation by parent container name.
- Multi-candidate escalation to `Ambiguous`.
- Class conflict escalation to `Ambiguous`.
- `SerializedOnly` and `RuntimeOnly` node classification.
- Parent and child-order discrepancy detection.
- Ancestor suppression of false visibility discrepancies.
- Priority-triage sorting (`Ambiguous` -> `Matched` -> `SerializedOnly` -> `RuntimeOnly`).

### 4. Accessibility Audit Engine Tests (`AccessibleUMGInspector.Audit.*` — 55 Tests)
Implemented in [`AccessibilityAuditEngineTests.cpp`](file:///C:/BLS/AccessibleUMGInspector/Source/AccessibleUMGInspectorTests/Private/AccessibilityAuditEngineTests.cpp):
58–112: Comprehensive automated rule evaluation tests:
- **`A11Y-01`**: Hidden serialized controls exposed at runtime; suppression for legitimate activations.
- **`A11Y-02`**: Visible unnamed interactive controls; suppression for non-interactive widgets.
- **`A11Y-03`**: Focusable unnamed interactive controls.
- **`A11Y-04`**: Duplicate accessible names among active equivalent controls; suppression across inactive screens.
- **`A11Y-05`**: Interactive child reparenting outside expected containers.
- **`A11Y-06`**: Descendants escaping collapsed container suppression (via focus or Slate visibility).
- **`A11Y-07`**: Focus directly established on inactive screen controls.
- **`A11Y-08`**: Directional navigation targeting outside screen boundaries; `Navigation None` tolerance.
- **`A11Y-09`**: Modal containment failure with active background focus; suppression when background is collapsed.
- **`A11Y-10`**: Unexplained visibility divergence; suppression for expected screen transitions and inactive containers.
- **`A11Y-11`**: Overlapping interactive bounds (>10% intersection); suppression for parent-child composition and inactive screens.
- **`A11Y-12`**: Enabled action buttons with zero delegate or Blueprint event bindings.
- **`A11Y-13`**: Accessible child leaks under `NotAccessible` containers.
- **`A11Y-14`**: Ambiguous candidate identity escalation.
- **Replay Verification**: Full synthetic replay of multi-screen game shells confirming **0 errors** on stable initial screens.

---

## Writing New Automation Tests
When extending the test suite:
1. Use `IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestClass, "AccessibleUMGInspector.Category.TestName", TEST_FLAGS)`.
2. Construct transient objects with `NewObject<T>(GetTransientPackage())`.
3. Set `Flags |= RF_Transient` on synthetic objects.
4. Verify both truth conditions (`TestTrue`/`TestFalse`) and descriptive failure messages.
5. Never introduce hardcoded project asset paths or external file dependencies.
