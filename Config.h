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

             if (_modifierKey == "win") {
                return MOD_WIN;
            }

            return MOD_ALT;
        }

        BOOL IgnoreMinimizedWindows() const {
            return _ignoreMinimizedWindows;
        }
    private:
        Config();
        static Config *_instance;

        std::string _modifierKey;
        BOOL _ignoreMinimizedWindows = FALSE;
};
