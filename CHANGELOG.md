# Changelog

All notable changes to the Accessible UMG Inspector plugin are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.1.0-rc.1] - 2026-10-01

### Added
- **`GetInspectorCapabilities`**: Exposes plugin version, supported engine version (`5.8.2`), supported operations, output format choices, and strict read-only safety guarantees.
- **`InspectSerializedWidgetBlueprint`**: Authoritative, read-only introspection of `UWidgetBlueprint` assets. Catalogs widget hierarchy, slot properties, accessibility attributes (`AccessibleName`, `AccessibleSummary`, `AccessibleBehavior`), navigation rules, delegate bindings, and package clean-state invariance.
- **`InspectRuntimeUserWidget`**: Read-only introspection of live `UUserWidget` instances in active PIE and Game viewports. Passive extraction of Slate geometry (`GetTickSpaceGeometry()`), passive focus states (`FSlateApplication`), hover states, and accessibility properties without invoking `TakeWidget()`.
- **`CompareSerializedAndRuntime`**: Differential reconciliation engine mapping authored Blueprint nodes to live runtime Slate/UMG nodes. Categorizes elements as `Matched`, `Ambiguous`, `SerializedOnly`, `RuntimeOnly`, `Unavailable`, or `Unsupported`.
- **`AuditWidgetAccessibility`**: Automated accessibility evaluation covering 14 rules (`A11Y-01` through `A11Y-14`) across serialized, runtime, and differential evidence. Classifies findings as `Error`, `Warning`, `Info`, or `NotEvaluated`, grounded in explicit confidence bases (`Direct`, `Derived`, `Ambiguous`, `Unavailable`).
- **Two-Tier Visibility Model**: Decoupled `LocalVisibility` from `EffectiveVisibility` with top-down ancestor propagation (`ComputeEffectiveVisibility`). Inactive/collapsed container panels faithfully suppress descendant evaluation without mutating authored local design properties.
- **Dual Output Format**: High-density structured JSON for automation alongside clean, hierarchically numbered plain text optimized for NVDA and screen-reader review.
- **Native MCP Integration**: Automatic discovery and registration via Epic's `UToolsetRegistry` in `AccessibleUMGInspectorModule`.
- **Automation Test Suite**: 112 headless automation tests covering serialized reflection, runtime inspection, differential comparison, audit rule evaluation, and synthetic replay scenarios.

### Fixed
- **Ancestor State Conflation (Milestone 5 Correction)**: Resolved 44 false-positive errors on multi-screen game shells where child controls retained local `Visible` while their ancestor container was `Collapsed`. Replaced single-tier visibility checks with authoritative ancestor propagation, reducing start-screen false-positive errors to 0.
- **Pointer Hit-Testing Invalidation**: Ensured `HitTestInvisible` ancestor state correctly propagates to disable `bEffectivelyInteractive` on descendant controls.
- **Differential Visibility Discrepancy Suppression**: Suppressed false visibility and geometry mismatch warnings when a child's local `Visible` state is suppressed by an inactive ancestor.

### Security & Safety
- Strict read-only operation guaranteed across all operations: zero package dirtiness, zero transaction recording, zero synthetic input simulation, and zero process spawning.
