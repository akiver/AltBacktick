#pragma once
#include <shlobj.h>
#include <string>

class Config {
    public:
        static Config *GetInstance();

        Config(const Config &) = delete;
        Config &operator=(const Config &) = delete;

        UINT ModifierKey() const {
            if (_modifierKey == "ctrl") {
                return MOD_CONTROL;
            }

            return MOD_ALT;
        }
    private:
        Config();
        static Config *_instance;

        std::string _modifierKey;
};
