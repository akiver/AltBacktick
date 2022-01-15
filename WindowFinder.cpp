#include "WindowFinder.h"
#include <psapi.h>
#include <vector>

using namespace std;

struct EnumWindowsCallbackArgs {
    EnumWindowsCallbackArgs(WindowFinder *windowFinder) : windowFinder(windowFinder) {}
    WindowFinder *windowFinder;
    vector<HWND> windowHandles;
};

LPWSTR GetProcessNameFromProcessId(const DWORD processId) {
    LPWSTR filename = new WCHAR[MAX_PATH];
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);

    if (processHandle != nullptr) {
        GetModuleFileNameEx(processHandle, NULL, filename, MAX_PATH);
        CloseHandle(processHandle);
    }

    return filename;
}

BOOL WindowFinder::IsWindowOnCurrentDesktop(HWND windowHandle) {
    if (desktopManager == NULL) {
        return TRUE;
    }

    BOOL isWindowOnCurrentDesktop;
    if (SUCCEEDED(desktopManager->IsWindowOnCurrentVirtualDesktop(windowHandle, &isWindowOnCurrentDesktop)) &&
        !isWindowOnCurrentDesktop) {
        return FALSE;
    }

    return TRUE;
}

BOOL WindowFinder::IsWindowFromCurrentProcess(HWND windowHandle) {
    DWORD windowProcessId;
    GetWindowThreadProcessId(windowHandle, &windowProcessId);
    LPWSTR processName = GetProcessNameFromProcessId(windowProcessId);

    return wcscmp(currentProcessName, processName) == 0;
}

BOOL CALLBACK EnumWindowsCallback(HWND windowHandle, LPARAM parameters) {
    EnumWindowsCallbackArgs *args = (EnumWindowsCallbackArgs *)parameters;

    if (args->windowFinder->currentWindowHandle == windowHandle) {
        return TRUE;
    }

    if (GetWindow(windowHandle, GW_OWNER) != (HWND)0 || !IsWindowVisible(windowHandle)) {
        return TRUE;
    }

    DWORD windowStyle = (DWORD)GetWindowLongPtr(windowHandle, GWL_STYLE);
    if ((windowStyle & WS_POPUP) != 0) {
        return TRUE;
    }

    DWORD windowExtendedStyle = (DWORD)GetWindowLongPtr(windowHandle, GWL_EXSTYLE);
    if ((windowExtendedStyle & WS_EX_TOOLWINDOW) != 0) {
        return TRUE;
    }

    WINDOWINFO windowInfo;
    windowInfo.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(windowHandle, &windowInfo);
    if (windowInfo.rcWindow.right - windowInfo.rcWindow.left <= 1 ||
        windowInfo.rcWindow.bottom - windowInfo.rcWindow.top <= 1) {
        return TRUE;
    }

    // Relying on PID may not be accurate with some programs, instead rely on the process name.
    if (!args->windowFinder->IsWindowFromCurrentProcess(windowHandle)) {
        return TRUE;
    }

    if (!args->windowFinder->IsWindowOnCurrentDesktop(windowHandle)) {
        return TRUE;
    }

    args->windowHandles.push_back(windowHandle);

    return TRUE;
}

std::vector<HWND> WindowFinder::FindCurrentProcessWindows() {
    currentWindowHandle = GetForegroundWindow();
    DWORD currentProcessId;
    GetWindowThreadProcessId(currentWindowHandle, &currentProcessId);
    currentProcessName = GetProcessNameFromProcessId(currentProcessId);
    EnumWindowsCallbackArgs args(this);
    EnumWindows((WNDENUMPROC)&EnumWindowsCallback, (LPARAM)&args);

    return args.windowHandles;
}

WindowFinder::WindowFinder() : desktopManager(nullptr) {
    CoInitialize(nullptr);
    CoCreateInstance(__uuidof(VirtualDesktopManager), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&desktopManager));
}
