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
// Wide Windows paths only: byte-wise ANSI -> wchar_t breaks Cyrillic usernames.
// ---------------------------------------------------------------------------
static std::wstring FileError(const std::wstring& action, DWORD error,
                              const std::wstring& path) {
    wchar_t message[512]{};
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, error, 0, message, 512, nullptr);
    std::wstring detail(message);
    while (!detail.empty() && (detail.back() == L'\r' || detail.back() == L'\n' || detail.back() == L' '))
        detail.pop_back();
    std::wstring result = action + L" (Win32 " + std::to_wstring(error) + L")";
    if (!detail.empty()) result += L": " + detail;
    if (!path.empty()) result += L"\n" + path;
    return result;
}

static bool EnsureDirectory(const std::wstring& path, std::wstring& outStatus) {
    if (CreateDirectoryW(path.c_str(), nullptr)) return true;
    DWORD error = GetLastError();
    if (error == ERROR_ALREADY_EXISTS) {
        DWORD attributes = GetFileAttributesW(path.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY))
            return true;
    }
    outStatus = FileError(L"Cannot create cache folder", error, path);
    return false;
}

std::wstring EnsureCacheDir(std::wstring& outStatus) {
    DWORD needed = GetCurrentDirectoryW(0, nullptr);
    if (needed == 0) {
        outStatus = FileError(L"Cannot read working directory", GetLastError(), L"");
        return {};
    }
    std::vector<wchar_t> buffer(needed);
    DWORD length = GetCurrentDirectoryW(needed, buffer.data());
    if (length == 0) {
        outStatus = FileError(L"Cannot read working directory", GetLastError(), L"");
        return {};
    }
    if (length >= needed) {
        outStatus = L"Working directory changed - try again";
        return {};
    }
    std::wstring cacheDir(buffer.data(), length);
    if (cacheDir.back() != L'\\' && cacheDir.back() != L'/') cacheDir += L'\\';
    cacheDir += Globals::CACHE_DIR_NAME;
    if (!EnsureDirectory(cacheDir, outStatus)) return {};

    DWORD attributes = GetFileAttributesW(cacheDir.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES)
        SetFileAttributesW(cacheDir.c_str(), attributes | FILE_ATTRIBUTE_HIDDEN);
    return cacheDir;
}

// WinINet errors often have no text in the system message table.
static std::wstring NetworkError(const std::wstring& action, DWORD error,
                                 const std::wstring& url) {
    wchar_t message[512]{};
    DWORD length = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  nullptr, error, 0, message, 512, nullptr);
    if (length == 0) {
        HMODULE module = GetModuleHandleW(L"wininet.dll");
        if (module) FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                                   module, error, 0, message, 512, nullptr);
    }
    std::wstring detail(message);
    while (!detail.empty() && (detail.back() == L'\r' || detail.back() == L'\n' || detail.back() == L' '))
        detail.pop_back();
    std::wstring result = action + L" (WinINet " + std::to_wstring(error) + L")";
    if (!detail.empty()) result += L": " + detail;
    result += L"\n" + url;
    OutputDebugStringW((result + L"\n").c_str());
    return result;
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
        outStatus = NetworkError(L"InternetOpen failed", GetLastError(), url);
        return false;
    }

    DWORD connectTimeout = 15000, sendTimeout = 15000, receiveTimeout = 30000;
    InternetSetOptionW(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &connectTimeout, sizeof(connectTimeout));
    InternetSetOptionW(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &sendTimeout, sizeof(sendTimeout));
    InternetSetOptionW(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &receiveTimeout, sizeof(receiveTimeout));

    // GitHub API requires a User-Agent (set above) and we follow redirects by default.
    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                  INTERNET_FLAG_SECURE | INTERNET_FLAG_KEEP_CONNECTION;

    HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(),
                                      L"Accept: application/vnd.github+json\r\n",
                                      (DWORD)-1L, flags, 0);
    if (!hUrl) {
        DWORD error = GetLastError();
        outStatus = NetworkError(L"InternetOpenUrl failed", error, url);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Check HTTP status code
    DWORD statusCode = 0;
    DWORD len = sizeof(statusCode);
    if (!HttpQueryInfoW(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                        &statusCode, &len, nullptr)) {
        outStatus = NetworkError(L"Cannot read HTTP status", GetLastError(), url);
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }
    if (statusCode < 200 || statusCode >= 300) {
        outStatus = L"HTTP error " + std::to_wstring(statusCode) + L"\n" + url;
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[8192];
    DWORD bytesRead = 0;
    for (;;) {
        if (!InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead)) {
            DWORD error = GetLastError();
            outStatus = NetworkError(L"Download interrupted", error, url);
            outData.clear();
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInternet);
            return false;
        }
        if (bytesRead == 0) break;
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

    // Each game version gets its own subfolder so DLLs never mix.
    std::wstring cacheDir = EnsureCacheDir(outStatus);
    if (cacheDir.empty()) return {};
    std::wstring versionDir = cacheDir + L"\\" + Utf8ToWide(tag);
    if (!EnsureDirectory(versionDir, outStatus)) return {};
    std::wstring fileName = Utf8ToWide(assetName);
    if (fileName.empty() || fileName.find_first_of(L"\\/:") != std::wstring::npos)
        fileName = L"controllin.dll";
    std::wstring dllPath = versionDir + L"\\" + fileName;

    // A failed download/write must never truncate a previously good DLL.
    std::wstring tempPath = dllPath + L".part-" + std::to_wstring(GetCurrentProcessId())
                            + L"-" + std::to_wstring(GetTickCount64());
    HANDLE hFile = CreateFileW(tempPath.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        outStatus = FileError(L"Cannot write DLL cache", GetLastError(), tempPath);
        return {};
    }

    DWORD writeError = ERROR_SUCCESS;
    size_t offset = 0;
    while (offset < dllData.size()) {
        DWORD chunk = static_cast<DWORD>((std::min)(dllData.size() - offset,
                                                  static_cast<size_t>(1024 * 1024)));
        DWORD written = 0;
        if (!WriteFile(hFile, dllData.data() + offset, chunk, &written, nullptr)) {
            writeError = GetLastError();
            break;
        }
        if (written == 0) {
            writeError = ERROR_WRITE_FAULT;
            break;
        }
        offset += written;
    }
    if (writeError == ERROR_SUCCESS && !FlushFileBuffers(hFile)) writeError = GetLastError();
    if (!CloseHandle(hFile) && writeError == ERROR_SUCCESS) writeError = GetLastError();
    if (writeError != ERROR_SUCCESS) {
        DeleteFileW(tempPath.c_str());
        outStatus = FileError(L"Cannot finish DLL cache write", writeError, dllPath);
        return {};
    }
    if (!MoveFileExW(tempPath.c_str(), dllPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DWORD error = GetLastError();
        DeleteFileW(tempPath.c_str());
        outStatus = FileError(error == ERROR_SHARING_VIOLATION
            ? L"DLL is in use - restart Minecraft"
            : L"Cannot replace cached DLL", error, dllPath);
        return {};
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
