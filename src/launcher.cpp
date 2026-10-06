#include "pch.h"
#include "launcher.h"
#include "config.h"
#include "inject.h"

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

// ---------------------------------------------------------------------------
// Very small JSON string-value extractor (no external deps).
// Finds the first "browser_download_url" that ends with .dll.
// ---------------------------------------------------------------------------
static std::string ExtractDllAssetUrl(const std::string& json) {
    const std::string key = "\"browser_download_url\"";
    size_t pos = 0;
    while ((pos = json.find(key, pos)) != std::string::npos) {
        size_t colon = json.find(':', pos + key.size());
        if (colon == std::string::npos) break;
        size_t firstQuote = json.find('"', colon);
        if (firstQuote == std::string::npos) break;
        size_t secondQuote = json.find('"', firstQuote + 1);
        if (secondQuote == std::string::npos) break;

        std::string value = json.substr(firstQuote + 1, secondQuote - firstQuote - 1);
        // Prefer .dll assets
        if (value.size() >= 4 &&
            _stricmp(value.c_str() + value.size() - 4, ".dll") == 0) {
            return value;
        }
        pos = secondQuote + 1;
    }
    return std::string();
}

// ---------------------------------------------------------------------------
// Download latest GitHub release .dll into hidden cache
// ---------------------------------------------------------------------------
std::wstring DownloadLatestGithubDll(std::wstring& outStatus) {
    std::vector<char> apiResponse;
    if (!HttpGet(Globals::GITHUB_API_URL, apiResponse, outStatus)) {
        return std::wstring();
    }

    std::string json(apiResponse.begin(), apiResponse.end());
    std::string assetUrl = ExtractDllAssetUrl(json);
    if (assetUrl.empty()) {
        outStatus = L"No .dll asset found in latest release";
        return std::wstring();
    }

    std::wstring wAssetUrl(assetUrl.begin(), assetUrl.end());

    // Derive a filename from the URL
    std::wstring fileName = L"controllin.dll";
    size_t slash = wAssetUrl.find_last_of(L'/');
    if (slash != std::wstring::npos && slash + 1 < wAssetUrl.size()) {
        fileName = wAssetUrl.substr(slash + 1);
    }

    std::vector<char> dllData;
    if (!HttpGet(wAssetUrl, dllData, outStatus) || dllData.empty()) {
        if (outStatus == L"OK") outStatus = L"Downloaded file is empty";
        return std::wstring();
    }

    std::wstring cacheDir = EnsureCacheDir();
    std::wstring dllPath = cacheDir + L"\\" + fileName;

    HANDLE hFile = CreateFileW(dllPath.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        outStatus = L"Failed to write DLL to cache";
        return std::wstring();
    }

    DWORD written = 0;
    WriteFile(hFile, dllData.data(), (DWORD)dllData.size(), &written, nullptr);
    CloseHandle(hFile);

    if (written != dllData.size()) {
        outStatus = L"Incomplete write to cache";
        return std::wstring();
    }

    outStatus = L"Downloaded latest release";
    return dllPath;
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
