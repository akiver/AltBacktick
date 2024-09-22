#pragma once
#include <shlobj.h>
#include <string>
#include <vector>

class WindowFinder {
  public:
    WindowFinder();
    std::vector<HWND> FindCurrentProcessWindows();
    BOOL IsWindowOnCurrentDesktop(HWND windowHandle) const;
    BOOL IsWindowFromCurrentProcess(HWND windowHandle) const;
    std::wstring GetProcessUniqueId(HWND windowHandle) const;

  private:
    IVirtualDesktopManager *desktopManager; // Win10 ++ only.
    static std::wstring GetProcessNameFromProcessId(DWORD processId);
    std::wstring GetCurrentDesktopId() const;
    std::wstring GetCurrentProcessUniqueId() const;
};