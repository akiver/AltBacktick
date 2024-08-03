#include "AltBacktick.h"
#include "AppInstaller.h"
#include "WindowFinder.h"
#include "framework.h"
#include <shlobj.h>
#include <tlhelp32.h>
#include <vector>
#include <INIReader.h>
#include "Config.h"
#include <iostream>
#include <sstream>

HWND lastWindow = nullptr;
BOOL modifierPressed = false;
BOOL hotkeyPressed = false;
int offset = 0;
std::vector<HWND> mru; // most recently used.
UINT modifierKey;

LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            KBDLLHOOKSTRUCT* kbEvent = (KBDLLHOOKSTRUCT *)lParam;
            modifierPressed = (modifierKey == MOD_CONTROL && (kbEvent->vkCode == VK_CONTROL || kbEvent->vkCode == VK_LCONTROL || kbEvent->vkCode == VK_RCONTROL)) ||
                              (modifierKey == MOD_ALT && (kbEvent->vkCode == VK_MENU || kbEvent->vkCode == VK_LMENU || kbEvent->vkCode == VK_RMENU));
            if (modifierPressed && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)) {
                if (hotkeyPressed && lastWindow != nullptr) {
                    std::vector<HWND>::iterator it = std::find(mru.begin(), mru.end(), lastWindow);
                    if (it != mru.end()) {
                        mru.erase(it);
                    }
                    mru.insert(mru.begin(), lastWindow);
                }
                std::ostringstream oss;
                oss << "Resetting offset to zero (was " << offset << ")" << std::endl;
                OutputDebugStringA(oss.str().c_str());
                hotkeyPressed = false;
                offset = 0;
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
int StartBackgroundApp() {
    HANDLE mutex = CreateMutex(NULL, FALSE, L"MyAltBacktickMutex");
    DWORD lastError = GetLastError();
    if (lastError == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, L"AltBacktick is already running.", L"Warning", MB_ICONERROR);
        return 0;
    }

    MSG msg;
    UINT keyCode = MapVirtualKey(BACKTICK_SCAN_CODE, MAPVK_VSC_TO_VK);
    modifierKey = Config::GetInstance()->ModifierKey();
    if (!RegisterHotKey(NULL, NULL, modifierKey, keyCode)) {
        DWORD lastError = GetLastError();
        if (lastError == ERROR_HOTKEY_ALREADY_REGISTERED) {
            MessageBox(
                NULL, L"Failed to register the hotkey.\nMake sure no other application is already binding to it.",
                L"Failed to register hotkey", MB_ICONEXCLAMATION);
            return 0;
        }
    }

    HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, 0, 0);

    WindowFinder windowFinder;
    while (GetMessage(&msg, nullptr, 0, 0)) {
      
        if (msg.message == WM_HOTKEY) {
            hotkeyPressed = true;
            if (mru.size() && mru.begin() + offset + 1 < mru.end())
                offset++;
            else
                offset = 0;
            std::vector<HWND> windows = windowFinder.FindCurrentProcessWindows();
            HWND windowToFocus = nullptr;
            /*for (std::vector<HWND>::iterator mruIt = mru.begin(); mruIt != mru.end();) {
                std::vector<HWND>::iterator wIt = std::find(windows.begin(), windows.end(), *mruIt);
                if (wIt == windows.end()) {
                    // Invalid window?
                    std::ostringstream oss;
                    oss << "Invalid window " << *mruIt << std::endl; 
                    OutputDebugStringA(oss.str().c_str());
                    //std::cout << "Invalid window " << *mruIt << std::endl;
                    mruIt = mru.erase(mruIt);
                } else
                    mruIt++;
            }*/
            for (const HWND &window : windows) {
                // Add any windows not in most recently used
                //std::cout << "Adding window " << window << std::endl;
                std::vector<HWND>::iterator it = std::find(mru.begin(), mru.end(), window);
                if (it == mru.end()) {
                    std::ostringstream oss;
                    oss << "Adding window " << window << std::endl;
                    OutputDebugStringA(oss.str().c_str());
                    mru.insert(mru.end(), window);
                }
            }
            if (!mru.size())
                continue;
            windowToFocus = *(mru.begin() + offset);
            std::ostringstream oss;
            oss << "Window to focus " << windowToFocus << " at offset " << offset << std::endl;
            OutputDebugStringA(oss.str().c_str());

            if (windowToFocus != nullptr) {
                WINDOWPLACEMENT placement;
                GetWindowPlacement(windowToFocus, &placement);
                if (placement.showCmd == SW_SHOWMINIMIZED) {
                    ShowWindow(windowToFocus, SW_RESTORE);
                }
                SetForegroundWindow(windowToFocus);
                lastWindow = windowToFocus;
            }
        }
    }

    if (mutex != nullptr) {
        CloseHandle(mutex);
        UnhookWindowsHookEx(keyboardHook);
    }

    return static_cast<int>(msg.wParam);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

#if IGNORE_INSTALLATION
    return StartBackgroundApp();
#endif

    int exitCode = 0;
    BOOL isAppStartedFromLocalAppDataFolder = AppInstaller::IsAppStartedFromLocalAppDataFolder();
    if (isAppStartedFromLocalAppDataFolder) {
        exitCode = StartBackgroundApp();
    } else {
        int clickedButton = AppInstaller::ShowInstallerTaskDialog();
        switch (clickedButton) {
        case INSTALL_BUTTON_ID:
            AppInstaller::Install();
            break;
        case UNINSTALL_BUTTON_ID:
            AppInstaller::Uninstall();
            break;
        case RUN_ONCE_BUTTON_ID:
            exitCode = StartBackgroundApp();
            break;
        }
    }

    return exitCode;
}
