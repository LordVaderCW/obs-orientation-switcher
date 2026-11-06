# OBS Orientation Switcher

**A lightweight, native C++/Qt plugin for OBS Studio that enables one-click switching between landscape and portrait video resolutions.**

Developed by **[LordVaderCW](https://github.com/LordVaderCW)**.

OBS Orientation Switcher integrates directly into the OBS Studio interface, providing a dockable control panel and dedicated Tools menu commands for switching video orientation without manually editing resolution settings.

Designed for content creators who work with both traditional widescreen video and vertical content, including streaming, recording, YouTube Shorts, TikTok and other portrait-format platforms.

**Version:** 1.0.2  
**Platform:** Windows x64  
**Target OBS Version:** 32.2.1  
**Language:** C++17  
**Interface Framework:** Qt 6  
**Status:** Tested and operational

---

## Compatibility

**OBS Orientation Switcher v1.0.2 has been successfully built and tested with OBS Studio 32.2.1 on Windows x64.**

All implemented features have been confirmed operational in the tested environment.

| Component | Compatibility |
|---|---|
| OBS Studio 32.2.1 | Tested and working |
| Windows x64 | Tested and working |
| Native OBS frontend integration | Working |
| Qt-based dockable interface | Working |
| Landscape/portrait switching | Working |
| OBS Tools menu integration | Working |
| 500 ms repeat-click protection | Working |
| Active output protection | Working |
| Developer footer and About dialog | Working |

Compatibility with other OBS Studio versions and operating systems has not yet been verified.

---

## Features

### One-Click Orientation Switching

Switch between two predefined video configurations without opening OBS Studio's Video settings.

| Orientation | Base Canvas | Output Resolution |
|---|---|---|
| Landscape | 1920 × 1080 | 1920 × 1080 |
| Portrait | 1080 × 1920 | 1080 × 1920 |

Both the base canvas and scaled output resolution are changed together.

The plugin updates the active OBS profile and requests a video subsystem reset to apply the new configuration.

### Dockable Control Panel

OBS Orientation Switcher provides a native Qt-based dock that integrates directly into the OBS Studio workspace.

The dock includes:

- A single button for toggling between landscape and portrait.
- Automatic detection of the current orientation.
- A status label displaying the active resolution.
- Detection of custom resolution configurations.
- Automatic interface updates.
- A developer attribution footer.
- An embedded circular developer avatar.
- An About dialog containing plugin and developer information.

The dock can be repositioned alongside other OBS panels.

**Example dock states:**

```text
+--------------------------------------+
|                                      |
|         Switch to Portrait           |
|                                      |
|       Landscape | 1920 x 1080        |
|                                      |
|  [Avatar] LordVaderCW         About  |
+--------------------------------------+
```

```text
+--------------------------------------+
|                                      |
|         Switch to Landscape          |
|                                      |
|        Portrait | 1080 x 1920        |
|                                      |
|  [Avatar] LordVaderCW         About  |
+--------------------------------------+
```

The examples are illustrative representations of the dock interface.

### OBS Tools Menu Integration

The plugin registers two additional commands under the OBS Studio Tools menu:

```text
Tools
 ├── Orientation: Landscape (1920 x 1080)
 └── Orientation: Portrait (1080 x 1920)
```

These commands use the same resolution-switching logic and safety checks as the dock.

### 500 ms Silent Click Protection

Version 1.0.2 includes a lightweight repeat-click protection mechanism.

When a valid orientation change is accepted, a monotonic timer starts a 500-millisecond guard interval.

Additional orientation requests are ignored while a switch is executing or during the remaining guard interval.

The button remains visually enabled during this period.

This provides:

- Protection against accidental double-clicks.
- Reduced frequency of consecutive OBS video-reset requests.
- No unnecessary button disabling or visual interruption.
- Consistent behaviour between the dock and Tools menu.

The guard does not queue additional requests. Rejected clicks are simply discarded.

**Technical note:** The 500 ms guard is an input-protection mechanism, not a guarantee that all internal OBS video and audio processing has completed.

### Active Output Protection

To prevent disruption to active OBS sessions, orientation switching is prohibited while the OBS frontend reports any of the following as active:

- Streaming
- Recording
- Replay buffer
- Virtual camera

The dock's orientation button is disabled while these outputs are active.

Tools menu requests are also rejected.

Third-party outputs managed independently of these frontend states may not be detected.

### Automatic Orientation Detection

The plugin reads the current base canvas and output resolution from the active OBS profile.

It identifies three states:

**Landscape**

```text
Base Canvas:       1920 x 1080
Output Resolution: 1920 x 1080
```

**Portrait**

```text
Base Canvas:       1080 x 1920
Output Resolution: 1080 x 1920
```

**Custom**

Any configuration that does not exactly match either predefined resolution combination.

When a custom configuration is detected, the dock offers a switch to the predefined landscape configuration.

### Embedded Developer Attribution

The plugin includes a developer footer displaying:

- LordVaderCW
- A circular 16 × 16 developer avatar
- An About button

The avatar is embedded directly into the plugin DLL as a Windows resource.

No separate avatar file is required at runtime.

### Native Implementation

OBS Orientation Switcher is implemented using C++17 and Qt 6.

It uses the OBS frontend API for profile access, video resets, output-state checks and interface integration.

The plugin does not depend on browser automation, external web services or scripting engines.

---

## System Requirements

### Runtime Requirements

- Windows 64-bit
- OBS Studio 32.2.1
- Required OBS and Qt runtime dependencies provided by a compatible OBS installation

### Development Requirements

- Visual Studio 2022
- Desktop development with C++ workload
- Windows SDK
- C++17 support
- Qt 6 development headers and import libraries
- OBS Studio 32.2.1 development headers and x64 import libraries
- CMake 3.28+ for the CMake build workflow

OBS Studio's installed runtime files alone may not provide all the development headers and libraries necessary to compile the plugin.

---

## Installation

### Installing a Precompiled Release

If a precompiled DLL is available from the repository's Releases section:

1. Download the Windows x64 release.
2. Close OBS Studio completely.
3. Extract the release archive if necessary.
4. Locate `obs-orientation-switcher.dll`.
5. Copy the DLL into the OBS Studio 64-bit plugin directory.

For a standard OBS Studio installation:

```text
C:\Program Files\obs-studio\obs-plugins\64bit\
```

6. Restart OBS Studio.

The plugin will register its Tools menu commands and dock during startup.

**Note:** Precompiled releases are only available when published by the developer. The repository source code can also be compiled manually.

### Accessing the Plugin

Once installed, the following Tools menu commands become available:

```text
Orientation: Landscape (1920 x 1080)
Orientation: Portrait (1080 x 1920)
```

The Orientation Switcher dock should also appear in the OBS interface.

If hidden, check the OBS Docks menu.

---

## Building from Source

The project supports development using Visual Studio 2022 and, where the required presets and dependency packages are available, CMake.

### Method 1: Visual Studio 2022

This is the primary Windows development workflow.

**Step 1 — Obtain the source**

Clone the repository:

```powershell
git clone https://github.com/LordVaderCW/obs-orientation-switcher.git
cd obs-orientation-switcher
```

**Step 2 — Configure dependencies**

Obtain development headers and import libraries compatible with OBS Studio 32.2.1.

Open:

```text
OBSPaths.props
```

Update the relevant paths to point to your OBS and Qt development directories.

The OBS include directories must provide:

```text
obs-module.h
obs-frontend-api.h
util/config-file.h
```

The OBS library directories must provide:

```text
obs.lib
obs-frontend-api.lib
```

Ensure the selected Qt development libraries are compatible with the OBS build.

Do not mix incompatible architectures, runtime configurations or OBS development versions.

**Step 3 — Open the solution**

Open:

```text
obs-orientation-switcher.sln
```

In Visual Studio 2022.

**Step 4 — Select the build configuration**

```text
Configuration: Release
Platform:      x64
```

**Step 5 — Build**

Select:

```text
Build > Build Solution
```

The expected output is:

```text
build/x64/Release/obs-orientation-switcher.dll
```

**Step 6 — Install**

Copy the resulting DLL into the OBS Studio 64-bit plugin directory and restart OBS.

### Method 2: CMake

For environments with the repository's CMake presets and matching OBS development package exports, open a Visual Studio 2022 Developer PowerShell.

Configure:

```powershell
cmake --preset windows-x64 -DCMAKE_PREFIX_PATH="C:/path/to/obs-32.2.1-development-prefix"
```

Build:

```powershell
cmake --build --preset windows-x64-release
```

The expected output is:

```text
build/windows-x64/Release/obs-orientation-switcher.dll
```

If CMake cannot locate OBS development packages, check the configured prefix for the appropriate `libobs` and `obs-frontend-api` CMake package configuration files.

The precise package names and locations depend on the OBS development build being used.

---

## Usage

### Switching from Landscape to Portrait

1. Open OBS Studio.
2. Ensure streaming, recording, replay buffer and virtual camera are stopped.
3. Locate the Orientation Switcher dock.
4. Click **Switch to Portrait**.

The plugin changes the active profile to:

```text
Base Canvas:       1080 x 1920
Output Resolution: 1080 x 1920
```

### Switching from Portrait to Landscape

Click **Switch to Landscape**.

The plugin changes the active profile to:

```text
Base Canvas:       1920 x 1080
Output Resolution: 1920 x 1080
```

### Switching Using the Tools Menu

Alternatively, select the required orientation directly from the OBS Tools menu.

The same safety and repeat-click checks apply.

---

## Technical Overview

OBS Orientation Switcher operates through the OBS frontend API.

The plugin reads and modifies the active profile's video configuration values:

```ini
[Video]
BaseCX=1920
BaseCY=1080
OutputCX=1920
OutputCY=1080
```

For portrait orientation:

```ini
[Video]
BaseCX=1080
BaseCY=1920
OutputCX=1080
OutputCY=1920
```

### Orientation Switching Sequence

When a switch is requested, the plugin:

1. Checks whether any monitored OBS output is active.
2. Rejects requests while an orientation switch is already executing.
3. Rejects requests within the 500 ms repeat-click interval.
4. Reads the active profile's video resolution.
5. Skips the reset if the requested resolution is already configured.
6. Starts the switching guard and monotonic timer.
7. Updates the four video resolution configuration values.
8. Calls `obs_frontend_reset_video()` to apply the new configuration.
9. Calls `obs_frontend_save()` to persist the profile settings.
10. Releases the execution guard and updates the dock.

### OBS Frontend APIs

The plugin uses the following OBS frontend functions:

```cpp
obs_frontend_get_profile_config();

obs_frontend_streaming_active();
obs_frontend_recording_active();
obs_frontend_replay_buffer_active();
obs_frontend_virtualcam_active();

obs_frontend_reset_video();
obs_frontend_save();

obs_frontend_add_tools_menu_item();
obs_frontend_add_dock_by_id();
obs_frontend_remove_dock();
```

The interface is implemented using Qt Widgets, with `QElapsedTimer` providing monotonic timing for repeat-click protection.

---

## Project Structure

The project uses the following principal files and directories:

```text
obs-orientation-switcher/
|
|-- src/
|   `-- plugin-main.cpp
|
|-- resources/
|   |-- resource.h
|   `-- plugin.rc
|
|-- assets/
|   `-- lordvadercw.png
|
|-- OBSPaths.props
|-- obs-orientation-switcher.sln
|-- CMakeLists.txt
|-- CMakePresets.json
|-- README.md
|
`-- build/
    `-- x64/
        `-- Release/
            `-- obs-orientation-switcher.dll
```

Build output directories and local development dependencies should not normally be committed to source control.

The precise repository contents may vary depending on the build workflow and release packaging.

---

## Known Limitations

### Scene Layouts Are Not Automatically Converted

The plugin changes OBS video resolutions but does not reposition, resize or crop individual scene sources.

Scenes designed for landscape may require adjustments when displayed on a portrait canvas.

For best results, prepare suitable scene layouts for both orientations.

### Active Profile Modification

Orientation switching changes the active OBS profile and saves the resulting settings.

Back up important OBS profiles before making substantial changes.

### Fixed Resolution Presets

Version 1.0.2 uses two predefined resolution configurations:

- 1920 × 1080
- 1080 × 1920

Custom user-configurable presets are not yet implemented.

### Video Reset Completion

`obs_frontend_reset_video()` does not provide a success return value.

Although the plugin has been tested successfully on OBS Studio 32.2.1, users should verify the resulting resolution in **Settings → Video** if an unexpected configuration is observed.

### Other OBS Versions

Compatibility has been confirmed for OBS Studio 32.2.1 on Windows x64.

Earlier or later OBS versions have not yet been verified.

---

## Troubleshooting

### Plugin Does Not Load

Check that:

- OBS Studio is the 64-bit Windows version.
- The plugin DLL was built for x64.
- The DLL is located in the correct OBS plugin directory.
- Required runtime dependencies are available.
- OBS Studio has been restarted after installation.

Inspect the OBS log for messages containing:

```text
[Orientation Switcher]
```

### Orientation Button Does Not Respond

Ensure that OBS is not currently streaming, recording, running the replay buffer or using the virtual camera.

Repeated clicks within 500 ms are intentionally ignored.

### Resolution Does Not Appear to Change

Check the current configuration under:

**OBS Studio → Settings → Video**

If necessary, restart OBS Studio and verify that the active profile contains the expected values.

### Scene Sources Appear Incorrectly Positioned

The plugin does not automatically convert scene layouts.

Adjust source positions and scaling for the selected orientation.

---

## Development Roadmap

Potential future enhancements include:

- User-configurable orientation resolutions.
- Additional resolution presets.
- Keyboard shortcut integration.
- Improved coordination with OBS video lifecycle events.
- Optional scene switching when changing orientation.
- Orientation-specific scene management.
- Additional platform support.
- Broader OBS Studio version compatibility.
- Expanded diagnostics and logging.

These features are under consideration and are not part of version 1.0.2.

---

## Version History

### v1.0.2 — 500 ms Silent Click Protection

- Introduced a 500 ms monotonic repeat-click guard.
- Changed cooldown behaviour so the orientation button remains visually enabled.
- Repeated requests within the guard interval are silently ignored.
- Retained protection against switching while monitored OBS outputs are active.
- Preserved dock, Tools menu and developer attribution functionality.
- Successfully tested with OBS Studio 32.2.1 on Windows x64.

### v1.0.1 — Orientation Switching Protection

- Introduced protection against overlapping orientation requests.
- Added a temporary switching cooldown.
- Improved handling of repeated switching operations.

### v1.0.0 — Initial Implementation

- Added landscape and portrait resolution switching.
- Added OBS Tools menu commands.
- Added a dockable Qt control panel.
- Added automatic orientation detection.
- Added active output protection.
- Added embedded developer avatar and About dialog.
- Added OBS profile persistence.

---

## References

- [OBS Studio — Official GitHub Repository](https://github.com/obsproject/obs-studio)
- [OBS Studio Plugin Template](https://github.com/obsproject/obs-plugintemplate)
- [OBS Studio Developer Documentation](https://docs.obsproject.com/)
- [Qt Documentation](https://doc.qt.io/)

---

## Developer

**LordVaderCW**

Software development, reverse engineering and native application tooling.

**GitHub:** https://github.com/LordVaderCW

Contributions, bug reports and feature suggestions are welcome through the repository's GitHub Issues section.

---

## Licence

See the repository's `LICENSE` file for licensing terms, if provided.

No licence is declared by this README alone.

---

## Disclaimer

OBS Orientation Switcher is an independent third-party plugin developed by LordVaderCW.

It is not an official OBS Project product and is not affiliated with or endorsed by the OBS Project.

OBS Studio and its associated trademarks remain the property of their respective owners.