# Security and Safety Specification: Accessible UMG Inspector

## Core Safety Philosophy
Accessible UMG Inspector is engineered with a strict, verifiable read-only safety design. It serves as an observational inspection layer that developers, QA engineers, and automated agents can trust never to corrupt assets, alter runtime states, or introduce memory leaks.

---

## 1. Package and Asset Immutability
- **Zero Asset Dirtying**: Inspection of serialized assets uses passive object reflection. At no point is `Modify()`, `MarkPackageDirty()`, or `SetDirtyFlag(true)` called.
- **Package Verification**: `InspectSerializedWidgetBlueprint` verifies `UPackage::IsDirty()` before and after inspection. If the package was clean prior to inspection, it remains clean after inspection as verified by pre- and post-inspection package state checks.
- **Zero Asset Saving**: Neither `SavePackage()` nor any file serialization API is ever invoked.
- **Zero Blueprint Recompilation**: Assets are inspected as-is without invoking `FKismetEditorUtilities::CompileBlueprint` or mutating class generated headers.

---

## 2. Runtime Viewport and Memory Safety
- **Zero Slate Allocation (`No TakeWidget`)**:
  - In Unreal Engine, calling `UWidget::TakeWidget()` forces the creation of a new underlying `SWidget` instance, which alters widget tree lifecycle states.
  - Accessible UMG Inspector inspects live Slate widgets **exclusively** through `UWidget::GetCachedWidget()`.
  - If a widget has not yet generated a Slate representation, `GetCachedWidget()` returns `nullptr`. The inspector reports `bHasSlateWidget = false` without forcing Slate widget construction.
- **Passive Focus Query**:
  - Focus is inspected passively via `FSlateApplication::Get().GetKeyboardFocusedWidget()` and `GetUserFocusedWidget(0)`.
  - The inspector never invokes `SetFocus()`, `SetKeyboardFocus()`, or `SetUserFocus()`.
- **Zero Synthetic Input Dispatch**:
  - The plugin does not dispatch synthetic key events, mouse clicks, hover events, or touch gestures.
  - APIs like `FSlateApplication::ProcessKeyDownEvent`, `ProcessMouseButtonDownEvent`, or `OnMouseEnter` are strictly forbidden.
- **Zero In-Memory Caches**:
  - All report generation operations are functional and stateless.
  - No static or persistent caches hold raw `UObject*`, `UWidget*`, or `TSharedPtr<SWidget>` references across inspection calls.

---

## 3. Network and Process Boundaries
- **No Additional Network Transport**:
  - Accessible UMG Inspector does not spawn HTTP servers, WebSocket listeners, or separate IPC pipes.
  - All external communication occurs strictly through Epic's native `ModelContextProtocol` endpoint (local-only at `127.0.0.1:8000`).
- **No Process Spawning**:
  - The plugin does not invoke `FPlatformProcess::CreateProc`, `Start-Process`, or any shell execution commands.
  - PIE sessions and Editor lifecycles are controlled externally by the developer or AI coordinator.

---

## 4. Static Prohibited-API Source Scan
A static prohibited-API source scan of all production code (`Source/AccessibleUMGInspector`) was conducted to verify absence of prohibited mutation or interaction APIs:

| Prohibited API | Production Matches | Classification | Result |
| :--- | :--- | :--- | :--- |
| `Modify` | 0 | None | **Clean** |
| `MarkPackageDirty` | 0 | None | **Clean** |
| `SetDirtyFlag` | 0 | None | **Clean** |
| `SavePackage` | 0 | None | **Clean** |
| `CompileBlueprint` | 0 | None | **Clean** |
| `TakeWidget` | 1 | Inline comment disclaimer | **Clean** |
| `AddToViewport` | 0 | None | **Clean** |
| `RemoveFromParent` | 0 | None | **Clean** |
| `SetVisibility` | 0 | None | **Clean** |
| `SetIsEnabled` | 0 | None | **Clean** |
| `SetFocus` | 0 | None | **Clean** |
| `SetKeyboardFocus` | 0 | None | **Clean** |
| `SetUserFocus` | 0 | None | **Clean** |
| `ProcessKey` | 0 | None | **Clean** |
| `ProcessMouse` | 0 | None | **Clean** |
| `Click` | 0 | None | **Clean** |
| `Drag` | 0 | None | **Clean** |
| `Hover` | 2 | Finding limitation strings | **Clean** |
| `Type` | 4 | Slate / Slot type string formatting | **Clean** |
| `StartPIE` | 0 | None | **Clean** |
| `StopPIE` | 0 | None | **Clean** |

Every check passed with **zero unauthorized invocations**.
