#pragma once

#include <string>

namespace Globals {

    inline char WORKING_DIR[1024]{};

    // Launch path for the game (minecraft:// protocol).
    inline const std::wstring MC_PATH{ L"minecraft://" };

    // Constants
    inline const std::wstring MCBE_PROC_NAME{ L"Minecraft.Windows.exe" };

    // GitHub release API for qcountel/controllin (the DLL is always taken from the latest release)
    inline const std::wstring GITHUB_API_URL{ L"https://api.github.com/repos/qcountel/controllin/releases/latest" };

    // Hidden cache folder name (created in the working dir, FILE_ATTRIBUTE_HIDDEN)
    inline const std::wstring CACHE_DIR_NAME{ L".cache" };

    // UWP "ALL APPLICATION PACKAGES" well-known SID
    inline const std::wstring ALL_APP_PACKAGES_SID{ L"S-1-15-2-1" };

} //namespace Globals
