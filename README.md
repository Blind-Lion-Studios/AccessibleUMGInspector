# Accessible UMG Inspector

Accessible UMG Inspector is a dedicated, read-only Unreal Engine Editor plugin created by Blind Lion Studios for blind and visually impaired (BVI) developers, technical designers, and QA engineers.

The plugin provides accessibility-first inspection of authored UMG Widget Blueprints and live runtime viewport UI state, exposing dual output formats: structured JSON for automation and concise, hierarchically numbered plain text optimized for NVDA and screen-reader review.

All capabilities are accessible via Epic's native Unreal MCP (Model Context Protocol) and ToolsetRegistry transport.

---

## What the tool does

1. Read-only Unreal Engine Editor plugin: Operates passively without modifying assets, dirtying packages, or altering runtime UI states.
2. Serialized UMG inspection: Authoritative reflection of `UWidgetBlueprint`, `UWidgetTree`, `UWidget`, `UPanelSlot`, `AccessibleWidgetData`, navigation rules, and delegate bindings.
3. Live runtime UMG and cached Slate inspection: Passive inspection of live `UUserWidget` instances, `GetTickSpaceGeometry()` screen coordinates, focus states via `FSlateApplication`, hover states, and effective visibility without forcing Slate widget construction (`TakeWidget` is never invoked).
4. Serialized and runtime comparison: Differential reconciliation classifying nodes as Matched, Ambiguous, SerializedOnly, RuntimeOnly, Unavailable, or Unsupported, recording discrepancies across visibility, state, accessibility, and geometry.
5. Evidence-qualified accessibility audits: Evaluates automated rules (A11Y-01 through A11Y-14) with explicit confidence bases (`Direct`, `Derived`, `Ambiguous`, `Unavailable`), rule limitations, and actionable remediation guidance.
6. Hierarchically numbered NVDA-friendly text: Screen-reader-friendly plain-text reports formatted with clear parent-child numbering (`1.`, `1.1.`, `1.1.2.`) so blind developers can navigate UI hierarchies without relying on visual indentation.
7. Structured JSON: Complete machine-readable output for automated pipelines, external tools, and agent workflows.

---

## Current support

- Engine version: Unreal Engine 5.8.2 (`BuildId: 55116800`)
- Platform: Windows Editor (`Win64`, `TargetAllowList: ["Editor"]`)
- Transport: Epic native Unreal MCP (`http://127.0.0.1:8000/mcp`) via `ToolsetRegistry`
- Release status: Experimental release candidate 0.1.0-rc.1

---

## What it does not do

1. No asset remediation: Does not edit Blueprints, mutate packages, or automatically apply fixes.
2. No user-input simulation: Does not dispatch synthetic keystrokes, mouse clicks, hover events, or touch gestures.
3. No PIE lifecycle control: Does not start, stop, or pause PIE (Play-In-Editor) or Game sessions.
4. No OS accessibility-tree inspection: Does not introspect Windows UI Automation (UIA) or IAccessible2 (IA2) trees.
5. No direct NVDA verification: Does not directly drive screen-reader audio, speech synthesis, or verify spoken output.
6. No hit-test-grid verification: Does not introspect the Slate hit-test grid, pointer routing, or clipping rules.
7. No accessibility certification: Automated audit findings are development evidence, not certification of legal or regulatory compliance.
8. No crash-cause determination: Discrepancies surface structural divergence, not engine crash diagnostics or memory corruption causes.

---

## Project status

Accessible UMG Inspector is provided as-is as an experimental accessibility development tool.

Blind Lion Studios does not guarantee support, maintenance, compatibility updates, issue resolution, or continued development. Contributions may be reviewed when capacity and project priorities allow.

Automated findings produced by this tool are development evidence, not certification of accessibility or regulatory compliance.

---

## Contributions

Focused bug reports and narrowly scoped pull requests are welcome. Submission does not guarantee review, acceptance, support, or a response within any particular period.

Before proposing new infrastructure or major features, describe the verified accessibility gap, existing alternatives considered, and why the change belongs in this plugin.

---

## Future accessibility opportunities

This repository may grow when real Unreal Engine development exposes reusable accessibility gaps that existing mature tools do not solve.

Potential future work should remain evidence-driven, bounded, read-only where practical, and useful beyond one game project. Ideas may include focus-path inspection, transition-state comparison, accessible automation summaries, crash-evidence narration, input-mapping audits, audio-accessibility inspection, subtitle auditing, and text representations of visual Unreal assets.

This section is not a committed roadmap.

---

## Toolset operations

All operations belong to the toolset `AccessibleUMGInspector.AccessibleUMGInspectorToolset`:

1. `GetInspectorCapabilities`: Returns capability metadata, supported operations, format types, and read-only safety design flags.
2. `InspectSerializedWidgetBlueprint`: Inspects an authored `UWidgetBlueprint` asset on disk/memory given an explicit asset path.
3. `InspectRuntimeUserWidget`: Inspects live `UUserWidget` instances in active PIE or Game viewports.
4. `CompareSerializedAndRuntime`: Reconciles an authored asset against live runtime UI state.
5. `AuditWidgetAccessibility`: Evaluates accessibility rules (A11Y-01 through A11Y-14) across serialized, runtime, and differential evidence.

---

## Documentation index

- [Architecture Specification](Docs/ARCHITECTURE.md)
- [Audit Rule Reference (A11Y-01 to A11Y-14)](Docs/AUDIT_RULE_REFERENCE.md)
- [MCP Tool Reference](Docs/MCP_TOOL_REFERENCE.md)
- [NVDA Command & Navigation Guide](Docs/NVDA_COMMAND_GUIDE.md)
- [Installation Guide](Docs/INSTALLATION.md)
- [Removal Guide](Docs/REMOVAL.md)
- [Testing & Automation Guide](Docs/TESTING.md)
- [Security & Safety Specification](Docs/SECURITY_AND_SAFETY.md)
- [Known Limitations & Non-Claims](Docs/KNOWN_LIMITATIONS.md)
- [Compatibility Matrix](Docs/COMPATIBILITY.md)
- [Case Study: Isolated GridFall Proving Proof](Docs/CASE_STUDY_GRIDFALL_ISOLATED_PROOF.md)
- [Development Guidelines](Docs/DEVELOPMENT_GUIDELINES.md)
- [Changelog](CHANGELOG.md)
- [License](LICENSE)

---

## License & Ownership

Copyright Blind Lion Studios. All Rights Reserved. Licensed under the MIT License.
