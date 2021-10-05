#pragma once
#include <shlobj.h>
#include <vector>

class WindowFinder {
  public:
    WindowFinder();
    std::vector<HWND> FindCurrentProcessWindows();
    BOOL IsWindowOnCurrentDesktop(HWND windowHandle);
    BOOL IsWindowFromCurrentProcess(HWND windowHandle);
    HWND currentWindowHandle;
    LPWSTR currentProcessName;

  private:
    IVirtualDesktopManager *desktopManager; // Win10 ++ only.
};
