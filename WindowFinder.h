#pragma once
#include <shlobj.h>
#include <string>
#include <vector>

class WindowFinder {
  public:
    WindowFinder();
    std::vector<HWND> FindCurrentProcessWindows();
    BOOL IsWindowOnCurrentDesktop(HWND windowHandle);
    BOOL IsWindowFromCurrentProcess(HWND windowHandle);
    HWND currentWindowHandle;
    std::wstring currentProcessName;

  private:
    IVirtualDesktopManager *desktopManager; // Win10 ++ only.
};

std::wstring GetProcessNameFromProcessId(const DWORD processId);