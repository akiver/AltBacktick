#pragma once
#pragma comment(lib, "comctl32.lib")
#pragma comment(                                                                                                       \
    linker,                                                                                                            \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <string>
#include <windows.h>

#define INSTALL_BUTTON_ID 1000
#define RUN_ONCE_BUTTON_ID 1001
#define UNINSTALL_BUTTON_ID 1002

class AppInstaller {
  public:
    static void Install();
    static void Uninstall();
    static BOOL IsAppInstalled();
    static int ShowInstallerTaskDialog();
    static BOOL IsAppStartedFromLocalAppDataFolder();

  private:
    static BOOL AppExecutableExists();
    static PWSTR GetAppLocalFolderPath();
    static void KillAppProcess();
    static std::wstring GetCurrentProcessExecutablePath();
};
