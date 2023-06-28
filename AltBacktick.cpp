#include "AltBacktick.h"
#include "AppInstaller.h"
#include "WindowFinder.h"
#include "framework.h"
#include <shlobj.h>
#include <tlhelp32.h>
#include <vector>

int StartBackgroundApp() {
    HANDLE mutex = CreateMutex(NULL, FALSE, L"MyAltBacktickMutex");
    DWORD lastError = GetLastError();
    if (lastError == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, L"AltBacktick is already running.", L"Warning", MB_ICONERROR);
        return 0;
    }

    MSG msg;
    UINT keyCode = MapVirtualKey(BACKTICK_SCAN_CODE, MAPVK_VSC_TO_VK);
    if (!RegisterHotKey(NULL, 1, MOD_ALT, keyCode)) {
        DWORD lastError = GetLastError();
        if (lastError == ERROR_HOTKEY_ALREADY_REGISTERED) {
            MessageBox(
                NULL, L"Failed to register the ALT+~ hotkey.\nMake sure no other application is already binding to it.",
                L"Failed to register hotkey", MB_ICONEXCLAMATION);
            return 0;
        }
    }
    if (!RegisterHotKey(NULL, 2, MOD_ALT | MOD_SHIFT, keyCode)){
        DWORD lastError = GetLastError();
        if (lastError == ERROR_HOTKEY_ALREADY_REGISTERED) {
            MessageBox(
                NULL, L"Failed to register the ALT+SHIFT+~ hotkey.\nMake sure no other application is already binding to it.",
                L"Failed to register hotkey", MB_ICONEXCLAMATION);
            return 0;
        }
    }

    WindowFinder windowFinder;
    HWND lastWindow = nullptr;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            std::vector<HWND> windows = windowFinder.FindCurrentProcessWindows();
            HWND windowToFocus = nullptr;
            for (const HWND &window : windows) {
                if (window != lastWindow || windows.size() == 1) {
                    windowToFocus = window;
                }
            }

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
