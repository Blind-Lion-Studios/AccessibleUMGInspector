# Accessible UMG Inspector

**Accessible UMG Inspector** is a dedicated, read-only Unreal Engine 5.8 Editor plugin created by **Blind Lion Studios** for blind and visually impaired (BVI) developers, technical designers, and QA engineers.

It exposes accessibility-first inspection of serialized UMG Widget Blueprints and runtime UI state via Epic's native **Unreal MCP** and **ToolsetRegistry** transport.

---

## Key Principles

1. **Accessibility-First Output**: Provides dual output: structured JSON for automation and concise, hierarchically numbered plain text optimized for NVDA and screen-reader review.
2. **Strict Read-Only Safety**: Zero package dirtying, zero asset modification, zero transaction mutations, and zero synthetic input simulation.
3. **Deterministic Serialized & Runtime Reflection**: Authoritative introspection of `UWidgetBlueprint`, `UWidgetTree`, `UWidget`, `UPanelSlot`, `AccessibleWidgetData`, and navigation rules.
4. **Two-Tier Visibility & Ancestor Propagation**: Decouples authored local widget visibility (`LocalVisibility`) from effective runtime exposure (`EffectiveVisibility`). Inactive/collapsed ancestor screens faithfully suppress descendant evaluation without discarding authored local design properties.
5. **Project-Neutral**: Contains no hardcoded project names, game shell states, or proprietary control identifiers.

---

## Supported Engine Version

- **Plugin Version**: **0.1.0-rc.1**
- **Target Engine**: Unreal Engine **5.8.2** (`BuildId: 55116800`)
- **Target Platform**: Win64 Editor (`TargetAllowList: ["Editor"]`)
- **Transport**: Native Unreal MCP (`http://127.0.0.1:8000/mcp`) via `ToolsetRegistry`

---

## Toolset Surface

### `AccessibleUMGInspectorToolset.GetInspectorCapabilities`
Returns capability metadata, supported operations, format types, and safety guarantees.

### `AccessibleUMGInspectorToolset.InspectSerializedWidgetBlueprint`
Accepts an explicit asset path (e.g. `"/Game/UI/WBP_Menu"`) and returns complete serialized widget hierarchy, slot properties, accessibility attributes, navigation rules, bindings, duplicate name warnings, and package clean-state verification.

### `AccessibleUMGInspectorToolset.InspectRuntimeUserWidget`
Inspects live runtime `UUserWidget` instances in active PIE or Game viewports. Captures active visibility, passive Slate geometry (`GetTickSpaceGeometry()`), passive focus states (`FSlateApplication`), hover states, and accessibility properties without spawning Slate widgets (`TakeWidget` is never called).

### `AccessibleUMGInspectorToolset.CompareSerializedAndRuntime`
Reconciles an authored `UWidgetBlueprint` asset against a live runtime `UUserWidget` hierarchy. Classifies every node (`Matched`, `Ambiguous`, `SerializedOnly`, `RuntimeOnly`, `Unavailable`, `Unsupported`), records structural/visibility/state/accessibility/geometry discrepancies, and surfaces non-mutating evidence chains.

### `AccessibleUMGInspectorToolset.AuditWidgetAccessibility`
Evaluates automated accessibility rules (A11Y-01 through A11Y-14) across serialized, runtime, and differential evidence. Classifies findings (`Error`, `Warning`, `Info`, `NotEvaluated`), grounds assertions in explicit confidence bases (`Direct`, `Derived`, `Ambiguous`, `Unavailable`), provides remediation guidance and limitations, and generates screen-reader-optimized NVDA text and structured JSON.

---

## Documentation Index

- [Architecture Specification](file:///C:/BLS/AccessibleUMGInspector/Docs/ARCHITECTURE.md)
- [Audit Rule Reference (A11Y-01 to A11Y-14)](file:///C:/BLS/AccessibleUMGInspector/Docs/AUDIT_RULE_REFERENCE.md)
- [MCP Tool Reference](file:///C:/BLS/AccessibleUMGInspector/Docs/MCP_TOOL_REFERENCE.md)
- [NVDA Command & Navigation Guide](file:///C:/BLS/AccessibleUMGInspector/Docs/NVDA_COMMAND_GUIDE.md)
- [Installation Guide](file:///C:/BLS/AccessibleUMGInspector/Docs/INSTALLATION.md)
- [Removal Guide](file:///C:/BLS/AccessibleUMGInspector/Docs/REMOVAL.md)
- [Testing & Automation Guide](file:///C:/BLS/AccessibleUMGInspector/Docs/TESTING.md)
- [Security & Safety Specification](file:///C:/BLS/AccessibleUMGInspector/Docs/SECURITY_AND_SAFETY.md)
- [Known Limitations & Non-Claims](file:///C:/BLS/AccessibleUMGInspector/Docs/KNOWN_LIMITATIONS.md)
- [Compatibility Matrix](file:///C:/BLS/AccessibleUMGInspector/Docs/COMPATIBILITY.md)
- [Case Study: Isolated GridFall Proving Proof](file:///C:/BLS/AccessibleUMGInspector/Docs/CASE_STUDY_GRIDFALL_ISOLATED_PROOF.md)
- [Development Guidelines](file:///C:/BLS/AccessibleUMGInspector/Docs/DEVELOPMENT_GUIDELINES.md)
- [Changelog](file:///C:/BLS/AccessibleUMGInspector/CHANGELOG.md)
- [License](file:///C:/BLS/AccessibleUMGInspector/LICENSE)

---

## License & Ownership

Copyright Blind Lion Studios. All Rights Reserved. Licensed under the MIT License.
