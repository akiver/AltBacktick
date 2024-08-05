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
#include <unordered_map>

HWND lastWindow = nullptr;
BOOL modifierPressed = false;
BOOL hotkeyPressed = false;
std::unordered_map<std::wstring, int> offsets; // process name to current position in the handles vector.
std::unordered_map<std::wstring, std::vector<HWND>> mruMap; // hash table of most recently used windows indexed by process name.
UINT modifierKey;

LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            KBDLLHOOKSTRUCT* kbEvent = (KBDLLHOOKSTRUCT *)lParam;
            // Whether the configured modifier key is being pressed or released.
            modifierPressed = (modifierKey == MOD_CONTROL && (kbEvent->vkCode == VK_CONTROL || kbEvent->vkCode == VK_LCONTROL || kbEvent->vkCode == VK_RCONTROL)) ||
                              (modifierKey == MOD_ALT && (kbEvent->vkCode == VK_MENU || kbEvent->vkCode == VK_LMENU || kbEvent->vkCode == VK_RMENU));
            if (modifierPressed && (wParam == WM_KEYUP /* || wParam == WM_SYSKEYUP*/)) {
                // Get the Most Recently Used list for the currently focused window.
                HWND currentWindow = GetForegroundWindow();
                DWORD currentProcessId;
                GetWindowThreadProcessId(currentWindow, &currentProcessId);
                std::wstring currentProcessName = GetProcessNameFromProcessId(currentProcessId);
                std::vector<HWND>& mru = mruMap[currentProcessName];
                // After a WM_HOTKEY event, move focused window to the front.
                if (hotkeyPressed && currentWindow != nullptr) {
                    std::vector<HWND>::iterator it = std::find(mru.begin(), mru.end(), currentWindow);
                    if (it != mru.end()) {
                        mru.erase(it);
                    }
                    mru.insert(mru.begin(), currentWindow);
                }
                std::ostringstream oss;
                oss << "Resetting offset to zero (was " << offsets[currentProcessName] << ")" << std::endl;
                OutputDebugStringA(oss.str().c_str());
                hotkeyPressed = false;
                lastWindow = currentWindow;
                offsets[currentProcessName] = 0;
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
            // Get the Most Recently Used list for the currently focused window.
            HWND currentWindowHandle = GetForegroundWindow();
            DWORD currentProcessId;
            GetWindowThreadProcessId(currentWindowHandle, &currentProcessId);
            std::wstring currentProcessName = GetProcessNameFromProcessId(currentProcessId);
            std::vector<HWND>& mru = mruMap[currentProcessName];
            int& offset = offsets[currentProcessName];
            OutputDebugStringW(currentProcessName.c_str());
            std::vector<HWND> windows = windowFinder.FindCurrentProcessWindows();

            // Current window should be first if the user clicked or alt-tabbed to another window.
            if (currentWindowHandle != lastWindow) {
                HWND curWindow = GetForegroundWindow();
                std::vector<HWND>::iterator it = std::find(mru.begin(), mru.end(), curWindow);
                if (it != mru.end()) {
                    mru.erase(it);
                }
                mru.insert(mru.begin(), curWindow);
            }

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
            if (mru.begin() + offset + 1 < mru.end())
                offset++;
            else
                offset = 0;

            windowToFocus = mru[offset];
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
            hotkeyPressed = true;
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
