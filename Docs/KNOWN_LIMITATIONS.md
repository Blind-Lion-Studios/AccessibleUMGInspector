# Known Limitations and Architectural Boundaries

## Overview
Accessible UMG Inspector is designed to provide authoritative, non-mutating UMG and Slate hierarchy evidence. To maintain scientific integrity and prevent false assumptions, this document explicitly details the architectural boundaries and non-claims of the toolset.

---

## 1. No Hit-Test Grid Introspection
- **Observed Evidence**: The inspector reads absolute screen position and size via `SWidget::GetTickSpaceGeometry()`.
- **Limitation**: Screen bounds do **not** prove pointer routing, clipping, or hit-test grid membership.
- **Why It Matters**: Two controls may have non-overlapping bounds but still fail hit-testing due to invisible overlay panels, clipping boundaries, or custom widget hit-test overrides (`OnPaint`, `ComputeVolatility`).
- **Human Review**: Interactive click/tap verification and pointer testing remain required.

---

## 2. No OS Accessibility-Tree Equivalence (UIA / IA2)
- **Observed Evidence**: The inspector reflects authored properties on `UWidget::AccessibleWidgetData` and Slate accessibility flags (`bOverrideAccessibleDefaults`, `AccessibleName`, `AccessibleSummary`, `AccessibleBehavior`).
- **Limitation**: Internal Unreal Engine accessibility properties do **not** prove operating system accessibility tree exposure.
- **Why It Matters**: On Windows, accessibility bridges (such as UI Automation or IAccessible2) may prune, filter, or fail to expose Slate accessibility nodes due to third-party screen-reader drivers or OS-level accessibility bugs.
- **Human Review**: End-to-end testing with a live screen reader (NVDA, Narrator, JAWS) running against the active game viewport is mandatory.

---

## 3. No Direct Screen-Reader Audio or Speech Verification
- **Observed Evidence**: The inspector formats plain-text reports (`NvdaText`) designed for optimal navigation with NVDA review shortcuts.
- **Limitation**: The inspector does **not** communicate directly with screen-reader processes or verify speech synthesis.
- **Why It Matters**: Plain-text formatting ensures that a developer reading an automated report can navigate efficiently; it does not verify that a player in the game will hear the intended voice announcement.
- **Human Review**: Human audio review must confirm that focus announcements, speech rate, and audio cues operate correctly during live gameplay.

---

## 4. No Crash-Cause Detection or Diagnosis
- **Observed Evidence**: Discrepancies between serialized declarations and runtime hierarchies (e.g. visibility divergence, missing bindings) are logged as observational findings.
- **Limitation**: Findings describe structural divergence; they do **not** diagnose engine-level crashes, memory corruption, or driver panics.
- **Why It Matters**: If a game crashes during screen transitions, an audit finding (such as `A11Y-10 VisibilityStateMismatch`) identifies an unexpected state, but cannot prove causality without memory dump and call-stack analysis.
- **Human Review**: Crash diagnosis requires standard C++ debugging tools (Visual Studio, WinDbg, Unreal Crash Reporter).

---

## 5. Strict Read-Only Operation (No Automatic Remediation)
- **Observed Evidence**: Findings include `RemediationGuidance` suggesting specific developer actions (e.g., "Assign accessible name in Accessibility properties").
- **Limitation**: The inspector will **never** automatically apply fixes, edit blueprints, or mutate C++ source code.
- **Why It Matters**: Automatic remediation in complex game UIs frequently introduces unintended side effects (breaking animation timelines, altering binding delegates, or violating gameplay state machines).
- **Human Responsibility**: Developers and technical designers must author and verify all UI changes intentionally.

---

## 6. Engine Version Boundary
- **Supported Version**: Unreal Engine **5.8.2** (`BuildId: 55116800`).
- **Limitation**: Not tested, validated, or supported on Unreal Engine 5.7, 5.6, or earlier versions.
- **Why It Matters**: UMG reflection properties, `FSlateApplication` focus query interfaces, and `ModelContextProtocol` toolset registrations undergo breaking changes between major engine versions.
