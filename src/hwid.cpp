// Compiled without the precompiled header.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>

#include <string>
#include <vector>

#include "hwid.h"

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")

namespace {

// MUST stay identical to the client (controllin-client, crates/client/src/license.rs,
// hwid_bytes / machine_guid / volume_serial): the key is checked against the HWID the
// DLL computes inside the game, so the injector has to show exactly the same value.
//   source = "guid:" + lowercase(trim(MachineGuid))     (if MachineGuid has >= 16 chars)
//          | "vol:" + %08x serial of the system drive   (fallback)
//          | "none"
//   HWID   = first 12 bytes of sha256("controllin-hwid-v1|" + source),
//            hex upper-case in groups of 4: XXXX-XXXX-XXXX-XXXX-XXXX-XXXX

std::string Trim(const std::string& s) {
    const char* ws = " \t\r\n\v\f";
    size_t a = s.find_first_not_of(ws);
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

std::string MachineGuid() {
    wchar_t buf[128]{};
    DWORD size = sizeof(buf);
    // Always the 64-bit view: a 32-bit build would otherwise read a different (redirected) key.
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid",
                     RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY, nullptr, buf, &size) != ERROR_SUCCESS)
        return {};
    std::string out;
    for (const wchar_t* p = buf; *p; ++p) out += (char)*p;   // GUID is plain ASCII
    if (out.size() < 16) return {};                            // client: s.len() >= 16
    out = Trim(out);
    for (char& c : out)
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    return out;
}

std::string VolumeSource() {
    wchar_t drive[MAX_PATH]{};
    DWORD n = GetEnvironmentVariableW(L"SystemDrive", drive, MAX_PATH);
    std::wstring root = (n > 0 && n < MAX_PATH) ? std::wstring(drive) : std::wstring(L"C:");
    root += L"\\";
    DWORD serial = 0;
    if (!GetVolumeInformationW(root.c_str(), nullptr, 0, &serial, nullptr, nullptr, nullptr, 0) || serial == 0)
        return {};
    char hex[16]{};
    wsprintfA(hex, "%08x", serial);
    return std::string("vol:") + hex;
}

bool Sha256(const std::string& data, unsigned char out[32]) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    bool ok = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0 &&
              BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) == 0 &&
              BCryptHashData(hash, (PUCHAR)data.data(), (ULONG)data.size(), 0) == 0 &&
              BCryptFinishHash(hash, out, 32, 0) == 0;
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
    return ok;
}

} // namespace

std::wstring GetHWID() {
    // ONE source, always the first that works (same order as the client)
    std::string source;
    std::string guid = MachineGuid();
    if (!guid.empty()) source = "guid:" + guid;
    else source = VolumeSource();
    if (source.empty()) source = "none";

    unsigned char digest[32]{};
    if (!Sha256("controllin-hwid-v1|" + source, digest)) return {};

    static const wchar_t* HEX = L"0123456789ABCDEF";
    std::wstring id;
    for (int i = 0; i < 12; ++i) {
        if (i && i % 2 == 0) id += L'-';
        id += HEX[digest[i] >> 4];
        id += HEX[digest[i] & 15];
    }
    return id;
}
