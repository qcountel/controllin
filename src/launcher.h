#pragma once

#include <string>

// Ensures the hidden .cache folder exists in the working dir and returns its full path.
std::wstring EnsureCacheDir();

// Downloads the .dll of the newest controllin release whose title contains `tag` as a word
// (e.g. "1.16" matches "v2.0.5 1.16") into the hidden cache folder (.cache\<tag>\).
// Returns the full path to the downloaded file, or an empty string on failure.
// outRelease receives the release title; a human-readable status is written to outStatus.
std::wstring DownloadLatestGithubDll(const char* tag, std::wstring& outRelease, std::wstring& outStatus);

// Selected game version (Globals::SELECTED_VERSION), stored in HKCU\Software\Controllin.
void LoadSelectedVersion();
void SaveSelectedVersion();

// Launches Minecraft via the configured path/protocol using ShellExecuteW.
bool LaunchMinecraft(const std::wstring& mcPath);

// Waits up to timeoutMs for Minecraft.Windows.exe to appear. Returns its PID or 0.
unsigned long WaitForMinecraft(unsigned int timeoutMs);
