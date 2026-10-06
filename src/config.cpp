#include "pch.h"
#include "config.h"

Config::Config() {
    this->path = Globals::WORKING_DIR;
    this->path += "\\config.txt";
}

bool Config::serializeConfig() {
    std::wifstream cFile(this->path);

    if (cFile.is_open()) {
        std::wstring currLine;
        while (std::getline(cFile, currLine)) {

            if (currLine.empty() || currLine[0] == L'#') {
                continue;
            }

            size_t delimiterPos = currLine.find(L'=');
            if (delimiterPos == std::wstring::npos) {
                continue;
            }
            this->currentKey = currLine.substr(0, delimiterPos);
            this->currentVal = currLine.substr(delimiterPos + 1);
            this->analyzeState();
        }

        cFile.close();
        return true;
    }

    // can't load config file, so we generate a new one with default globals
    return false;
}

bool Config::analyzeBool() {
    std::transform(this->currentVal.begin(), this->currentVal.end(), this->currentVal.begin(), [](wchar_t c) {
        return static_cast<wchar_t>(std::tolower(static_cast<int>(c)));
    });
    return (this->currentVal == L"true") || (this->currentVal == L"1");
}

int Config::analyzeInt() {
    if (!this->currentVal.empty() &&
        std::all_of(this->currentVal.begin(), this->currentVal.end(), [](wchar_t c) { return ::iswdigit(c); })) {
        return std::stoi(this->currentVal);
    }
    return 0;
}

bool Config::updateConfigFile() const {
    std::wofstream cfgStream(this->path, std::ios::trunc);
    if (cfgStream.is_open()) {
        cfgStream << L"# Controllin Injector config\n";
        cfgStream << L"dllSource=" << Globals::DLL_SOURCE << L"\n";
        cfgStream << L"localDllPath=" << Globals::LOCAL_DLL_PATH << L"\n";
        cfgStream << L"mcPath=" << Globals::MC_PATH << L"\n";

        cfgStream.close();
        return true;
    }
    return false;
}

void Config::analyzeState() {
    if (this->currentKey == L"dllSource") {
        int src = this->analyzeInt();
        Globals::DLL_SOURCE = (src == Globals::SOURCE_LOCAL) ? Globals::SOURCE_LOCAL : Globals::SOURCE_GITHUB;
    }
    else if (this->currentKey == L"localDllPath") {
        Globals::LOCAL_DLL_PATH = this->currentVal;
    }
    else if (this->currentKey == L"mcPath") {
        if (!this->currentVal.empty()) {
            Globals::MC_PATH = this->currentVal;
        }
    }
    // Unknown keys are ignored silently for forward compatibility.
}
