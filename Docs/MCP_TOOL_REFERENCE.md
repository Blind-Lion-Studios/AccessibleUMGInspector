# MCP Tool Reference: Accessible UMG Inspector

## Overview
Accessible UMG Inspector exposes five operations through Epic's native Unreal Model Context Protocol (MCP) transport (`http://127.0.0.1:8000/mcp`).

Toolset Name:
```text
AccessibleUMGInspector.AccessibleUMGInspectorToolset
```

All operations are strictly read-only:
- Zero asset or package dirtying (`bZeroPackageMutation = true`).
- Zero synthetic input dispatch (`bZeroSyntheticInput = true`).
- Zero Slate widget allocation during inspection (`TakeWidget()` is never invoked).
- Zero background processes spawned.

---

## Operation 1: `GetInspectorCapabilities`

### Description
Returns plugin version metadata, supported Unreal Engine version, list of available inspection operations, format types, and explicit safety design principles.

### Inputs
None.

### Output
Returns `FInspectorCapabilities`:
- `PluginName` (`FString`): `"AccessibleUMGInspector"`
- `PluginVersion` (`FString`): `"0.1.0-rc.1"`
- `SupportedEngineVersion` (`FString`): `"5.8.2"`
- `SupportedOperations` (`TArray<FString>`): Array of 5 operation names.
- `SupportedFormats` (`TArray<FString>`): `["NvdaText", "Json", "Both"]`
- `bReadOnlyByDesign` (`bool`): `true`
- `bRequiresExplicitAssetPath` (`bool`): `true`
- `bZeroSyntheticInput` (`bool`): `true`
- `bZeroPackageMutation` (`bool`): `true`

### MCP Call Example
```json
{
  "toolset": "AccessibleUMGInspector.AccessibleUMGInspectorToolset",
  "tool": "GetInspectorCapabilities",
  "arguments": {}
}
```

---

## Operation 2: `InspectSerializedWidgetBlueprint`

### Description
Performs an authoritative, read-only inspection of an authored `UWidgetBlueprint` asset on disk/memory. Catalogs the widget hierarchy, slot properties, accessibility attributes, navigation rules, delegate bindings, and verifies that the package remains completely clean.

### Inputs
- **`AssetPath`** (`string`, **required**): Authoritative asset path to the Widget Blueprint (e.g. `"/Game/UI/WBP_MainMenu"`). Must not be empty.
- **`Format`** (`string`, optional): Output format: `"NvdaText"`, `"Json"`, or `"Both"` (default).

### Behavior
- **Asset Resolution**: Loads asset via `LoadObject<UWidgetBlueprint>`. If missing or invalid class, returns `bSuccess = false` and `Availability = Unavailable`.
- **Package Invariance**: Inspects `UPackage::IsDirty()` before and after traversal. If dirtiness changes, logs a package mutation error.
- **Ambiguity**: Detects widgets sharing duplicate variable or component names and reports them under `Warnings`.
- **Read-Only**: Does not modify properties, compile blueprints, or invoke transaction recording.

### MCP Call Example
```json
{
  "toolset": "AccessibleUMGInspector.AccessibleUMGInspectorToolset",
  "tool": "InspectSerializedWidgetBlueprint",
  "arguments": {
    "AssetPath": "/Game/UI/WBP_MainMenu",
    "Format": "Both"
  }
}
```

---

## Operation 3: `InspectRuntimeUserWidget`

### Description
Performs a read-only inspection of live `UUserWidget` instances in active PIE or Game viewports. Captures active visibility, passive Slate geometry (`GetTickSpaceGeometry()`), passive focus states (`FSlateApplication`), hover states, and accessibility properties without spawning Slate widgets.

### Inputs
- **`WidgetReference`** (`string`, optional): Target widget name, object path, or class. If empty, resolves single top-level user widget.
- **`WorldContext`** (`string`, optional): Target world name or path. If empty, resolves active PIE/Game world.
- **`MaxDepth`** (`int32`, optional): Maximum traversal depth (clamped to `[1, 100]`, default `32`).
- **`Format`** (`string`, optional): Output format: `"NvdaText"`, `"Json"`, or `"Both"` (default).

### Behavior
- **World Resolution**: Scans `GEngine->GetWorldContexts()` for PIE or Game worlds. If multiple worlds exist and `WorldContext` is empty, returns `Availability = Ambiguous` with candidate list.
- **Widget Resolution**: Finds matching `UUserWidget`. If multiple candidates match and `WidgetReference` is ambiguous, returns `Availability = Ambiguous` with candidates.
- **Two-Tier Visibility**: Automatically computes `ComputeEffectiveVisibility`, determining `bEffectivelyVisible`, `bEffectivelyInteractive`, `bEffectivelyFocusable`, and `BlockingAncestorIndex`.
- **Passive Focus**: Queries `FSlateApplication::Get().GetKeyboardFocusedWidget()` passively. Never calls `SetFocus` or simulates key events.
- **Passive Geometry**: Reads `SWidget::GetTickSpaceGeometry()` from cached Slate widgets via `GetCachedWidget()`. Never calls `TakeWidget()`.

### MCP Call Example
```json
{
  "toolset": "AccessibleUMGInspector.AccessibleUMGInspectorToolset",
  "tool": "InspectRuntimeUserWidget",
  "arguments": {
    "WidgetReference": "",
    "WorldContext": "",
    "MaxDepth": 32,
    "Format": "Both"
  }
}
```

---

## Operation 4: `CompareSerializedAndRuntime`

### Description
Reconciles an authored `UWidgetBlueprint` asset against a live runtime `UUserWidget` hierarchy. Classifies every node (`Matched`, `Ambiguous`, `SerializedOnly`, `RuntimeOnly`, `Unavailable`, `Unsupported`), records structural/visibility/state/accessibility/geometry discrepancies, and surfaces non-mutating evidence chains.

### Inputs
- **`AssetPath`** (`string`, **required**): Authoritative asset path to Widget Blueprint. Must not be empty.
- **`RuntimeWidgetReference`** (`string`, optional): Target runtime widget name, path, or class.
- **`WorldContext`** (`string`, optional): Target world name or path.
- **`MaxDepth`** (`int32`, optional): Traversal depth (clamped to `[1, 100]`, default `32`).
- **`Format`** (`string`, optional): Output format: `"NvdaText"`, `"Json"`, or `"Both"` (default).

### Behavior
- **Matching Priority**: Exact name and class compatibility -> parent container disambiguation -> multi-candidate escalation to `Ambiguous`.
- **Ancestor Suppression Awareness**: Suppresses false visibility and geometry discrepancies when a child control retains local `Visible` under a collapsed or hidden ancestor container.
- **Deterministic Ordering**: Sorts output for screen-reader triage: `Ambiguous` first, followed by `Matched`, `SerializedOnly`, and `RuntimeOnly`.

### MCP Call Example
```json
{
  "toolset": "AccessibleUMGInspector.AccessibleUMGInspectorToolset",
  "tool": "CompareSerializedAndRuntime",
  "arguments": {
    "AssetPath": "/Game/UI/WBP_MainMenu",
    "RuntimeWidgetReference": "",
    "WorldContext": "",
    "MaxDepth": 32,
    "Format": "Both"
  }
}
```

---

## Operation 5: `AuditWidgetAccessibility`

### Description
Evaluates automated accessibility rules (`A11Y-01` through `A11Y-14`) across serialized, runtime, and differential evidence. Classifies findings (`Error`, `Warning`, `Info`, `NotEvaluated`), grounds assertions in explicit confidence bases (`Direct`, `Derived`, `Ambiguous`, `Unavailable`), provides remediation guidance and limitations, and generates screen-reader-optimized NVDA text and structured JSON.

### Inputs
- **`AssetPath`** (`string`, optional): Widget Blueprint asset path.
- **`RuntimeWidgetReference`** (`string`, optional): Target live `UUserWidget` reference.
- **`WorldContext`** (`string`, optional): Target world or PIE context.
- **`RuleSet`** (`array[string]`, optional): List of rule IDs to evaluate (e.g. `["A11Y-02", "A11Y-06"]`). Empty evaluates all implemented rules.
- **`MaxDepth`** (`int32`, optional): Traversal depth (clamped to `[1, 100]`, default `32`).
- **`Format`** (`string`, optional): Output format: `"NvdaText"`, `"Json"`, or `"Both"` (default).

### Behavior
- **Target Requirement**: At least one of `AssetPath` or `RuntimeWidgetReference` must be supplied.
- **Evidence Mode**: Automatically selects `Serialized` (only asset provided), `Runtime` (only runtime provided), or `Differential` (both provided).
- **Two-Tier Visibility**: Inactive/collapsed ancestor screens faithfully suppress descendant evaluation. Does not report false positives on hidden screen layers.
- **Non-Evaluating Rules**: If evidence is missing for a rule (e.g. evaluating `A11Y-07` without a live runtime report), returns `NotEvaluated` with explicit missing evidence details. Never generates false passes or false errors.

### MCP Call Example
```json
{
  "toolset": "AccessibleUMGInspector.AccessibleUMGInspectorToolset",
  "tool": "AuditWidgetAccessibility",
  "arguments": {
    "AssetPath": "/Game/UI/WBP_MainMenu",
    "RuntimeWidgetReference": "",
    "WorldContext": "",
    "RuleSet": [],
    "MaxDepth": 32,
    "Format": "Both"
  }
}
```

---

## Architectural Limitations & Non-Claims
Across all 5 operations:
1. **No Hit-Test Grid Claim**: Passive bounds checking via `GetTickSpaceGeometry()` does not inspect Unreal's internal hit-test grid or prove pointer routing.
2. **No OS Accessibility Tree Claim**: Authoritative UMG accessibility properties do not guarantee OS UI Automation (UIA) or IAccessible2 (IA2) tree inclusion.
3. **No Direct Screen Reader Speech Verification**: NVDA text output is formatted for screen-reader readability; it does not directly drive the NVDA speech engine.
4. **No Crash Causality Claim**: State divergences and audit findings describe observed structure; they do not diagnose engine-level crashes.
5. **No Automatic Remediation**: All operations are read-only. Fixing defects requires developer authoring in Blueprint or C++.
