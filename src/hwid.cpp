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

std::string MachineGuid() {
    wchar_t buf[128]{};
    DWORD size = sizeof(buf);
    // Always the 64-bit view: a 32-bit build would otherwise read a different (redirected) key.
    if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid",
                     RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY, nullptr, buf, &size) != ERROR_SUCCESS)
        return {};
    std::string out;
    for (const wchar_t* p = buf; *p; ++p) out += (char)towlower(*p);   // GUID is plain ASCII
    return out;
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
    std::string guid = MachineGuid();
    if (guid.empty()) return {};
    unsigned char digest[32]{};
    if (!Sha256("controllin:" + guid, digest)) return {};

    static const wchar_t* HEX = L"0123456789ABCDEF";
    std::wstring id;
    for (int i = 0; i < 12; ++i) {
        if (i && i % 2 == 0) id += L'-';
        id += HEX[digest[i] >> 4];
        id += HEX[digest[i] & 15];
    }
    return id;
}
