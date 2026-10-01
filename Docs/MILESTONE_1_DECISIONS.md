# Milestone 1 Architectural Decisions & Record

**Repository**: `Blind-Lion-Studios/AccessibleUMGInspector`  
**Milestone**: Milestone 1 (Serialized Read-Only Inspector Core)  
**Status**: Accepted & Implemented  
**Date**: 2026-10-01  

---

## 1. Context and Proven Motivations

From the isolated proofs in GridFall:
1. **Epic UMGToolSet**: Successfully proves authoritative serialized property extraction via reflection, but lacks dedicated, unified accessibility aggregation tools and requires iterative multi-step property queries.
2. **Epic SlateInspectorToolset**: Successfully provides general Slate UI snapshots, but generic root-window snapshots exhaust depth (limit 30) inside Unreal Editor docking chrome (`SWindow` -> `SDockTab` -> `SSplitter`) before ever reaching in-viewport PIE game widgets. It also prunes collapsed widgets and lacks UMG object identity.
3. **Dedicated Read-Only Inspector Justification**: An independent, accessibility-first plugin directly querying `UWidgetBlueprint` and `UWidgetTree` provides immediate, deterministic, non-mutating reports without editor-chrome clutter or depth exhaustion.

---

## 2. Accepted Milestone 1 Decisions

### Decision 1: Standalone Editor Plugin Structure
- Maintain `AccessibleUMGInspector` as a standalone Editor-only plugin with its own repository at `C:\BLS\AccessibleUMGInspector`.
- Module split:
  - `AccessibleUMGInspector`: Production inspection core, formats, and reflected MCP toolset.
  - `AccessibleUMGInspectorTests`: Synthetic automation test suite and unit test fixtures.

### Decision 2: Unreal MCP & ToolsetRegistry Integration
- Inherit from `UToolsetDefinition`.
- Tools registered as static UFunctions with `meta = (AICallable)`.
- No secondary or custom MCP servers; communicates through Unreal Engine's native loopback MCP server (`http://127.0.0.1:8000/mcp`).

### Decision 3: Direct Unreal API Inspection Core
- Direct inspection of `UWidgetBlueprint`, `UWidgetTree`, `UWidget`, `UPanelSlot`, `UWidgetNavigation`, and `AccessibleWidgetData`.
- Gathers full structural layout, slot properties, accessibility text/summary overrides, default visibility, enabled state, focusability, variable state, and delegate bindings in a single synchronous traversal pass.

### Decision 4: Strict Read-Only Contract
- Enforced at the architectural level:
  - No `UObject::Modify()`.
  - No `UPackage::SetDirtyFlag()` or `MarkPackageDirty()`.
  - No transaction recording (`GUndo`).
  - Pre-inspection package dirty check: captures `Package->IsDirty()`.
  - Post-inspection package dirty check: verifies `Package->IsDirty()`. If a clean package becomes dirty during inspection, the report explicitly fails with `bPackageRemainsCleanAfter = false`.

### Decision 5: NVDA-Friendly Dual Output
- Emits structured JSON (`JsonData`) and screen-reader-optimized plain text (`NvdaText`).
- Text formatting principles:
  - Headline summary first.
  - Actionable warnings (duplicate names, missing labels) immediately below headline.
  - Hierarchical numbering (`1.`, `1.1.`, `1.1.2.`) to remove visual-only whitespace indentation dependency.

### Decision 6: Explicit Asset Path Requirement
- Tools strictly require an explicit, project-neutral asset path (e.g. `"/Game/UI/WBP_Menu"`).
- Broad project sweeps or unconstrained asset searches are prohibited.

---

## 3. Milestone Boundary Verification

The following items are intentionally **excluded** from Milestone 1:
- Runtime viewport / `UUserWidget` inspection (Milestone 2).
- Serialized-to-runtime reconciliation and diffing (Milestone 3).
- Advanced accessibility audit rule engine (Milestone 4).
- GridFall workspace integration and live testing (Milestone 5).
- Input simulation, property modification, or automatic repairs (Deferred permanently from read-only core).
