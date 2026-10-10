#pragma once

#include <string>

// Hardware ID used to issue a Controllin + subscription key.
// SHA-256 of "controllin:" + the Windows MachineGuid (HKLM\SOFTWARE\Microsoft\Cryptography),
// first 12 bytes as "XXXX-XXXX-XXXX-XXXX-XXXX-XXXX". Stable across launches and updates of the
// injector; changes only when Windows is reinstalled. Empty string if it cannot be read.
std::wstring GetHWID();
