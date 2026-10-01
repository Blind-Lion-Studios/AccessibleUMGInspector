# Installing Accessible UMG Inspector

## Overview
Accessible UMG Inspector is an Editor-only plugin built for Unreal Engine 5.8.2. It integrates with Epic's native Unreal Model Context Protocol (MCP) and `ToolsetRegistry` to expose accessibility-first inspection operations.

---

## System Requirements
- **Unreal Engine**: Version 5.8.2.
- **Operating System**: Windows 10/11 (64-bit).
- **Target Platform**: Win64 Editor (`TargetAllowList: ["Editor"]`).
- **Required Epic Plugins**:
  - `ModelContextProtocol` (Engine plugin)
  - `ToolsetRegistry` (Engine plugin)

---

## Step 1: Copy Plugin into Project
1. Navigate to your project root folder (e.g. `C:\YourProject\`).
2. If a `Plugins` directory does not exist, create it:
   ```text
   C:\YourProject\Plugins\
   ```
3. Copy or clone `AccessibleUMGInspector` into the `Plugins` directory:
   ```text
   C:\YourProject\Plugins\AccessibleUMGInspector\
   ```
4. Verify the folder structure contains:
   ```text
   C:\YourProject\Plugins\AccessibleUMGInspector\AccessibleUMGInspector.uplugin
   C:\YourProject\Plugins\AccessibleUMGInspector\Source\
   ```

---

## Step 2: Enable Prerequisites in Project Descriptor
Ensure your `.uproject` file enables the required plugins:
```json
{
  "Plugins": [
    {
      "Name": "ModelContextProtocol",
      "Enabled": true
    },
    {
      "Name": "ToolsetRegistry",
      "Enabled": true
    },
    {
      "Name": "AccessibleUMGInspector",
      "Enabled": true
    }
  ]
}
```

---

## Step 3: Build Plugin Binaries
Run Unreal Engine's AutomationTool to compile the plugin:
```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin `
    -Plugin="C:\YourProject\Plugins\AccessibleUMGInspector\AccessibleUMGInspector.uplugin" `
    -Package="C:\YourProject\Plugins\AccessibleUMGInspector\Saved\BuildPlugin" `
    -TargetPlatforms=Win64
```
Copy the compiled binaries from `Saved\BuildPlugin\Binaries` into `Plugins\AccessibleUMGInspector\Binaries`.

---

## Step 4: Launch Unreal Editor with MCP Enabled
Launch Unreal Editor with the local MCP server enabled on port 8000:
```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" `
    "C:\YourProject\YourProject.uproject" `
    -EnableMcp
```

---

## Step 5: Verify Toolset Discovery
In your MCP client or AI agent environment, call `unreal-mcp.list_toolsets`.
Verify that `AccessibleUMGInspector.AccessibleUMGInspectorToolset` appears in the list with all 5 operations:
- `GetInspectorCapabilities`
- `InspectSerializedWidgetBlueprint`
- `InspectRuntimeUserWidget`
- `CompareSerializedAndRuntime`
- `AuditWidgetAccessibility`

---

## Troubleshooting Installation Issues

### Issue 1: Toolset Not Discovered
- **Symptom**: `list_toolsets` does not include `AccessibleUMGInspector`.
- **Cause**: Plugin module was not loaded during Editor startup.
- **Resolution**: Confirm `AccessibleUMGInspector.uplugin` specifies `"LoadingPhase": "PostEngineInit"`, and verify that the plugin is set to `"Enabled": true` in the project descriptor. Check the Editor output log for `FAccessibleUMGInspectorModule`.

### Issue 2: Plugin Not Enabled
- **Symptom**: Compilation warnings or missing class references.
- **Cause**: Project descriptor does not list `AccessibleUMGInspector` in `"Plugins"`.
- **Resolution**: Open `YourProject.uproject` in a text editor and add `"AccessibleUMGInspector"` with `"Enabled": true`.

### Issue 3: Unsupported Engine Version
- **Symptom**: Compilation errors in UMG or Slate headers.
- **Cause**: Engine version is earlier than Unreal Engine 5.8.
- **Resolution**: Accessible UMG Inspector requires Unreal Engine 5.8.2. Upgrading earlier engine versions is required.

### Issue 4: MCP Endpoint Unavailable
- **Symptom**: MCP connection to `127.0.0.1:8000` times out.
- **Cause**: Editor was launched without `-EnableMcp` or another process is occupying port 8000.
- **Resolution**: Check port availability with `Get-NetTCPConnection -LocalPort 8000`. Ensure Editor is started with `-EnableMcp`.
