# Compatibility Matrix: Accessible UMG Inspector

## Supported Environments

| Dimension | Supported | Status / Notes |
| :--- | :--- | :--- |
| **Unreal Engine 5.8.2** | **Yes** | Primary target platform (`BuildId: 55116800`). 112/112 tests pass. |
| Unreal Engine 5.8.0 – 5.8.1 | Untested | Compatible API surface; recommended to verify with test suite. |
| Unreal Engine 5.7 and earlier | **No** | Unsupported. Breaking differences in Slate reflection and MCP. |
| Unreal Engine 5.9+ | Untested | Future compatibility to be evaluated upon engine release. |

---

## Platform Support

| Platform | Target Support | Status |
| :--- | :--- | :--- |
| **Win64 Editor** | **Yes** | Fully verified (`TargetAllowList: ["Editor"]`). |
| Linux Editor | Untested | Depends on Linux MCP support and toolchain. |
| Mac Editor | Untested | Depends on macOS MCP support and toolchain. |
| Standalone Game / Monolithic | **No** | Strictly forbidden. The plugin is Editor-only (`EditorOnly: true`). |

---

## Required Epic Plugins

| Plugin Name | Minimum Version | Purpose |
| :--- | :--- | :--- |
| **`ModelContextProtocol`** | UE 5.8 native | Local MCP JSON-RPC transport (`127.0.0.1:8000`). |
| **`ToolsetRegistry`** | UE 5.8 native | Dynamic discovery and registration of toolset classes. |

---

## Screen Reader Compatibility
Accessible UMG Inspector generates plain-text reports optimized for:
- **NVDA** (NonVisual Desktop Access) on Windows 10/11.
- **JAWS** (Job Access With Speech).
- **Windows Narrator**.
- Any standard Markdown reader or Braille display.

---

## Deprecation & Versioning Policy
Accessible UMG Inspector follows [Semantic Versioning 2.0.0](https://semver.org/):
- **Patch versions** (`0.1.x`): Bug fixes, rule clarifications, documentation updates. Zero breaking API changes.
- **Minor versions** (`0.x.0`): New inspection operations, new audit rules, backwards-compatible schema extensions.
- **Major versions** (`1.0.0+`): Breaking schema modifications or major engine version transitions.
