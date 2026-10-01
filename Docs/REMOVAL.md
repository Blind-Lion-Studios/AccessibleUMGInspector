# Removing Accessible UMG Inspector

## Overview
Accessible UMG Inspector is completely self-contained within its plugin directory. It creates no persistent files outside its folder, makes no project asset modifications, and injects no persistent runtime hooks.

---

## Disabling the Plugin (Temporary)
To temporarily disable Accessible UMG Inspector while keeping source files intact:

1. Close Unreal Editor.
2. Open your project descriptor (`YourProject.uproject`) in a text editor.
3. Locate the `AccessibleUMGInspector` entry under `Plugins` and change `"Enabled"` to `false`:
   ```json
   {
     "Name": "AccessibleUMGInspector",
     "Enabled": false
   }
   ```
4. Restart Unreal Editor.
5. In your MCP client, call `list_toolsets` to confirm `AccessibleUMGInspector` is no longer registered.

---

## Complete Uninstallation (Permanent)
To completely remove Accessible UMG Inspector from your project:

1. Close Unreal Editor and confirm all Editor processes are terminated.
2. Remove the `AccessibleUMGInspector` entry from the `Plugins` array in `YourProject.uproject`.
3. Delete the plugin directory:
   ```powershell
   Remove-Item -Recurse -Force "C:\YourProject\Plugins\AccessibleUMGInspector"
   ```
4. Delete the project intermediate build folders to clean up cached metadata:
   ```powershell
   Remove-Item -Recurse -Force "C:\YourProject\Intermediate"
   Remove-Item -Recurse -Force "C:\YourProject\Binaries"
   ```
5. Regenerate project solution files and rebuild your project.

---

## Verification of Clean Removal
1. Launch Unreal Editor.
2. Query the MCP endpoint:
   - Verify `AccessibleUMGInspector` does not appear in `list_toolsets`.
3. Verify Git status:
   - Confirm no untracked files or orphan binaries remain in `Plugins/`.
