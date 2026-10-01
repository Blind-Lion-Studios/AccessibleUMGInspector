# Accessible UMG Inspector — Development Guidelines

These instructions govern all AI agent and developer contributions to the `AccessibleUMGInspector` repository.

---

## 1. Safety & Git Boundaries

1. **Explicit Human Authorization for Git Mutations**:
   - STOP and wait for explicit human authorization before executing any `git commit`, `git push`, `git merge`, `git branch`, `git reset`, `git checkout`, or remote creation commands.
   - Do not stage or commit without an explicit prompt instruction specifying the commit scope.
2. **Never Touch Unreal Generated Files**:
   - `Binaries/`, `Intermediate/`, `DerivedDataCache/`, and `Saved/` must remain untracked in `.gitignore`.
3. **Strict Workspace Isolation**:
   - Never access, inspect, build, or modify `C:\BLS\GridFall` or any other project repository during development of this tool unless explicitly authorized for cross-project integration testing.

---

## 2. Read-Only Safety Principles

Every tool and inspection function in this plugin must adhere to the read-only contract:
1. **No Mutations**: Never call `Modify()`, `MarkPackageDirty()`, property setters, or execute state-altering delegates.
2. **No Compilation or Saves**: Never trigger Blueprint compilation, asset saving, or file overwrites.
3. **No Synthetic Input**: Never dispatch synthetic mouse, keyboard, or touch events.
4. **Dirty State Verification**: Before opening an asset, record `Package->IsDirty()`. After inspection, re-verify `Package->IsDirty()`. If inspection changes a package from clean to dirty, immediately flag an invariant violation.
5. **No PIE Control**: The inspector does not start, stop, or pause PIE sessions.

---

## 3. Accessibility-First Output Discipline

1. **Dual Representation**:
   - Every operation must support structured JSON (for programmatic ingestion) and screen-reader-optimized plain text (for NVDA and Braille displays).
2. **NVDA Text Requirements**:
   - Begin with a short, complete status headline.
   - Place actionable warnings (e.g. unlabeled buttons, duplicate names) at the very top of the output.
   - Use hierarchical numbering (`1.`, `1.1.`, `1.1.2.`) so blind developers can track parent-child depth without relying on visual indentation.
   - Include direction-based navigation rules and bound delegates clearly.

---

## 4. Supported Engine Version & Lifecycle

- **Baseline Engine Version**: **Unreal Engine 5.8.2** (`BuildId: 55116800`).
- **Transport**: Registered via `UToolsetRegistry` for Unreal MCP on `127.0.0.1:8000`.
- **Target Allow List**: `"Editor"`. Never include runtime game client build targets.
- **Detached Lifecycle**: When testing with an Unreal Editor instance, never use synchronous process blocking (`Start-Process -Wait`). Use asynchronous, bounded polling or the project lifecycle scripts.

---

## 5. Milestone Boundaries

- **Milestone 1**: Standalone repository structure, Serialized Widget Blueprint inspection core (`GetInspectorCapabilities`, `InspectSerializedWidgetBlueprint`), NVDA text formatter, JSON serialization, synthetic unit tests.
- **Milestone 2**: Runtime Viewport & UserWidget inspection core (`InspectRuntimeUserWidget`).
- **Milestone 3**: Differential reconciliation engine (`CompareSerializedAndRuntime`).
- **Milestone 4**: Automated accessibility audit rule engine (`AuditWidgetAccessibility`).
- **Milestone 5**: Isolated project integration testing.
- **Milestone 6**: Documentation, accessible command reference, and release tagging.

Do not start a subsequent milestone without explicit human authorization.
