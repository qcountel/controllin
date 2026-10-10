#pragma once

#include <string>

namespace Globals {

    inline char WORKING_DIR[1024]{};

    // Launch path for the game (minecraft:// protocol).
    inline const std::wstring MC_PATH{ L"minecraft://" };

    // Constants
    inline const std::wstring MCBE_PROC_NAME{ L"Minecraft.Windows.exe" };

    // GitHub release list for qcountel/controllin, newest first. The DLL for a game version is taken
    // from the newest release whose title carries that version's tag, e.g. "v2.0.5 1.16" or "v2.1 26.52".
    inline const std::wstring GITHUB_API_URL{ L"https://api.github.com/repos/qcountel/controllin/releases?per_page=100" };

    // Game versions the injector offers. `tag` is the word looked for in the release title.
    struct GameVersion {
        const wchar_t* name;   // shown in the UI
        const char*    tag;    // release title tag
    };
    inline const GameVersion VERSIONS[] = {
        { L"1.16.100", "1.16" },
        { L"26.52",    "26.52" },
    };
    inline constexpr int VERSION_COUNT = (int)(sizeof(VERSIONS) / sizeof(VERSIONS[0]));
    inline int SELECTED_VERSION = 0;   // index into VERSIONS, saved in HKCU\Software\Controllin

    // Controllin + subscription
    inline const wchar_t* PLUS_PRICE    = L"170р";
    inline const wchar_t* TELEGRAM_USER = L"@anx1ous";
    inline const wchar_t* TELEGRAM_URL  = L"https://t.me/anx1ous";

    // Hidden cache folder name (created in the working dir, FILE_ATTRIBUTE_HIDDEN)
    inline const std::wstring CACHE_DIR_NAME{ L".cache" };

    // UWP "ALL APPLICATION PACKAGES" well-known SID
    inline const std::wstring ALL_APP_PACKAGES_SID{ L"S-1-15-2-1" };

} //namespace Globals
