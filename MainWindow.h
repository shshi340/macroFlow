#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "MacroManager.h"
#include "HotkeyManager.h"
#include "MacroExecutor.h"

enum class MacroCategory {
    BASIC,
    IMAGE,
    COMBO
};

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Create(HINSTANCE hInstance);
    void Show(int nCmdShow);

    HWND GetHwnd() const { return m_hwnd; }
    HINSTANCE GetInstance() const { return m_hInstance; }

    // Fontes publiques pour les dialogues
    HFONT m_fontTitle;
    HFONT m_fontNormal;
    HFONT m_fontSmall;
    HFONT m_fontBold;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void CreateControls();
    void PaintSidebar(HDC hdc, RECT& rect);
    void PaintContent(HDC hdc, RECT& rect);
    void PaintMacroCard(HDC hdc, int x, int y, int index);
    void DrawCategoryButtons(HDC hdc);

    void SwitchCategory(MacroCategory category);
    void RefreshMacroList();

    void OnAddMacro();
    void OnEditMacro(int index);
    void OnDeleteMacro(int index);
    void OnToggleStatus(int index);

    void ShowBasicMacroDialog(int editIndex = -1);
    void ShowImageMacroDialog(int editIndex = -1);
    void ShowComboMacroDialog(int editIndex = -1);

    static INT_PTR CALLBACK ComboMacroDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Nouvelles fonctions
    void SaveMacros();
    void LoadMacros();
    void StartHotkeyMonitoring();
    void StopHotkeyMonitoring();
    void ProcessHotkeys();

    HWND m_hwnd;
    HINSTANCE m_hInstance;
    HWND m_contentPanel;
    HWND m_scrollBar;

    MacroCategory m_currentCategory;
    int m_scrollPos;
    int m_hoverIndex;

    bool m_macrosEnabled;

    // Gestionnaires
    MacroManager m_macroManager;
    HotkeyManager m_hotkeyManager;
    MacroExecutor m_macroExecutor;

    // Thread de monitoring
    HANDLE m_monitorThread;
    bool m_monitorRunning;

    // Références aux vecteurs de macros
    std::vector<BasicMacro>& m_basicMacros;
    std::vector<ImageMacro>& m_imageMacros;
    std::vector<ComboMacro>& m_comboMacros;

    // Couleurs
    COLORREF COLOR_BG;
    COLORREF COLOR_SIDEBAR;
    COLORREF COLOR_CARD;
    COLORREF COLOR_TEXT;
    COLORREF COLOR_TEXT_GRAY;
    COLORREF COLOR_GREEN;
    COLORREF COLOR_BLUE;
    COLORREF COLOR_PURPLE;
    COLORREF COLOR_BORDER;

    struct DialogData {
        MainWindow* pMainWindow;
        int editIndex;
        BasicMacro* basicMacro;
        ImageMacro* imageMacro;
        ComboMacro* comboMacro;

        DialogData() : pMainWindow(nullptr), editIndex(-1),
                       basicMacro(nullptr), imageMacro(nullptr), comboMacro(nullptr) {}
    };
};
