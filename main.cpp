#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

#include "MainWindow.h"

#pragma comment(lib, "comctl32.lib")

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    // Initialiser Common Controls pour les styles modernes
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // Cr�er la fen�tre principale
    MainWindow mainWindow;

    if (!mainWindow.Create(hInstance)) {
        MessageBoxW(nullptr, L"Failed to create main window!", L"Error", MB_ICONERROR);
        return 1;
    }

    mainWindow.Show(nCmdShow);

    // Boucle de messages
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
