#include "WindowFinder.h"
#include <psapi.h>
#include <vector>

using namespace std;

struct EnumWindowsCallbackArgs {
    EnumWindowsCallbackArgs(WindowFinder *windowFinder) : windowFinder(windowFinder) {}
    WindowFinder *windowFinder;
    vector<HWND> windowHandles;
};

wstring GetProcessNameFromProcessId(const DWORD processId) {
    wchar_t filename[MAX_PATH] = {0};
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (processHandle != nullptr) {
        GetModuleFileNameEx(processHandle, NULL, filename, MAX_PATH);
        CloseHandle(processHandle);
    }

    return wstring(filename);
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
    wstring processName = GetProcessNameFromProcessId(windowProcessId);

    return currentProcessName == processName;
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
    DWORD windowExtendedStyle = (DWORD)GetWindowLongPtr(windowHandle, GWL_EXSTYLE);
    BOOL hasAppWindowStyle = (windowExtendedStyle & WS_EX_APPWINDOW) != 0;
    if ((windowStyle & WS_POPUP) != 0 && !hasAppWindowStyle) {
        return TRUE;
    }

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
    if (SUCCEEDED(CoInitialize(nullptr))) {
        CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&desktopManager));
    }
}
