#include "AltBacktick.h"
#include "AppInstaller.h"
#include "WindowFinder.h"
#include "framework.h"
#include <shlobj.h>
#include <tlhelp32.h>
#include <vector>
#include <INIReader.h>
#include "Config.h"
#include <deque>
#include <sstream>
#include <unordered_map>

HWND lastWindow = nullptr;
BOOL isModifierKeyPressed = false;
std::unordered_map<std::wstring, std::deque<HWND>> mruMap; // MRU list indexed by process name.
std::unordered_map<std::wstring, int> offsets; // Current position in the MRU list of windows for each process indexed by process name.
UINT modifierKey;

bool IsModifierKeyKeyboardEvent(const KBDLLHOOKSTRUCT *kbEvent) {
    if (modifierKey == MOD_CONTROL) {
        return kbEvent->vkCode == VK_CONTROL || kbEvent->vkCode == VK_LCONTROL || kbEvent->vkCode == VK_RCONTROL;
    }

    if (modifierKey == MOD_ALT) {
        return kbEvent->vkCode == VK_MENU || kbEvent->vkCode == VK_LMENU || kbEvent->vkCode == VK_RMENU;
    }

    return false;
}

 void UpdateMRUForProcess(const HWND currentWindow, const std::wstring &processName) {
    auto &mru = mruMap[processName];
    auto it = std::find(mru.begin(), mru.end(), currentWindow);
    if (it != mru.end()) {
        mru.erase(it);
    }
    mru.push_front(currentWindow);
    offsets[processName] = 0; // Reset the offset after MRU update
}

LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (isModifierKeyPressed && nCode == HC_ACTION && wParam == WM_KEYUP) {
        KBDLLHOOKSTRUCT* kbEvent = (KBDLLHOOKSTRUCT *)lParam;
        if (IsModifierKeyKeyboardEvent(kbEvent)) {
            HWND currentWindow = GetForegroundWindow();
            if (currentWindow != nullptr) {
                DWORD currentProcessId;
                GetWindowThreadProcessId(currentWindow, &currentProcessId);
                std::wstring currentProcessName = GetProcessNameFromProcessId(currentProcessId);
                UpdateMRUForProcess(currentWindow, currentProcessName);
            }

            isModifierKeyPressed = false;
            lastWindow = currentWindow;
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
    if (keyboardHook == NULL) {
        MessageBox(NULL, L"Failed to register keyboard hook.", L"Error", MB_ICONEXCLAMATION);
        return 0;
    }

    WindowFinder windowFinder;
    while (GetMessage(&msg, nullptr, 0, 0)) {
      
        if (msg.message == WM_HOTKEY) {
            // Get the Most Recently Used list for the currently focused window.
            HWND currentWindowHandle = GetForegroundWindow();
            DWORD currentProcessId;
            GetWindowThreadProcessId(currentWindowHandle, &currentProcessId);
            std::wstring currentProcessName = GetProcessNameFromProcessId(currentProcessId);
            std::deque<HWND> &mru = mruMap[currentProcessName];
            int& offset = offsets[currentProcessName];

            // Remove windows in the queue that don't exist anymore.
            for (auto it = mru.begin(); it != mru.end();) {
                if (!IsWindow(*it)) {
                    it = mru.erase(it);
                } else {
                    ++it;
                }
            }

            // Current window should be first if the user clicked or alt-tabbed to another window.
            if (currentWindowHandle != lastWindow) {
                UpdateMRUForProcess(currentWindowHandle, currentProcessName);
            }

            std::vector<HWND> windows = windowFinder.FindCurrentProcessWindows();
            for (const HWND &window : windows) {
                if (std::find(mru.begin(), mru.end(), window) == mru.end()) {
                    mru.push_back(window);
                }
            }

            if (mru.empty()) {
                continue;
            }
            if (mru.begin() + offset + 1 < mru.end()) {
                offset++;
            } else {
                offset = 0;
            }

            HWND windowToFocus = mru[offset];

            if (windowToFocus != nullptr && windowToFocus != currentWindowHandle) {
                WINDOWPLACEMENT placement;
                GetWindowPlacement(windowToFocus, &placement);
                if (placement.showCmd == SW_SHOWMINIMIZED) {
                    ShowWindow(windowToFocus, SW_RESTORE);
                }
                SetForegroundWindow(windowToFocus);
                lastWindow = windowToFocus;
            }

            isModifierKeyPressed = true;
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
