# Controllin Injector

A minimal launcher-injector for Minecraft Bedrock Edition (UWP).

## UI
- **Gear button (top-left)** – opens **Settings**.
- **Big PLAY button (center)** – launches the game (if needed), downloads/loads the DLL, and injects it.
- Dark, minimalist theme with the app logo and a one-line status under PLAY.

## Settings
- **DLL source**
  - *Local DLL* – pick a `.dll` / `.exe` from your PC.
  - *GitHub* – auto-downloads the latest release from
    `https://github.com/qcountel/controllin/releases/`.
- **Minecraft path** – `minecraft://` protocol (default) or a path to the game `.appx` / `.exe`.
- **Save** – writes settings to `config.txt`.

## PLAY logic (runs on a background std::thread)
1. Check whether `Minecraft.Windows.exe` is running (`GetProcId`).
2. If not running, launch it via `ShellExecuteW` and wait up to ~20s for the process.
3. Get the DLL: download the latest GitHub release (`.dll` asset) via the GitHub API, or use the local file.
4. GitHub downloads are saved to a hidden `.cache` folder (`FILE_ATTRIBUTE_HIDDEN`).
5. Grant the UWP `ALL APPLICATION PACKAGES` SID (`S-1-15-2-1`) to the DLL and inject with `CreateRemoteThread` + `LoadLibraryW`.

## Build
Requires CMake 3.20+, a C++20 MSVC toolchain, and the bundled wxWidgets 3.2.0.

```
build.bat
```
or manually:
```
cmake -S . -B build -A x64
cmake --build build --config Release
```

Output: `build\Release\Controllin Injector.exe`. The app requires administrator rights (set in the
manifest) and links `wininet` (downloads) and `dwmapi` (dark title bar).

## Troubleshooting
VCRUNTIME140_1.dll not found: install the x64 Visual C++ redistributable from
https://support.microsoft.com/en-us/help/2977003

This project uses [wxWidgets](https://wxwidgets.org).
