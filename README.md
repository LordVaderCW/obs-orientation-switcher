# OBS Orientation Switcher — OBS Studio 32.2.1

Native Windows x64 C++ plugin targeting the **OBS Studio 32.2.1** frontend API. Adds two entries to the Tools menu to switch both base canvas and scaled output resolution:

- Landscape: 1920 × 1080
- Portrait: 1080 × 1920

**Source package, not a precompiled binary. Compatibility with OBS 32.2.1 is targeted, not yet verified by a build/runtime test.**

## Prerequisites

- OBS Studio **32.2.1**, Windows x64
- Visual Studio 2022 with the Desktop development with C++ workload
- CMake 3.28+
- OBS **32.2.1** development/build artifacts containing `libobs` and `obs-frontend-api` CMake package configurations and libraries (matching architecture and runtime)

OBS installed binaries alone may not provide the development CMake package exports. Build the matching OBS 32.2.1 source tag, or use the official OBS plugin template's dependency workflow, pinned to matching OBS 32.2.1 artifacts. Do not mix OBS 31.x headers/import libraries with 32.2.1.

## Build

Open a Visual Studio 2022 Developer PowerShell in the extracted project directory:

```powershell
cmake --preset windows-x64 -DCMAKE_PREFIX_PATH="C:/path/to/obs-32.2.1-development-prefix"
cmake --build --preset windows-x64-release
```

If CMake cannot locate packages, check that the prefix contains `libobsConfig.cmake` and `obs-frontend-apiConfig.cmake` (or their corresponding exported package files). The package naming/layout must match your OBS SDK/build.

## Install

Close OBS. Copy `build/windows-x64/Release/obs-orientation-switcher.dll` to the OBS installation's `obs-plugins/64bit` directory (typically `C:\Program Files\obs-studio\obs-plugins\64bit`). Restart OBS. In **Tools**, choose Landscape or Portrait.

## Limitations

- Stop streaming, recording, replay buffer and virtual camera before switching. Other plugin-managed active outputs may not be detectable.
- The plugin does not reposition scene items; sources may fall outside the new canvas.
- The plugin changes the **active OBS profile** and saves it. Back up your profile first.
- `obs_frontend_reset_video()` has no success return value; verify the resulting resolution in OBS Settings → Video.
- This project has not been compiled or run against OBS Studio 32.2.1 in this environment.

## Reference

Official OBS Studio repository: https://github.com/obsproject/obs-studio
Official plugin template: https://github.com/obsproject/obs-plugintemplate

## Visual Studio 2022 solution (direct MSBuild)

1. Obtain **OBS Studio 32.2.1 matching development headers and x64 import libraries** (not just the installed OBS runtime).
2. Open `OBSPaths.props` and replace the placeholder paths with the actual development include and library directories. The include directory must expose `obs-module.h`, `obs-frontend-api.h`, and `util/config-file.h`; the library directory must contain `obs.lib` and `obs-frontend-api.lib`. Adjust the directories if your SDK layout differs.
3. Open `obs-orientation-switcher.sln` in Visual Studio 2022 with the Desktop development with C++ workload installed.
4. Select **Release | x64** and **Build > Build Solution**.
5. The resulting DLL is `build/x64/Release/obs-orientation-switcher.dll`. Install it into the OBS 64-bit plugin folder and restart OBS.

The paths in `OBSPaths.props` are placeholders, not a bundled SDK. No OBS binaries are distributed with this source project. This project has not been compiled or runtime-tested here.
