#pragma once

#include <string>

namespace Globals {

    inline char WORKING_DIR[1024]{};

    // DLL source: 0 = local file on PC, 1 = GitHub latest release
    enum DllSource { SOURCE_LOCAL = 0, SOURCE_GITHUB = 1 };
    inline int DLL_SOURCE = Globals::SOURCE_GITHUB;

    // Path to a user-selected local .dll / .exe (used when DLL_SOURCE == SOURCE_LOCAL)
    inline std::wstring LOCAL_DLL_PATH{};

    // Launch path for the game. Supports the minecraft:// protocol or a path to .appx / .exe
    inline std::wstring MC_PATH{ L"minecraft://" };

    // Constants
    inline const std::wstring MCBE_PROC_NAME{ L"Minecraft.Windows.exe" };
    inline const std::wstring NO_DLL_PATH_SELECTED_MSG{ L"Click \"Browse\" to select a DLL file" };

    // GitHub release API for qcountel/controllin
    inline const std::wstring GITHUB_API_URL{ L"https://api.github.com/repos/qcountel/controllin/releases/latest" };

    // Hidden cache folder name (created in the working dir, FILE_ATTRIBUTE_HIDDEN)
    inline const std::wstring CACHE_DIR_NAME{ L".cache" };

    // UWP "ALL APPLICATION PACKAGES" well-known SID
    inline const std::wstring ALL_APP_PACKAGES_SID{ L"S-1-15-2-1" };

} //namespace Globals

class Config {
    std::string path;
    //Config name and state
    std::wstring currentKey;
    std::wstring currentVal;

    int analyzeInt();
    bool analyzeBool();
    void analyzeState();
public:
    Config();

    bool serializeConfig();
    bool updateConfigFile() const;
};
