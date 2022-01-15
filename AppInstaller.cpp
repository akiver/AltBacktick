#include "AppInstaller.h"
#include <shlobj.h>
#include <string>
#include <tlhelp32.h>

using namespace std;

wstring AppInstaller::GetCurrentProcessExecutablePath() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileName(0, buffer, MAX_PATH);

    return wstring(buffer);
}

PWSTR AppInstaller::GetAppLocalFolderPath() {
    PWSTR localAppDataFolderPath = nullptr;
    HRESULT hResult = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &localAppDataFolderPath);
    if (hResult != S_OK) {
        MessageBox(NULL, L"Failed to detect local AppData folder location.", L"Error", MB_ICONERROR);
    }

    return localAppDataFolderPath;
}

void AppInstaller::KillAppProcess() {
    DWORD currentProcessId = GetCurrentProcessId();
    HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(processEntry);
    BOOL entryExists = Process32First(snapshotHandle, &processEntry);
    while (entryExists) {
        if (wcscmp(processEntry.szExeFile, L"AltBacktick.exe") == 0 && processEntry.th32ProcessID != currentProcessId) {
            HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, 0, processEntry.th32ProcessID);
            if (processHandle != nullptr) {
                TerminateProcess(processHandle, 9);
                CloseHandle(processHandle);
            }
        }
        entryExists = Process32Next(snapshotHandle, &processEntry);
    }
    CloseHandle(snapshotHandle);
}

BOOL AppInstaller::AppExecutableExists() {
    PWSTR localAppDataFolderPath = GetAppLocalFolderPath();
    if (localAppDataFolderPath == nullptr) {
        return FALSE;
    }

    wstring executableInstallationPath(localAppDataFolderPath + wstring(L"\\AkiVer\\AltBacktick\\AltBacktick.exe") +
                                       L'\0');
    DWORD fileAttributes = GetFileAttributes(executableInstallationPath.c_str());

    return (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL AppInstaller::IsAppInstalled() {
    char buffer[512];
    DWORD bufferSize = sizeof(buffer);
    HRESULT result = RegGetValue(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                 L"AltBacktick", RRF_RT_REG_SZ, nullptr, &buffer, &bufferSize);
    RegCloseKey(HKEY_CURRENT_USER);

    return result == ERROR_SUCCESS && AppExecutableExists();
}

BOOL AppInstaller::IsAppStartedFromLocalAppDataFolder() {
    wchar_t executablePath[MAX_PATH];
    GetModuleFileName(0, executablePath, MAX_PATH);

    return wcsstr(executablePath, L"\\AppData\\Local\\AkiVer\\AltBacktick") != nullptr;
}

int AppInstaller::ShowInstallerTaskDialog() {
    BOOL isAppInstalled = IsAppInstalled();
    TASKDIALOGCONFIG dialogConfig;
    TASKDIALOG_BUTTON buttons[2];
    if (isAppInstalled) {
        buttons[0] = {RUN_ONCE_BUTTON_ID, L"Run AltBacktick without installing"};
        buttons[1] = {UNINSTALL_BUTTON_ID, L"Uninstall AltBacktick"};
    } else {
        buttons[0] = {INSTALL_BUTTON_ID, L"Automatically start AltBacktick"};
        buttons[1] = {RUN_ONCE_BUTTON_ID, L"Run AltBacktick without installing"};
    }

    ZeroMemory(&dialogConfig, sizeof(dialogConfig));
    dialogConfig.cbSize = sizeof(TASKDIALOGCONFIG);
    dialogConfig.pButtons = buttons;
    dialogConfig.cButtons = 2;
    dialogConfig.pszMainIcon = TD_INFORMATION_ICON;
    dialogConfig.pszMainInstruction =
        L"Would you like AltBacktick to start automatically the next time you log in to your PC?";
    dialogConfig.pszWindowTitle = L"Start AltBacktick automatically?";
    dialogConfig.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS;
    int clickedButtonNumber;
    TaskDialogIndirect(&dialogConfig, &clickedButtonNumber, NULL, NULL);

    return clickedButtonNumber;
}

void AppInstaller::Install() {
    PWSTR localAppDataFolderPath = GetAppLocalFolderPath();
    if (localAppDataFolderPath == nullptr) {
        return;
    }

    wstring executableInstallationPath(localAppDataFolderPath + wstring(L"\\AkiVer\\AltBacktick\\AltBacktick.exe") +
                                       L'\0');
    HKEY hKey = NULL;
    RegCreateKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey);
    RegSetValueEx(hKey, L"AltBacktick", 0, REG_SZ, (BYTE *)executableInstallationPath.c_str(),
                  ((DWORD)executableInstallationPath.length() + 1) * 2);

    wstring currentExecutablePath = GetCurrentProcessExecutablePath();
    currentExecutablePath.push_back('\0');

    SHFILEOPSTRUCT fileOperation = {nullptr};
    fileOperation.wFunc = FO_COPY;
    fileOperation.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_FILESONLY;
    fileOperation.pFrom = currentExecutablePath.c_str();
    fileOperation.pTo = executableInstallationPath.c_str();
    int result = SHFileOperation(&fileOperation);

    if (result != 0) {
        MessageBox(NULL, L"Failed to copy executable into local AppData folder.", L"Error", MB_ICONERROR);
        return;
    }

    STARTUPINFO startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInformation;
    ZeroMemory(&processInformation, sizeof(processInformation));
    if (!CreateProcess(NULL, (LPWSTR)executableInstallationPath.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo,
                       &processInformation)) {
        MessageBox(NULL, L"Failed to start the application.", L"Error", MB_ICONERROR);
    }
    CloseHandle(processInformation.hThread);
    CloseHandle(processInformation.hProcess);
}

void AppInstaller::Uninstall() {
    HKEY hKey = NULL;
    HRESULT result =
        RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
    if (result == ERROR_SUCCESS) {
        RegDeleteValue(hKey, L"AltBacktick");
    }
    RegCloseKey(hKey);

    KillAppProcess();
    PWSTR localAppDataFolderPath = GetAppLocalFolderPath();
    if (localAppDataFolderPath != nullptr) {
        wstring installationFolderPath(localAppDataFolderPath + wstring(L"\\AkiVer\\AltBacktick") + L'\0');
        SHFILEOPSTRUCT fileOperation = {nullptr};
        fileOperation.wFunc = FO_DELETE;
        fileOperation.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI;
        fileOperation.pFrom = installationFolderPath.c_str();
        SHFileOperation(&fileOperation);
    }
}
