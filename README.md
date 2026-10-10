# Controllin Injector

A minimal launcher-injector for Minecraft Bedrock Edition (UWP).

## UI
- **PLAY tab (top-left)** – the main screen with the big **PLAY** button and a one-line status under it.
- **VERSION button** (above PLAY) – picks the game version: **1.16.100** or **26.52**. The choice is
  saved in `HKCU\Software\Controllin`.
- **CONTROLLIN + (top-right, gold)** – subscription: 170р forever, bought from the owner in Telegram
  ([@anx1ous](https://t.me/anx1ous)). The dialog shows the PC's **HWID** with a copy button — the HWID
  is needed to create the subscription key (SHA-256 of the Windows MachineGuid, see `src/hwid.cpp`).
- Dark, minimalist theme; the app / window / taskbar icon is the isometric grass block.
- The DLL is the newest release of `https://github.com/qcountel/controllin/releases/` whose **title**
  carries the selected version's tag as a separate word: `1.16` (e.g. `v2.0.5 1.16`) or `26.52`
  (e.g. `v2.1.0 26.52`). Releases without a tag are ignored. The game is launched via `minecraft://`.

## PLAY logic (runs on a background std::thread)
1. Check whether `Minecraft.Windows.exe` is running (`GetProcId`).
2. If not running, launch it via `ShellExecuteW` and wait up to ~20s for the process.
3. Find the newest release tagged with the selected version and download its `.dll` asset.
4. GitHub downloads are saved to a hidden `.cache\<tag>` folder (`FILE_ATTRIBUTE_HIDDEN`).
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
