# Milestone 2 Architectural Decisions & Record

**Repository**: `Blind-Lion-Studios/AccessibleUMGInspector`  
**Milestone**: Milestone 2 (Runtime Viewport & UserWidget Inspector Core)  
**Status**: Accepted & Implemented  
**Date**: 2026-10-01  

---

## 1. Context and Proven Motivations

From the isolated proofs in GridFall:
1. **Epic SlateInspectorToolset Limitations**: Produced general Slate accessibility snapshots, but bounded depth exhausted within Unreal Editor docking chrome (`SWindow` -> `SDockTab` -> `SSplitter`) before reaching in-viewport game widgets. SlateInspector also lacked UMG object identity, pruned collapsed controls entirely, and could not root at a specific `UUserWidget`.
2. **Dedicated Runtime UMG Inspector Justification**: A dedicated runtime inspector querying live `UUserWidget` instances and cached Slate widgets (`GetCachedWidget()`) captures active visibility, focus states, and screen geometry directly at the game viewport level without docking chrome interference.

---

## 2. Accepted Milestone 2 Decisions

### Decision 1: Passive Slate Inspection & Zero-Mutation Policy
- **Never Call `TakeWidget()`**: Unconstructed widgets must never have Slate widgets spawned as a side-effect of inspection. Inspection only queries `Widget->GetCachedWidget()`.
- **Passive Geometry Extraction**: Screen-space positions and dimensions are read exclusively through `SWidget::GetTickSpaceGeometry()`. `SlatePrepass()` and `Invalidate()` are never invoked.
- **Passive Focus Extraction**: Keyboard and user focus are checked passively via `FSlateApplication::Get().GetKeyboardFocusedWidget()` and `GetUserFocusedWidget(0)`. Focus is never set or modified (`SetFocus` / `SetKeyboardFocus` are forbidden).

### Decision 2: Resolution & Ambiguity Contract
- **Explicit World Context**:
  - If `WorldContext` is specified, matches exact name or path against active PIE/Game worlds.
  - If empty, auto-detects active PIE/Game world. If 0 found -> `Unavailable`. If > 1 found -> `Ambiguous` with full candidate paths.
- **Explicit Widget Reference**:
  - If `WidgetReference` is empty, resolves the single eligible top-level `UUserWidget` in the world (`GetParent() == nullptr`). If multiple exist -> `Ambiguous` with candidate paths. Never guesses.
  - If specified, matches exact `GetPathName()`, then unique `GetName()`, then unique class name. If multiple match -> `Ambiguous`. If 0 match -> `Unavailable`.

### Decision 3: Traversal Safety & Bounded Recursion
- **Cycle Prevention**: A `TSet<UWidget*> VisitedWidgets` tracker ensures that circular object references or nested widgets cannot induce infinite recursion.
- **Depth Clamping**: `MaxDepth` is clamped to `[1, 100]` (default 32). Traversals exceeding depth set `Report.bTruncated = true`.
- **Node Cap**: Total inspected nodes are capped at 500. Exceeding this limit stops expansion and sets `Report.bTruncated = true`.

### Decision 4: NVDA Output Architecture — Active vs Hidden Separation
- **Headline Summary**: Outputs root widget, target world, total widgets, active count, hidden/collapsed count, and currently focused widget name.
- **Active Controls Hierarchy**: Formats active/visible controls using screen-reader-friendly hierarchical numbering (`1.`, `1.1.`, `1.1.2.`).
- **Inactive / Collapsed / Hidden Subtrees**: Placed in a separate, dedicated section at the bottom of the transcript. This prevents screen-reader users from wading through collapsed dialogs or disabled layers while inspecting active UI.

### Decision 5: Concrete Test Subclass (`UTestConcreteUserWidget`)
- In Unreal Engine, `UUserWidget` carries the `CLASS_Abstract` flag, which generates engine-level load warnings when instantiated directly in test worlds.
- Implemented `UTestConcreteUserWidget` in `AccessibleUMGInspectorTests` as a concrete subclass of `UUserWidget`, eliminating all abstract instantiation warnings and ensuring clean test execution with zero warnings.

---

## 3. Verification & Regression Results

- Built targeting Unreal Engine 5.8.2 via `RunUAT.bat BuildPlugin`.
- **31 of 31 Automated Tests Passed** (`ExitCode: 0`):
  - 11 Serialized Blueprint Tests (Milestone 1 baseline preserved with zero regression).
  - 20 Runtime Viewport / UserWidget Tests (Milestone 2 implementation fully validated).

---

## 4. Milestone Boundary Observance

The following items remain strictly **excluded** from Milestone 2:
- Differential comparison between serialized and runtime structures (`CompareSerializedAndRuntime`, Milestone 3).
- Accessibility audit rules and violation reporting (`AuditWidgetAccessibility`, Milestone 4).
- GridFall project integration or live workspace testing (Milestone 5).
- Input simulation, property modification, or automatic repair tools (Excluded from read-only core).
- Unreal PIE lifecycle control (Inspector does not start or stop PIE).
