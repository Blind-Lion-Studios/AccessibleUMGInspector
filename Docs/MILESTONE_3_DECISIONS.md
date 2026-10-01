# Milestone 3 Architectural Decisions & Record

**Repository**: `Blind-Lion-Studios/AccessibleUMGInspector`  
**Milestone**: Milestone 3 (Serialized-to-Runtime Reconciliation Core)  
**Status**: Accepted & Implemented  
**Date**: 2026-10-01  

---

## 1. Context and Proven Motivations

From the isolated proofs in GridFall:
1. **Epic UMGToolSet Authoritative Serialized Structure**: Successfully serialized Widget Blueprint declarations, slot layout properties, and authored accessibility metadata.
2. **Dedicated Runtime Inspector**: Successfully captured live in-viewport widget states, passive Slate geometry (`GetTickSpaceGeometry()`), and passive keyboard focus without Editor docking chrome interference.
3. **The Differential Reconciliation Gap**: Serialized asset trees and live runtime widget trees are not identical 1-to-1 graphs. Live UMG trees feature internal Slate composition nodes, dynamically generated controls, collapsed/hidden subtrees without Slate representation, and reparented nodes. A deterministic reconciliation engine is essential to correlate authored intent against live reality.

---

## 2. Accepted Milestone 3 Decisions

### Decision 1: Classification Taxonomy
Every reconciled node is assigned one of six deterministic classifications:
- **`Matched`**: Exactly 1-to-1 correspondence established between serialized declaration and live runtime node.
- **`Ambiguous`**: Multiple serialized or runtime candidates exist that cannot be uniquely resolved, or class conflicts occur.
- **`SerializedOnly`**: Declared in Blueprint but absent from live runtime hierarchy. Distinguishes between valid Collapsed/Hidden unconstructed controls and unexpected missing widgets.
- **`RuntimeOnly`**: Present in live runtime hierarchy without a corresponding Blueprint declaration (dynamically created controls or Slate composition nodes).
- **`Unavailable`**: Input report failed to load or world/widget target could not be resolved.
- **`Unsupported`**: Target object does not support differential comparison.

### Decision 2: Multi-Pass Deterministic Matching Priority
Reconciliation follows a strict multi-pass priority chain:
1. **Pass 1 — Unambiguous 1-to-1 Candidates**: Matches nodes having an exact case-insensitive source name match and compatible reflection class with exactly 1 runtime candidate and 1 serialized candidate. Parent containment and child order are evaluated as evidence/discrepancies rather than dropping valid identity.
2. **Pass 2 — Parent-Chain Disambiguation**: Multi-candidate widgets sharing identical names (e.g. repeated buttons in different panels) are disambiguated by matching their parent container name.
3. **Pass 3 — Ambiguity Escalation**: If multiple serialized or runtime candidates remain ambiguous after parent filtering (e.g. duplicate buttons in the same container), all candidate records are classified as `Ambiguous`.
4. **Pass 4 — Class Conflict Escalation**: If names match but reflection classes conflict (e.g. Serialized Button vs Runtime TextBlock), the pair is escalated to `Ambiguous` with explicit conflict evidence.
5. **Pass 5 — SerializedOnly Classification**: Unmatched serialized widgets are recorded as `SerializedOnly`. Controls authored with default visibility `Collapsed` or `Hidden` receive valid non-error evidence explaining that live Slate nodes are omitted by design.
6. **Pass 6 — RuntimeOnly Classification**: Unmatched runtime widgets are recorded as `RuntimeOnly` with evidence identifying dynamic instantiation or internal Slate composition.

### Decision 3: Discrepancy Detection Without Identity Drop
Discrepancies are identified and tracked on matched and ambiguous nodes:
- **Parent/Containment Mismatches**: Serialized parent container differs from runtime parent container.
- **Child-Order Mismatches**: Relative container index shifted at runtime.
- **Visibility Discrepancies**: Authored default visibility differs from live runtime visibility.
- **State Discrepancies**: Interactive enabled state or keyboard focusability differs between serialized and runtime.
- **Accessibility Text Discrepancies**: Authored accessible name or summary label differs from dynamic runtime label.
- **Geometry Discrepancies**: Overlapping interactive bounds, zero-size interactive controls, or off-screen interactive controls.

### Decision 4: Deterministic NVDA & JSON Output Ordering
To ensure screen-reader efficiency and priority triage:
1. `Ambiguous` entries appear first to immediately surface identity conflicts.
2. `Matched` entries appear next in source-tree order.
3. `SerializedOnly` entries follow, clearly separating uninstantiated controls.
4. `RuntimeOnly` entries appear last.

### Decision 5: Non-Mutating Passive Reconciliation
- Reconciles reports by accepting pre-existing reports or by running serialized and runtime inspectors serially.
- Never creates, modifies, or invalidates Slate or UMG widgets.
- Never runs PIE lifecycle commands or synthetic input simulation.
- Accessible names and screen bounds are used strictly as supporting evidence and discrepancy indicators, never standalone identity proofs.

---

## 3. Verification & Regression Results

- Built targeting Unreal Engine 5.8.2 via `RunUAT.bat BuildPlugin`.
- **57 of 57 Automated Tests Passed** (`ExitCode: 0`):
  - 11 Serialized Blueprint Tests (Milestone 1 baseline preserved with zero regression).
  - 20 Runtime Viewport / UserWidget Tests (Milestone 2 baseline preserved with zero regression).
  - 26 Differential Comparator Tests (Milestone 3 implementation fully validated).

---

## 4. Milestone Boundary Observance

The following items remain strictly **excluded** from Milestone 3:
- Accessibility audit rules and violation reporting (`AuditWidgetAccessibility`, Milestone 4).
- GridFall project integration or live workspace testing (Milestone 5).
- Input simulation, property modification, or automatic repair tools (Excluded from read-only core).
- Unreal PIE lifecycle control (Inspector does not start or stop PIE).
