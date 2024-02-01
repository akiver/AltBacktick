#include "Config.h"
#include "INIReader.h"
#include <string>

Config *Config::_instance = nullptr;

std::string GetHomeDirectoryPath() {
    PWSTR path;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &path))) {
        std::wstring wpath(path);
        CoTaskMemFree(path);

        return std::string(wpath.begin(), wpath.end());
    }

    return "";
}

Config::Config() {
    std::string homePath = GetHomeDirectoryPath();
    if (homePath.empty()) {
        return;
    }

    std::string configPath = homePath + "\\.altbacktick";

    INIReader reader(configPath);
    if (reader.ParseError() < 0) {
        return;
    }

    _modifierKey = reader.GetString("keyboard", "modifier_key", "alt");
}


Config *Config::GetInstance() {
    if (_instance == nullptr) {
        _instance = new Config();
    }

    return _instance;
}
