#include "pch.h"
#include "launcher.h"
#include "config.h"
#include "inject.h"
#include "json.h"

#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <vector>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shell32.lib")

// ---------------------------------------------------------------------------
// Hidden cache folder
// ---------------------------------------------------------------------------
std::wstring EnsureCacheDir() {
    // WORKING_DIR is an ANSI buffer; build a wide path from it.
    std::string ansi(Globals::WORKING_DIR);
    std::wstring base(ansi.begin(), ansi.end());

    std::wstring cacheDir = base + L"\\" + Globals::CACHE_DIR_NAME;

    CreateDirectoryW(cacheDir.c_str(), nullptr); // ignores "already exists"
    SetFileAttributesW(cacheDir.c_str(), FILE_ATTRIBUTE_HIDDEN);

    return cacheDir;
}

// ---------------------------------------------------------------------------
// Small HTTP GET helper (WinINet). Returns raw bytes of the response body.
// ---------------------------------------------------------------------------
static bool HttpGet(const std::wstring& url, std::vector<char>& outData, std::wstring& outStatus) {
    outData.clear();

    HINTERNET hInternet = InternetOpenW(
        L"Controllin-Injector/1.0",
        INTERNET_OPEN_TYPE_PRECONFIG,
        nullptr, nullptr, 0);
    if (!hInternet) {
        outStatus = L"InternetOpen failed";
        return false;
    }

    // GitHub API requires a User-Agent (set above) and we follow redirects by default.
    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                  INTERNET_FLAG_SECURE | INTERNET_FLAG_KEEP_CONNECTION;

    HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(),
                                      L"Accept: application/vnd.github+json\r\n",
                                      (DWORD)-1L, flags, 0);
    if (!hUrl) {
        outStatus = L"InternetOpenUrl failed";
        InternetCloseHandle(hInternet);
        return false;
    }

    // Check HTTP status code
    DWORD statusCode = 0;
    DWORD len = sizeof(statusCode);
    HttpQueryInfoW(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                   &statusCode, &len, nullptr);
    if (statusCode >= 400) {
        outStatus = L"HTTP error " + std::to_wstring(statusCode);
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[8192];
    DWORD bytesRead = 0;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        outData.insert(outData.end(), buffer, buffer + bytesRead);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    outStatus = L"OK";
    return true;
}

static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

// True if `title` contains `tag` as a separate word: "v2.0.5 1.16" has "1.16", but "v2.1.16" does not.
// A longer version of the same line also matches ("1.16.100" for "1.16").
static bool TitleHasTag(const std::string& title, const std::string& tag) {
    size_t i = 0;
    while (i < title.size()) {
        while (i < title.size() && (title[i] == ' ' || title[i] == '\t' || title[i] == '[' || title[i] == '(')) ++i;
        size_t j = i;
        while (j < title.size() && title[j] != ' ' && title[j] != '\t' && title[j] != ']' && title[j] != ')') ++j;
        std::string word = title.substr(i, j - i);
        if (word == tag || (word.size() > tag.size() && word.compare(0, tag.size(), tag) == 0 && word[tag.size()] == '.'))
            return true;
        i = j + 1;
    }
    return false;
}

// Finds the newest published release with `tag` in its title and a .dll asset.
static bool FindReleaseDll(const std::string& json, const std::string& tag,
                           std::string& outUrl, std::string& outName, std::string& outTitle) {
    Json::JParser parser(json);
    Json::JValue root = parser.value();
    const Json::JArray* releases = root.arr();
    if (!releases) return false;
    for (const Json::JValue& rel : *releases) {   // GitHub lists releases newest first
        const Json::JValue* draft = rel.get("draft");
        if (draft && draft->boolean()) continue;
        const Json::JValue* name = rel.get("name");
        std::string title = name ? name->str() : std::string();
        if (!TitleHasTag(title, tag)) continue;
        const Json::JValue* assets = rel.get("assets");
        if (!assets || !assets->arr()) continue;
        for (const Json::JValue& asset : *assets->arr()) {
            const Json::JValue* aname = asset.get("name");
            const Json::JValue* url = asset.get("browser_download_url");
            if (!aname || !url) continue;
            std::string n = aname->str();
            if (n.size() >= 4 && _stricmp(n.c_str() + n.size() - 4, ".dll") == 0) {
                outUrl = url->str();
                outName = n;
                outTitle = title;
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Download the newest release .dll for a game version into the hidden cache
// ---------------------------------------------------------------------------
std::wstring DownloadLatestGithubDll(const char* tag, std::wstring& outRelease, std::wstring& outStatus) {
    std::vector<char> apiResponse;
    if (!HttpGet(Globals::GITHUB_API_URL, apiResponse, outStatus)) {
        return std::wstring();
    }

    std::string assetUrl, assetName, title;
    try {
        std::string json(apiResponse.begin(), apiResponse.end());
        if (!FindReleaseDll(json, tag, assetUrl, assetName, title)) {
            outStatus = L"No release with a .dll for " + Utf8ToWide(tag);
            return std::wstring();
        }
    } catch (...) {
        outStatus = L"Bad response from GitHub";
        return std::wstring();
    }
    outRelease = Utf8ToWide(title);

    std::vector<char> dllData;
    if (!HttpGet(Utf8ToWide(assetUrl), dllData, outStatus) || dllData.empty()) {
        if (outStatus == L"OK") outStatus = L"Downloaded file is empty";
        return std::wstring();
    }

    // Each game version gets its own subfolder so DLLs for different versions never mix.
    std::wstring versionDir = EnsureCacheDir() + L"\\" + Utf8ToWide(tag);
    CreateDirectoryW(versionDir.c_str(), nullptr);
    std::wstring fileName = Utf8ToWide(assetName);
    if (fileName.find_first_of(L"\\/:") != std::wstring::npos) fileName = L"controllin.dll";
    std::wstring dllPath = versionDir + L"\\" + fileName;

    HANDLE hFile = CreateFileW(dllPath.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        outStatus = (GetLastError() == ERROR_SHARING_VIOLATION)
            ? L"DLL is in use - restart Minecraft"
            : L"Failed to write DLL to cache";
        return std::wstring();
    }

    DWORD written = 0;
    WriteFile(hFile, dllData.data(), (DWORD)dllData.size(), &written, nullptr);
    CloseHandle(hFile);

    if (written != dllData.size()) {
        outStatus = L"Incomplete write to cache";
        return std::wstring();
    }

    outStatus = L"Downloaded " + outRelease;
    return dllPath;
}

// ---------------------------------------------------------------------------
// Selected version (HKCU\Software\Controllin, value "Version" = version name)
// ---------------------------------------------------------------------------
static const wchar_t* REG_KEY = L"Software\\Controllin";

void LoadSelectedVersion() {
    wchar_t buf[64]{};
    DWORD size = sizeof(buf);
    if (RegGetValueW(HKEY_CURRENT_USER, REG_KEY, L"Version", RRF_RT_REG_SZ, nullptr, buf, &size) != ERROR_SUCCESS)
        return;
    for (int i = 0; i < Globals::VERSION_COUNT; ++i)
        if (wcscmp(buf, Globals::VERSIONS[i].name) == 0) Globals::SELECTED_VERSION = i;
}

void SaveSelectedVersion() {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        return;
    const wchar_t* name = Globals::VERSIONS[Globals::SELECTED_VERSION].name;
    RegSetValueExW(key, L"Version", 0, REG_SZ, (const BYTE*)name, (DWORD)((wcslen(name) + 1) * sizeof(wchar_t)));
    RegCloseKey(key);
}

// ---------------------------------------------------------------------------
// Launch Minecraft
// ---------------------------------------------------------------------------
bool LaunchMinecraft(const std::wstring& mcPath) {
    HINSTANCE result = ShellExecuteW(nullptr, L"open", mcPath.c_str(),
                                     nullptr, nullptr, SW_SHOWNORMAL);
    return (reinterpret_cast<INT_PTR>(result) > 32);
}

// ---------------------------------------------------------------------------
// Wait for the Minecraft process to appear
// ---------------------------------------------------------------------------
unsigned long WaitForMinecraft(unsigned int timeoutMs) {
    const unsigned int step = 500;
    unsigned int elapsed = 0;
    while (elapsed < timeoutMs) {
        DWORD pid = GetProcId("Minecraft.Windows.exe");
        if (pid != 0) {
            return pid;
        }
        Sleep(step);
        elapsed += step;
    }
    return 0;
}
