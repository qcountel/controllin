#pragma once

#include <string>

// Ensures the hidden .cache folder exists in the working dir and returns its full path.
std::wstring EnsureCacheDir();

// Downloads the latest controllin release .dll from GitHub into the hidden cache folder.
// Returns the full path to the downloaded file, or an empty string on failure.
// A human-readable status is written to outStatus.
std::wstring DownloadLatestGithubDll(std::wstring& outStatus);

// Launches Minecraft via the configured path/protocol using ShellExecuteW.
bool LaunchMinecraft(const std::wstring& mcPath);

// Waits up to timeoutMs for Minecraft.Windows.exe to appear. Returns its PID or 0.
unsigned long WaitForMinecraft(unsigned int timeoutMs);
