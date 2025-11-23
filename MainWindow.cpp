#include "MainWindow.h"
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <sstream>
#include <cstdio>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// IDs de contrôles
#define ID_BTN_BASIC        1001
#define ID_BTN_IMAGE        1002
#define ID_BTN_COMBO        1003
#define ID_BTN_ADD          1004
#define ID_BTN_SETTINGS     1005

// IDs pour les dialogues
#define ID_EDIT_NAME        2001
#define ID_EDIT_HOTKEY      2002
#define ID_LIST_ACTIONS     2003
#define ID_BTN_ADD_ACTION   2004
#define ID_BTN_REMOVE_ACTION 2005
#define ID_BTN_SAVE         2006
#define ID_BTN_CANCEL       2007
#define ID_EDIT_IMAGE_PATH  2008
#define ID_BTN_BROWSE       2009
#define ID_EDIT_ACTION      2010
#define ID_SLIDER_CONFIDENCE 2011
#define ID_LABEL_CONFIDENCE 2012
#define ID_LIST_SKILLS      2013
#define ID_BTN_ADD_SKILL    2014
#define ID_BTN_REMOVE_SKILL 2015
#define ID_EDIT_DELAY       2016
#define ID_CHECK_COOLDOWN   2017

#pragma warning(disable: 4312)

MainWindow::MainWindow()
    : m_hwnd(nullptr)
    , m_hInstance(nullptr)
    , m_contentPanel(nullptr)
    , m_scrollBar(nullptr)
    , m_currentCategory(MacroCategory::BASIC)
    , m_scrollPos(0)
    , m_hoverIndex(-1)
    , m_fontTitle(nullptr)
    , m_fontNormal(nullptr)
    , m_fontSmall(nullptr)
    , m_fontBold(nullptr)
    , m_macrosEnabled(true)
    , m_monitorThread(nullptr)
    , m_monitorRunning(false)
    , m_basicMacros(m_macroManager.basicMacros)
    , m_imageMacros(m_macroManager.imageMacros)
    , m_comboMacros(m_macroManager.comboMacros)
{
    COLOR_BG = RGB(17, 24, 39);
    COLOR_SIDEBAR = RGB(31, 41, 55);
    COLOR_CARD = RGB(31, 41, 55);
    COLOR_TEXT = RGB(229, 231, 235);
    COLOR_TEXT_GRAY = RGB(156, 163, 175);
    COLOR_GREEN = RGB(16, 185, 129);
    COLOR_BLUE = RGB(59, 130, 246);
    COLOR_PURPLE = RGB(168, 85, 247);
    COLOR_BORDER = RGB(55, 65, 81);
}

MainWindow::~MainWindow() {
    StopHotkeyMonitoring();
    SaveMacros();

    if (m_fontTitle) DeleteObject(m_fontTitle);
    if (m_fontNormal) DeleteObject(m_fontNormal);
    if (m_fontSmall) DeleteObject(m_fontSmall);
    if (m_fontBold) DeleteObject(m_fontBold);
}

void MainWindow::SaveMacros() {
    wchar_t path[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, path);
    std::wstring fullPath = std::wstring(path) + L"\\macros.json";
    m_macroManager.SaveToFile(fullPath);
}

void MainWindow::LoadMacros() {
    wchar_t path[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, path);
    std::wstring fullPath = std::wstring(path) + L"\\macros.json";
    m_macroManager.LoadFromFile(fullPath);
    RefreshMacroList();
}

void MainWindow::StartHotkeyMonitoring() {
    if (m_monitorRunning) return;

    m_monitorRunning = true;

    // Enregistrer tous les hotkeys
    int hotkeyId = 1000;
    for (const auto& macro : m_basicMacros) {
        if (macro.enabled) {
            m_hotkeyManager.RegisterHotkey(hotkeyId++, macro.hotkey, macro.holdMode);
        }
    }

    for (const auto& macro : m_comboMacros) {
        if (macro.enabled) {
            m_hotkeyManager.RegisterHotkey(hotkeyId++, macro.hotkey, false);
        }
    }

    // Créer le thread de monitoring
    m_monitorThread = CreateThread(nullptr, 0, [](LPVOID param) -> DWORD {
        MainWindow* pThis = (MainWindow*)param;
        pThis->ProcessHotkeys();
        return 0;
    }, this, 0, nullptr);
}

void MainWindow::StopHotkeyMonitoring() {
    m_monitorRunning = false;
    if (m_monitorThread) {
        WaitForSingleObject(m_monitorThread, 1000);
        CloseHandle(m_monitorThread);
        m_monitorThread = nullptr;
    }
}

void MainWindow::ProcessHotkeys() {
    while (m_monitorRunning) {
        // Vérifier les macros basiques
        for (const auto& macro : m_basicMacros) {
            if (!macro.enabled) continue;

            if (macro.holdMode) {
                // Mode maintien : exécuter tant que la touche est maintenue
                if (m_hotkeyManager.IsKeyHeld(macro.hotkey)) {
                    if (!m_macroExecutor.IsExecuting()) {
                        m_macroExecutor.ExecuteBasicMacro(macro);
                    }
                } else {
                    if (m_macroExecutor.IsExecuting()) {
                        m_macroExecutor.StopExecution();
                    }
                }
            } else {
                // Mode pression simple
                static std::map<std::wstring, bool> keyStates;
                bool isPressed = m_hotkeyManager.IsKeyPressed(macro.hotkey);

                if (isPressed && !keyStates[macro.hotkey]) {
                    m_macroExecutor.ExecuteBasicMacro(macro);
                }
                keyStates[macro.hotkey] = isPressed;
            }
        }

        // Vérifier les macros combo
        for (const auto& macro : m_comboMacros) {
            if (!macro.enabled) continue;

            static std::map<std::wstring, bool> comboKeyStates;
            bool isPressed = m_hotkeyManager.IsKeyPressed(macro.hotkey);

            if (isPressed && !comboKeyStates[macro.hotkey]) {
                m_macroExecutor.ExecuteComboMacro(macro);
            }
            comboKeyStates[macro.hotkey] = isPressed;
        }

        // Image detection (à implémenter)
        // TODO: Ajouter la détection d'image en utilisant OpenCV

        Sleep(10); // Éviter une utilisation CPU excessive
    }
}


bool MainWindow::Create(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COLOR_BG);
    wc.lpszClassName = L"MacroFlowWindowClass";

    if (!RegisterClassExW(&wc)) {
        return false;
    }

    m_hwnd = CreateWindowExW(
        0,
        L"MacroFlowWindowClass",
        L"MacroFlow - Advanced Macro Manager",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1200, 700,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    if (!m_hwnd) {
        return false;
    }

    m_fontTitle = CreateFontW(32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    m_fontNormal = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    m_fontSmall = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    m_fontBold = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    CreateControls();

    // NOUVEAU: Charger les macros et démarrer le monitoring
    LoadMacros();
    StartHotkeyMonitoring();

    return true;
}

void MainWindow::Show(int nCmdShow) {
    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    }
    else {
        pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void MainWindow::DrawCategoryButtons(HDC hdc) {
    RECT btn1 = { 16, 100, 244, 144 };
    HRGN rgn1 = CreateRoundRectRgn(btn1.left, btn1.top, btn1.right, btn1.bottom, 12, 12);
    HBRUSH brush1 = CreateSolidBrush((m_currentCategory == MacroCategory::BASIC) ?
        RGB(22, 163, 116) : RGB(55, 65, 81));
    FillRgn(hdc, rgn1, brush1);
    DeleteObject(brush1);
    DeleteObject(rgn1);

    SetBkMode(hdc, TRANSPARENT);
    if (m_currentCategory == MacroCategory::BASIC) {
        SelectObject(hdc, m_fontBold);
        SetTextColor(hdc, RGB(255, 255, 255));
    } else {
        SelectObject(hdc, m_fontNormal);
        SetTextColor(hdc, RGB(156, 163, 175));
    }
    DrawTextW(hdc, L"⚡ Basic Macros", -1, &btn1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT btn2 = { 16, 154, 244, 198 };
    HRGN rgn2 = CreateRoundRectRgn(btn2.left, btn2.top, btn2.right, btn2.bottom, 12, 12);
    HBRUSH brush2 = CreateSolidBrush((m_currentCategory == MacroCategory::IMAGE) ?
        RGB(29, 78, 216) : RGB(55, 65, 81));
    FillRgn(hdc, rgn2, brush2);
    DeleteObject(brush2);
    DeleteObject(rgn2);

    if (m_currentCategory == MacroCategory::IMAGE) {
        SelectObject(hdc, m_fontBold);
        SetTextColor(hdc, RGB(255, 255, 255));
    } else {
        SelectObject(hdc, m_fontNormal);
        SetTextColor(hdc, RGB(156, 163, 175));
    }
    DrawTextW(hdc, L"🖼️ Image Detection", -1, &btn2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    RECT btn3 = { 16, 208, 244, 252 };
    HRGN rgn3 = CreateRoundRectRgn(btn3.left, btn3.top, btn3.right, btn3.bottom, 12, 12);
    HBRUSH brush3 = CreateSolidBrush((m_currentCategory == MacroCategory::COMBO) ?
        RGB(126, 34, 206) : RGB(55, 65, 81));
    FillRgn(hdc, rgn3, brush3);
    DeleteObject(brush3);
    DeleteObject(rgn3);

    if (m_currentCategory == MacroCategory::COMBO) {
        SelectObject(hdc, m_fontBold);
        SetTextColor(hdc, RGB(255, 255, 255));
    } else {
        SelectObject(hdc, m_fontNormal);
        SetTextColor(hdc, RGB(156, 163, 175));
    }
    DrawTextW(hdc, L"🎯 Combo Macros", -1, &btn3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            StopHotkeyMonitoring();
            SaveMacros();
            PostQuitMessage(0);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);

            RECT clientRect;
            GetClientRect(m_hwnd, &clientRect);

            HBRUSH bgBrush = CreateSolidBrush(COLOR_BG);
            FillRect(hdc, &clientRect, bgBrush);
            DeleteObject(bgBrush);

            RECT sidebarRect = { 0, 0, 260, clientRect.bottom };
            PaintSidebar(hdc, sidebarRect);

            DrawCategoryButtons(hdc);

            RECT contentRect = { 260, 0, clientRect.right, clientRect.bottom };
            PaintContent(hdc, contentRect);

            EndPaint(m_hwnd, &ps);
            return 0;
        }

case WM_LBUTTONDOWN: {
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    // Gestion des boutons de catégories (déjà existant)
    if (x >= 16 && x <= 244) {
        if (y >= 100 && y <= 144) {
            SwitchCategory(MacroCategory::BASIC);
            InvalidateRect(m_hwnd, nullptr, TRUE);
        } else if (y >= 154 && y <= 198) {
            SwitchCategory(MacroCategory::IMAGE);
            InvalidateRect(m_hwnd, nullptr, TRUE);
        } else if (y >= 208 && y <= 252) {
            SwitchCategory(MacroCategory::COMBO);
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }

    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);

    // Bouton "New Macro" (déjà existant)
    if (x >= clientRect.right - 180 && x <= clientRect.right - 30 && y >= 30 && y <= 70) {
        OnAddMacro();
    }

    // NOUVEAU : Gestion des clics sur les cartes de macros
    int yPos = 120 - m_scrollPos;
    int cardHeight = (m_currentCategory == MacroCategory::COMBO) ? 140 : 110;

    size_t macroCount = 0;
    switch (m_currentCategory) {
        case MacroCategory::BASIC: macroCount = m_basicMacros.size(); break;
        case MacroCategory::IMAGE: macroCount = m_imageMacros.size(); break;
        case MacroCategory::COMBO: macroCount = m_comboMacros.size(); break;
    }

    for (size_t i = 0; i < macroCount; i++) {
        RECT cardRect = { 290, yPos, 1170, yPos + 100 };

        if (x >= cardRect.left && x <= cardRect.right &&
            y >= cardRect.top && y <= cardRect.bottom) {

            // Clic sur le bouton Status (à droite)
            RECT statusBtnRect = { cardRect.left + 360, yPos + 34, cardRect.left + 460, yPos + 66 };
            if (x >= statusBtnRect.left && x <= statusBtnRect.right &&
                y >= statusBtnRect.top && y <= statusBtnRect.bottom) {
                OnToggleStatus((int)i);
                return 0;
            }

            // Clic ailleurs sur la carte = éditer
            OnEditMacro((int)i);
            return 0;
        }

        yPos += cardHeight;
    }

    return 0;
}

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            m_scrollPos -= delta / 4;
            if (m_scrollPos < 0) m_scrollPos = 0;
            InvalidateRect(m_hwnd, nullptr, TRUE);
            return 0;
        }

        case WM_SIZE: {
            InvalidateRect(m_hwnd, nullptr, TRUE);
            return 0;
        }
        case WM_RBUTTONDOWN: {
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    int yPos = 120 - m_scrollPos;
    int cardHeight = (m_currentCategory == MacroCategory::COMBO) ? 140 : 110;

    size_t macroCount = 0;
    switch (m_currentCategory) {
        case MacroCategory::BASIC: macroCount = m_basicMacros.size(); break;
        case MacroCategory::IMAGE: macroCount = m_imageMacros.size(); break;
        case MacroCategory::COMBO: macroCount = m_comboMacros.size(); break;
    }

    for (size_t i = 0; i < macroCount; i++) {
        RECT cardRect = { 290, yPos, 1170, yPos + 100 };

        if (x >= cardRect.left && x <= cardRect.right &&
            y >= cardRect.top && y <= cardRect.bottom) {

            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 1, L"✏️ Edit");
            AppendMenuW(hMenu, MF_STRING, 2, L"🗑️ Delete");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(hMenu, MF_STRING, 3, L"❌ Cancel");

            POINT pt = { x, y };
            ClientToScreen(m_hwnd, &pt);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN,
                                    pt.x, pt.y, 0, m_hwnd, nullptr);

            if (cmd == 1) OnEditMacro((int)i);
            else if (cmd == 2) OnDeleteMacro((int)i);

            DestroyMenu(hMenu);
            return 0;
        }

        yPos += cardHeight;
    }

    return 0;
}
    }

    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

void MainWindow::CreateControls() {
    RefreshMacroList();
}

void MainWindow::SwitchCategory(MacroCategory category) {
    m_currentCategory = category;
    m_scrollPos = 0;
    RefreshMacroList();
}

void MainWindow::RefreshMacroList() {
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void MainWindow::PaintSidebar(HDC hdc, RECT& rect) {
    HBRUSH sidebarBrush = CreateSolidBrush(COLOR_SIDEBAR);
    FillRect(hdc, &rect, sidebarBrush);
    DeleteObject(sidebarBrush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, m_fontTitle);

    RECT logoRect = { rect.left + 16, 20, rect.right, 60 };
    DrawTextW(hdc, L"MacroFlow", -1, &logoRect, DT_LEFT | DT_TOP);

    SelectObject(hdc, m_fontSmall);
    SetTextColor(hdc, COLOR_TEXT_GRAY);
    RECT subRect = { rect.left + 16, 58, rect.right, 80 };
    DrawTextW(hdc, L"Advanced Macro Manager", -1, &subRect, DT_LEFT | DT_TOP);

    RECT statusRect = { rect.left + 16, rect.bottom - 100, rect.right - 16, rect.bottom - 20 };
    HRGN hRgn = CreateRoundRectRgn(statusRect.left, statusRect.top, statusRect.right, statusRect.bottom, 15, 15);
    HBRUSH statusBrush = CreateSolidBrush(RGB(55, 65, 81));
    FillRgn(hdc, hRgn, statusBrush);

    HPEN statusPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
    SelectObject(hdc, statusPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, statusRect.left, statusRect.top, statusRect.right, statusRect.bottom, 15, 15);

    DeleteObject(statusBrush);
    DeleteObject(statusPen);
    DeleteObject(hRgn);

    SetTextColor(hdc, COLOR_TEXT_GRAY);
    RECT statusLabelRect = { statusRect.left + 12, statusRect.top + 12, statusRect.right, statusRect.top + 30 };
    DrawTextW(hdc, L"STATUS", -1, &statusLabelRect, DT_LEFT | DT_TOP);

    HBRUSH greenBrush = CreateSolidBrush(COLOR_GREEN);
    SelectObject(hdc, greenBrush);
    SelectObject(hdc, GetStockObject(NULL_PEN));
    Ellipse(hdc, statusRect.left + 12, statusRect.top + 40, statusRect.left + 22, statusRect.top + 50);
    DeleteObject(greenBrush);

    SetTextColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, m_fontNormal);
    RECT activeRect = { statusRect.left + 28, statusRect.top + 38, statusRect.right, statusRect.top + 60 };
    DrawTextW(hdc, L"System Active", -1, &activeRect, DT_LEFT | DT_TOP);
}

void MainWindow::PaintContent(HDC hdc, RECT& rect) {
    RECT headerRect = { rect.left, 0, rect.right, 100 };
    HBRUSH headerBrush = CreateSolidBrush(COLOR_SIDEBAR);
    FillRect(hdc, &headerRect, headerBrush);
    DeleteObject(headerBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
    SelectObject(hdc, borderPen);
    MoveToEx(hdc, headerRect.left, headerRect.bottom, nullptr);
    LineTo(hdc, headerRect.right, headerRect.bottom);
    DeleteObject(borderPen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, m_fontTitle);

    const wchar_t* title = L"Basic Macros";
    const wchar_t* subtitle = L"Create and manage basic keyboard/mouse macros";

    switch (m_currentCategory) {
        case MacroCategory::IMAGE:
            title = L"Image Detection";
            subtitle = L"Detect images on screen and trigger actions";
            break;
        case MacroCategory::COMBO:
            title = L"Combo Macros";
            subtitle = L"Auto-execute game combos with cooldown detection";
            break;
    }

    RECT titleRect = { rect.left + 30, 20, rect.right - 200, 50 };
    DrawTextW(hdc, title, -1, &titleRect, DT_LEFT | DT_TOP);

    SelectObject(hdc, m_fontSmall);
    SetTextColor(hdc, COLOR_TEXT_GRAY);
    RECT subtitleRect = { rect.left + 30, 58, rect.right - 200, 80 };
    DrawTextW(hdc, subtitle, -1, &subtitleRect, DT_LEFT | DT_TOP);

    RECT btnRect = { rect.right - 180, 30, rect.right - 30, 70 };
    HRGN btnRgn = CreateRoundRectRgn(btnRect.left, btnRect.top, btnRect.right, btnRect.bottom, 12, 12);
    HBRUSH btnBrush = CreateSolidBrush(COLOR_GREEN);
    FillRgn(hdc, btnRgn, btnBrush);
    DeleteObject(btnBrush);
    DeleteObject(btnRgn);

    SetTextColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, m_fontNormal);
    DrawTextW(hdc, L"✨ New Macro", -1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    int yPos = 120 - m_scrollPos;

    switch (m_currentCategory) {
        case MacroCategory::BASIC:
            for (size_t i = 0; i < m_basicMacros.size(); i++) {
                PaintMacroCard(hdc, rect.left + 30, yPos, (int)i);
                yPos += 110;
            }
            break;
        case MacroCategory::IMAGE:
            for (size_t i = 0; i < m_imageMacros.size(); i++) {
                PaintMacroCard(hdc, rect.left + 30, yPos, (int)i);
                yPos += 110;
            }
            break;
        case MacroCategory::COMBO:
            for (size_t i = 0; i < m_comboMacros.size(); i++) {
                PaintMacroCard(hdc, rect.left + 30, yPos, (int)i);
                yPos += 140;
            }
            break;
    }

    if ((m_currentCategory == MacroCategory::BASIC && m_basicMacros.empty()) ||
        (m_currentCategory == MacroCategory::IMAGE && m_imageMacros.empty()) ||
        (m_currentCategory == MacroCategory::COMBO && m_comboMacros.empty())) {

        SetTextColor(hdc, COLOR_TEXT_GRAY);
        SelectObject(hdc, m_fontNormal);
        RECT emptyRect = { rect.left, rect.top + 200, rect.right, rect.top + 300 };
        DrawTextW(hdc, L"No macros yet\n\nClick 'New Macro' to get started!", -1, &emptyRect,
            DT_CENTER | DT_VCENTER);
    }
}

void MainWindow::PaintMacroCard(HDC hdc, int x, int y, int index) {
    RECT cardRect = { x, y, x + 880, y + 100 };

    HRGN cardRgn = CreateRoundRectRgn(cardRect.left, cardRect.top, cardRect.right, cardRect.bottom, 15, 15);
    HBRUSH cardBrush = CreateSolidBrush(COLOR_CARD);
    FillRgn(hdc, cardRgn, cardBrush);

    HPEN cardPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
    SelectObject(hdc, cardPen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, cardRect.left, cardRect.top, cardRect.right, cardRect.bottom, 15, 15);

    DeleteObject(cardBrush);
    DeleteObject(cardPen);
    DeleteObject(cardRgn);

    COLORREF iconColor = COLOR_GREEN;
    if (m_currentCategory == MacroCategory::IMAGE) iconColor = COLOR_BLUE;
    if (m_currentCategory == MacroCategory::COMBO) iconColor = COLOR_PURPLE;

    RECT iconRect = { x + 16, y + 24, x + 68, y + 76 };
    HRGN iconRgn = CreateRoundRectRgn(iconRect.left, iconRect.top, iconRect.right, iconRect.bottom, 12, 12);
    HBRUSH iconBrush = CreateSolidBrush(iconColor);
    FillRgn(hdc, iconRgn, iconBrush);
    DeleteObject(iconBrush);
    DeleteObject(iconRgn);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, m_fontTitle);
    const wchar_t* emoji = L"⚡";
    if (m_currentCategory == MacroCategory::IMAGE) emoji = L"🖼️";
    if (m_currentCategory == MacroCategory::COMBO) emoji = L"🎯";
    DrawTextW(hdc, emoji, -1, &iconRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, m_fontNormal);

    std::wstring name = L"Macro " + std::to_wstring(index + 1);
    if (m_currentCategory == MacroCategory::BASIC && index < (int)m_basicMacros.size()) {
        name = m_basicMacros[index].name;
    } else if (m_currentCategory == MacroCategory::IMAGE && index < (int)m_imageMacros.size()) {
        name = m_imageMacros[index].name;
    } else if (m_currentCategory == MacroCategory::COMBO && index < (int)m_comboMacros.size()) {
        name = m_comboMacros[index].name;
    }

    RECT nameRect = { x + 84, y + 24, x + 500, y + 50 };
    DrawTextW(hdc, name.c_str(), -1, &nameRect, DT_LEFT | DT_TOP);

    SetTextColor(hdc, COLOR_TEXT_GRAY);
    SelectObject(hdc, m_fontSmall);
    RECT infoRect = { x + 84, y + 52, x + 500, y + 76 };
    DrawTextW(hdc, L"Click to edit • Right-click for options", -1, &infoRect, DT_LEFT | DT_TOP);

    RECT statusBtnRect = { x + 650, y + 34, x + 750, y + 66 };
    HRGN statusRgn = CreateRoundRectRgn(statusBtnRect.left, statusBtnRect.top, statusBtnRect.right, statusBtnRect.bottom, 8, 8);
    HBRUSH statusBrush = CreateSolidBrush(COLOR_GREEN);
    FillRgn(hdc, statusRgn, statusBrush);
    DeleteObject(statusBrush);
    DeleteObject(statusRgn);

    SetTextColor(hdc, RGB(255, 255, 255));
    SelectObject(hdc, m_fontNormal);
    DrawTextW(hdc, L"Active", -1, &statusBtnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void MainWindow::OnAddMacro() {
    switch (m_currentCategory) {
    case MacroCategory::BASIC:
        ShowBasicMacroDialog();
        break;
    case MacroCategory::IMAGE:
        ShowImageMacroDialog();
        break;
    case MacroCategory::COMBO:
        ShowComboMacroDialog();
        break;
    }
}

void MainWindow::OnEditMacro(int index) {
    switch (m_currentCategory) {
    case MacroCategory::BASIC:
        if (index >= 0 && index < (int)m_basicMacros.size())
            ShowBasicMacroDialog(index);
        break;
    case MacroCategory::IMAGE:
        if (index >= 0 && index < (int)m_imageMacros.size())
            ShowImageMacroDialog(index);
        break;
    case MacroCategory::COMBO:
        if (index >= 0 && index < (int)m_comboMacros.size())
            ShowComboMacroDialog(index);
        break;
    }
}

void MainWindow::OnDeleteMacro(int index) {
    int result = MessageBoxW(m_hwnd,
        L"Are you sure you want to delete this macro?",
        L"Confirm Delete",
        MB_YESNO | MB_ICONQUESTION);

    if (result == IDYES) {
        switch (m_currentCategory) {
        case MacroCategory::BASIC:
            if (index >= 0 && index < (int)m_basicMacros.size())
                m_basicMacros.erase(m_basicMacros.begin() + index);
            break;
        case MacroCategory::IMAGE:
            if (index >= 0 && index < (int)m_imageMacros.size())
                m_imageMacros.erase(m_imageMacros.begin() + index);
            break;
        case MacroCategory::COMBO:
            if (index >= 0 && index < (int)m_comboMacros.size())
                m_comboMacros.erase(m_comboMacros.begin() + index);
            break;
        default:
            break;
        }

        // Sauvegarder après suppression
        SaveMacros();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }
}

void MainWindow::OnToggleStatus(int index) {
    switch (m_currentCategory) {
    case MacroCategory::BASIC:
        if (index >= 0 && index < (int)m_basicMacros.size())
            m_basicMacros[index].enabled = !m_basicMacros[index].enabled;
        break;
    case MacroCategory::IMAGE:
        if (index >= 0 && index < (int)m_imageMacros.size())
            m_imageMacros[index].enabled = !m_imageMacros[index].enabled;
        break;
    case MacroCategory::COMBO:
        if (index >= 0 && index < (int)m_comboMacros.size())
            m_comboMacros[index].enabled = !m_comboMacros[index].enabled;
        break;
    }
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

// ============= BASIC MACRO DIALOG - VERSION CORRIGÉE =============
void MainWindow::ShowBasicMacroDialog(int editIndex) {
    DialogData* data = new DialogData();
    data->pMainWindow = this;
    data->editIndex = editIndex;

    if (editIndex >= 0 && editIndex < (int)m_basicMacros.size()) {
        data->basicMacro = &m_basicMacros[editIndex];
    } else {
        data->basicMacro = new BasicMacro();
        data->basicMacro->name = L"New Basic Macro";
        data->basicMacro->hotkey = L"F1";
        data->basicMacro->enabled = true;
        data->basicMacro->loop = false;
        data->basicMacro->holdMode = false;
    }

    // Créer la fenêtre de dialogue
    HWND hwndDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"#32770",
        L"⚡ Basic Macro Configuration",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
        0, 0, 600, 620,
        m_hwnd, nullptr, m_hInstance, nullptr
    );

    // Centrer la fenêtre
    RECT rcParent, rcDlg;
    GetWindowRect(m_hwnd, &rcParent);
    GetWindowRect(hwndDlg, &rcDlg);
    int x = rcParent.left + (rcParent.right - rcParent.left - 600) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - 620) / 2;
    SetWindowPos(hwndDlg, HWND_TOP, x, y, 600, 620, SWP_SHOWWINDOW);

    // Associer les données au dialogue
    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)data);

    HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    int leftMargin = 30;
    int controlWidth = 540;

    // Titre
    HWND hTitle = CreateWindowW(L"STATIC", L"⚡ Basic Macro Configuration",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        leftMargin, 20, controlWidth, 35, hwndDlg, nullptr, m_hInstance, nullptr);
    SendMessage(hTitle, WM_SETFONT, (WPARAM)m_fontTitle, TRUE);

    // Nom
    CreateWindowW(L"STATIC", L"Macro Name:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 70, 200, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    HWND hEditName = CreateWindowW(L"EDIT", data->basicMacro->name.c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        leftMargin, 95, controlWidth, 30, hwndDlg, (HMENU)ID_EDIT_NAME,
        m_hInstance, nullptr);
    SendMessage(hEditName, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Hotkey
    CreateWindowW(L"STATIC", L"Trigger Hotkey:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 140, 200, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    HWND hEditHotkey = CreateWindowW(L"EDIT", data->basicMacro->hotkey.c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        leftMargin, 165, 200, 30, hwndDlg, (HMENU)ID_EDIT_HOTKEY,
        m_hInstance, nullptr);
    SendMessage(hEditHotkey, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Options Loop et Hold Mode
    #define ID_CHECK_LOOP 2020
    #define ID_CHECK_HOLD 2021

    HWND hCheckLoop = CreateWindowW(L"BUTTON", L"🔄 Loop (Execute repeatedly)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        leftMargin + 220, 165, 320, 30, hwndDlg, (HMENU)ID_CHECK_LOOP,
        m_hInstance, nullptr);
    SendMessage(hCheckLoop, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hCheckLoop, BM_SETCHECK, data->basicMacro->loop ? BST_CHECKED : BST_UNCHECKED, 0);

    HWND hCheckHold = CreateWindowW(L"BUTTON", L"⏸️ Hold Mode (While key pressed)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        leftMargin, 205, 400, 30, hwndDlg, (HMENU)ID_CHECK_HOLD,
        m_hInstance, nullptr);
    SendMessage(hCheckHold, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hCheckHold, BM_SETCHECK, data->basicMacro->holdMode ? BST_CHECKED : BST_UNCHECKED, 0);

    // Actions
    CreateWindowW(L"STATIC", L"Actions List:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 250, 200, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    HWND hListActions = CreateWindowW(L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        leftMargin, 275, 420, 130, hwndDlg, (HMENU)ID_LIST_ACTIONS,
        m_hInstance, nullptr);
    SendMessage(hListActions, WM_SETFONT, (WPARAM)hFont, TRUE);

    for (const auto& action : data->basicMacro->actions) {
        SendMessageW(hListActions, LB_ADDSTRING, 0, (LPARAM)action.c_str());
    }

    HWND hBtnAdd = CreateWindowW(L"BUTTON", L"➕ Add",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        460, 275, 110, 30, hwndDlg, (HMENU)ID_BTN_ADD_ACTION,
        m_hInstance, nullptr);
    SendMessage(hBtnAdd, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hBtnRemove = CreateWindowW(L"BUTTON", L"🗑️ Remove",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        460, 315, 110, 30, hwndDlg, (HMENU)ID_BTN_REMOVE_ACTION,
        m_hInstance, nullptr);
    SendMessage(hBtnRemove, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Menu déroulant pour choisir le type d'action
    #define ID_COMBO_ACTION_TYPE 2022
    #define ID_EDIT_ACTION_KEY 2023

    CreateWindowW(L"STATIC", L"Add Action:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 420, 200, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    HWND hComboType = CreateWindowW(L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        leftMargin, 445, 250, 200, hwndDlg, (HMENU)ID_COMBO_ACTION_TYPE,
        m_hInstance, nullptr);
    SendMessage(hComboType, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageW(hComboType, CB_ADDSTRING, 0, (LPARAM)L"Press Key");
    SendMessageW(hComboType, CB_ADDSTRING, 0, (LPARAM)L"Click Left");
    SendMessageW(hComboType, CB_ADDSTRING, 0, (LPARAM)L"Click Right");
    SendMessageW(hComboType, CB_ADDSTRING, 0, (LPARAM)L"Click Middle");
    SendMessageW(hComboType, CB_ADDSTRING, 0, (LPARAM)L"Click XButton1 (Mouse4)");
    SendMessageW(hComboType, CB_ADDSTRING, 0, (LPARAM)L"Click XButton2 (Mouse5)");
    SendMessageW(hComboType, CB_ADDSTRING, 0, (LPARAM)L"Wait (ms)");
    SendMessage(hComboType, CB_SETCURSEL, 0, 0);

    HWND hEditActionKey = CreateWindowW(L"EDIT", L"Q",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        295, 445, 155, 30, hwndDlg, (HMENU)ID_EDIT_ACTION_KEY,
        m_hInstance, nullptr);
    SendMessage(hEditActionKey, WM_SETFONT, (WPARAM)hFont, TRUE);

    // Boutons Bas
    HWND hBtnSave = CreateWindowW(L"BUTTON", L"💾 Save Macro",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        leftMargin, 500, 250, 40, hwndDlg, (HMENU)ID_BTN_SAVE,
        m_hInstance, nullptr);
    SendMessage(hBtnSave, WM_SETFONT, (WPARAM)m_fontNormal, TRUE);

    HWND hBtnCancel = CreateWindowW(L"BUTTON", L"❌ Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        295, 500, 275, 40, hwndDlg, (HMENU)ID_BTN_CANCEL,
        m_hInstance, nullptr);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)m_fontNormal, TRUE);

    ShowWindow(hwndDlg, SW_SHOW);
    EnableWindow(m_hwnd, FALSE);

    // CORRECTION MAJEURE : Boucle de messages correcte
    MSG msg;
    bool dialogActive = true;

    while (dialogActive) {
        BOOL result = GetMessage(&msg, nullptr, 0, 0);

        if (result == 0 || result == -1) {
            // WM_QUIT reçu ou erreur
            dialogActive = false;
            break;
        }

        // Traiter les messages de la boîte de dialogue
        if (msg.hwnd == hwndDlg || IsChild(hwndDlg, msg.hwnd)) {

            if (msg.message == WM_COMMAND) {
                int wmId = LOWORD(msg.wParam);
                int wmEvent = HIWORD(msg.wParam);

                if (wmId == ID_BTN_ADD_ACTION) {
                    int typeIndex = (int)SendMessage(hComboType, CB_GETCURSEL, 0, 0);
                    wchar_t keyText[256];
                    GetWindowTextW(hEditActionKey, keyText, 256);

                    std::wstring action;
                    switch (typeIndex) {
                        case 0: action = L"Press " + std::wstring(keyText); break;
                        case 1: action = L"Click Left"; break;
                        case 2: action = L"Click Right"; break;
                        case 3: action = L"Click Middle"; break;
                        case 4: action = L"Click XButton1"; break;
                        case 5: action = L"Click XButton2"; break;
                        case 6: action = L"Wait " + std::wstring(keyText) + L"ms"; break;
                    }

                    data->basicMacro->actions.push_back(action);
                    SendMessageW(hListActions, LB_ADDSTRING, 0, (LPARAM)action.c_str());
                    continue; // Ne pas dispatcher ce message

                } else if (wmId == ID_BTN_REMOVE_ACTION) {
                    int sel = (int)SendMessage(hListActions, LB_GETCURSEL, 0, 0);
                    if (sel != LB_ERR && sel >= 0 && sel < (int)data->basicMacro->actions.size()) {
                        data->basicMacro->actions.erase(data->basicMacro->actions.begin() + sel);
                        SendMessage(hListActions, LB_DELETESTRING, sel, 0);
                    }
                    continue;

                } else if (wmId == ID_BTN_SAVE) {
                    // Récupérer les valeurs
                    wchar_t name[256], hotkey[64];
                    GetWindowTextW(hEditName, name, 256);
                    GetWindowTextW(hEditHotkey, hotkey, 64);

                    data->basicMacro->name = name;
                    data->basicMacro->hotkey = hotkey;
                    data->basicMacro->loop = (SendMessage(hCheckLoop, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    data->basicMacro->holdMode = (SendMessage(hCheckHold, BM_GETCHECK, 0, 0) == BST_CHECKED);

                    // Ajouter à la liste si c'est une nouvelle macro
                    if (editIndex == -1) {
                        m_basicMacros.push_back(*data->basicMacro);
                        delete data->basicMacro;
                        data->basicMacro = nullptr;
                    }

                    // Sauvegarder dans le JSON temporaire
                    SaveMacros();

                    // Redémarrer le monitoring
                    StopHotkeyMonitoring();
                    StartHotkeyMonitoring();

                    // Fermer le dialogue
                    dialogActive = false;
                    DestroyWindow(hwndDlg);
                    EnableWindow(m_hwnd, TRUE);
                    SetForegroundWindow(m_hwnd);
                    InvalidateRect(m_hwnd, nullptr, TRUE);
                    continue;

                } else if (wmId == ID_BTN_CANCEL) {
                    if (editIndex == -1 && data->basicMacro) {
                        delete data->basicMacro;
                        data->basicMacro = nullptr;
                    }
                    dialogActive = false;
                    DestroyWindow(hwndDlg);
                    EnableWindow(m_hwnd, TRUE);
                    SetForegroundWindow(m_hwnd);
                    continue;
                }
            }
            else if (msg.message == WM_CLOSE) {
                if (editIndex == -1 && data->basicMacro) {
                    delete data->basicMacro;
                    data->basicMacro = nullptr;
                }
                dialogActive = false;
                DestroyWindow(hwndDlg);
                EnableWindow(m_hwnd, TRUE);
                SetForegroundWindow(m_hwnd);
                continue;
            }
        }

        // Traiter les messages standards
        if (!IsDialogMessage(hwndDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // Nettoyage
    DeleteObject(hFont);
    delete data;
}

// ============= COMBO MACRO DIALOG - VERSION CORRIGÉE =============
void MainWindow::ShowComboMacroDialog(int editIndex) {
    DialogData* data = new DialogData();
    data->pMainWindow = this;
    data->editIndex = editIndex;

    if (editIndex >= 0 && editIndex < (int)m_comboMacros.size()) {
        data->comboMacro = &m_comboMacros[editIndex];
    } else {
        data->comboMacro = new ComboMacro();
        data->comboMacro->name = L"New Combo Macro";
        data->comboMacro->hotkey = L"F5";
        data->comboMacro->delayBetween = 200;
        data->comboMacro->detectCooldown = true;
        data->comboMacro->enabled = true;
    }

    HWND hwndDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"#32770",
        L"🎯 Combo Macro Configuration",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
        0, 0, 600, 650,
        m_hwnd, nullptr, m_hInstance, nullptr
    );

    RECT rcParent;
    GetWindowRect(m_hwnd, &rcParent);
    int x = rcParent.left + (rcParent.right - rcParent.left - 600) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - 650) / 2;
    SetWindowPos(hwndDlg, HWND_TOP, x, y, 600, 650, SWP_SHOWWINDOW);

    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)data);

    HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    int leftMargin = 30;
    int controlWidth = 540;

    HWND hTitle = CreateWindowW(L"STATIC", L"🎯 Combo Macro Configuration",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        leftMargin, 20, controlWidth, 35, hwndDlg, nullptr, m_hInstance, nullptr);
    SendMessage(hTitle, WM_SETFONT, (WPARAM)m_fontTitle, TRUE);

    CreateWindowW(L"STATIC", L"Combo Name:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 70, 200, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    HWND hEditName = CreateWindowW(L"EDIT", data->comboMacro->name.c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        leftMargin, 95, controlWidth, 30, hwndDlg, (HMENU)ID_EDIT_NAME,
        m_hInstance, nullptr);
    SendMessage(hEditName, WM_SETFONT, (WPARAM)hFont, TRUE);

    CreateWindowW(L"STATIC", L"Trigger Hotkey:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 140, 200, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    HWND hEditHotkey = CreateWindowW(L"EDIT", data->comboMacro->hotkey.c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        leftMargin, 165, 200, 30, hwndDlg, (HMENU)ID_EDIT_HOTKEY,
        m_hInstance, nullptr);
    SendMessage(hEditHotkey, WM_SETFONT, (WPARAM)hFont, TRUE);

    CreateWindowW(L"STATIC", L"Skills in Combo:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 210, 200, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    HWND hListSkills = CreateWindowW(L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        leftMargin, 235, 420, 120, hwndDlg, (HMENU)ID_LIST_SKILLS,
        m_hInstance, nullptr);
    SendMessage(hListSkills, WM_SETFONT, (WPARAM)hFont, TRUE);

    for (const auto& skill : data->comboMacro->skills) {
        SendMessageW(hListSkills, LB_ADDSTRING, 0, (LPARAM)skill.c_str());
    }

    HWND hBtnAddSkill = CreateWindowW(L"BUTTON", L"➕ Add",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        460, 235, 110, 30, hwndDlg, (HMENU)ID_BTN_ADD_SKILL,
        m_hInstance, nullptr);
    SendMessage(hBtnAddSkill, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hBtnRemoveSkill = CreateWindowW(L"BUTTON", L"🗑️ Remove",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        460, 275, 110, 30, hwndDlg, (HMENU)ID_BTN_REMOVE_SKILL,
        m_hInstance, nullptr);
    SendMessage(hBtnRemoveSkill, WM_SETFONT, (WPARAM)hFont, TRUE);

    CreateWindowW(L"STATIC", L"Delay Between Skills (ms):",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 370, 250, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    wchar_t delayText[32];
    swprintf_s(delayText, L"%d", data->comboMacro->delayBetween);
    HWND hEditDelay = CreateWindowW(L"EDIT", delayText,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
        leftMargin, 395, 150, 30, hwndDlg, (HMENU)ID_EDIT_DELAY,
        m_hInstance, nullptr);
    SendMessage(hEditDelay, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hCheckCooldown = CreateWindowW(L"BUTTON", L"Detect Cooldowns Automatically",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        leftMargin, 440, 400, 30, hwndDlg, (HMENU)ID_CHECK_COOLDOWN,
        m_hInstance, nullptr);
    SendMessage(hCheckCooldown, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hCheckCooldown, BM_SETCHECK, data->comboMacro->detectCooldown ? BST_CHECKED : BST_UNCHECKED, 0);

    HWND hBtnSave = CreateWindowW(L"BUTTON", L"💾 Save Combo",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        leftMargin, 490, 250, 40, hwndDlg, (HMENU)ID_BTN_SAVE,
        m_hInstance, nullptr);
    SendMessage(hBtnSave, WM_SETFONT, (WPARAM)m_fontNormal, TRUE);

    HWND hBtnCancel = CreateWindowW(L"BUTTON", L"❌ Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        295, 490, 275, 40, hwndDlg, (HMENU)ID_BTN_CANCEL,
        m_hInstance, nullptr);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)m_fontNormal, TRUE);

    ShowWindow(hwndDlg, SW_SHOW);
    EnableWindow(m_hwnd, FALSE);

    MSG msg;
    bool dialogActive = true;

    while (dialogActive) {
        BOOL result = GetMessage(&msg, nullptr, 0, 0);

        if (result == 0 || result == -1) {
            dialogActive = false;
            break;
        }

        if (msg.hwnd == hwndDlg || IsChild(hwndDlg, msg.hwnd)) {
            if (msg.message == WM_COMMAND) {
                int wmId = LOWORD(msg.wParam);

                if (wmId == ID_BTN_ADD_SKILL) {
                    wchar_t skill[256] = L"Q - New Skill";
                    data->comboMacro->skills.push_back(skill);
                    SendMessageW(hListSkills, LB_ADDSTRING, 0, (LPARAM)skill);
                    continue;

                } else if (wmId == ID_BTN_REMOVE_SKILL) {
                    int sel = (int)SendMessage(hListSkills, LB_GETCURSEL, 0, 0);
                    if (sel != LB_ERR && sel >= 0 && sel < (int)data->comboMacro->skills.size()) {
                        data->comboMacro->skills.erase(data->comboMacro->skills.begin() + sel);
                        SendMessage(hListSkills, LB_DELETESTRING, sel, 0);
                    }
                    continue;

                } else if (wmId == ID_BTN_SAVE) {
                    wchar_t name[256], hotkey[64], delayText[32];
                    GetWindowTextW(hEditName, name, 256);
                    GetWindowTextW(hEditHotkey, hotkey, 64);
                    GetWindowTextW(hEditDelay, delayText, 32);

                    data->comboMacro->name = name;
                    data->comboMacro->hotkey = hotkey;
                    data->comboMacro->delayBetween = _wtoi(delayText);
                    data->comboMacro->detectCooldown = (SendMessage(hCheckCooldown, BM_GETCHECK, 0, 0) == BST_CHECKED);

                    if (editIndex == -1) {
                        m_comboMacros.push_back(*data->comboMacro);
                        delete data->comboMacro;
                        data->comboMacro = nullptr;
                    }

                    SaveMacros();
                    StopHotkeyMonitoring();
                    StartHotkeyMonitoring();

                    dialogActive = false;
                    DestroyWindow(hwndDlg);
                    EnableWindow(m_hwnd, TRUE);
                    SetForegroundWindow(m_hwnd);
                    InvalidateRect(m_hwnd, nullptr, TRUE);
                    continue;

                } else if (wmId == ID_BTN_CANCEL) {
                    if (editIndex == -1 && data->comboMacro) {
                        delete data->comboMacro;
                        data->comboMacro = nullptr;
                    }
                    dialogActive = false;
                    DestroyWindow(hwndDlg);
                    EnableWindow(m_hwnd, TRUE);
                    SetForegroundWindow(m_hwnd);
                    continue;
                }
            } else if (msg.message == WM_CLOSE) {
                if (editIndex == -1 && data->comboMacro) {
                    delete data->comboMacro;
                    data->comboMacro = nullptr;
                }
                dialogActive = false;
                DestroyWindow(hwndDlg);
                EnableWindow(m_hwnd, TRUE);
                SetForegroundWindow(m_hwnd);
                continue;
            }
        }

        if (!IsDialogMessage(hwndDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DeleteObject(hFont);
    delete data;
}

// ============= IMAGE MACRO DIALOG =============
void MainWindow::ShowImageMacroDialog(int editIndex) {
    DialogData* data = new DialogData();
    data->pMainWindow = this;
    data->editIndex = editIndex;

    if (editIndex >= 0 && editIndex < (int)m_imageMacros.size()) {
        data->imageMacro = &m_imageMacros[editIndex];
    } else {
        data->imageMacro = new ImageMacro();
        data->imageMacro->name = L"New Image Macro";
        data->imageMacro->imagePath = L"";
        data->imageMacro->action = L"Press Q";
        data->imageMacro->confidence = 85;
        data->imageMacro->enabled = true;
    }

    HWND hwndDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"#32770",
        L"🖼️ Image Detection Macro",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
        0, 0, 600, 500,
        m_hwnd, nullptr, m_hInstance, nullptr
    );

    RECT rcParent;
    GetWindowRect(m_hwnd, &rcParent);
    int x = rcParent.left + (rcParent.right - rcParent.left - 600) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - 500) / 2;
    SetWindowPos(hwndDlg, HWND_TOP, x, y, 600, 500, SWP_SHOWWINDOW);

    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM)data);

    HFONT hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    int leftMargin = 30;
    int controlWidth = 540;

    HWND hTitle = CreateWindowW(L"STATIC", L"🖼️ Image Detection Macro",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        leftMargin, 20, controlWidth, 35, hwndDlg, nullptr, m_hInstance, nullptr);
    SendMessage(hTitle, WM_SETFONT, (WPARAM)m_fontTitle, TRUE);

    CreateWindowW(L"STATIC", L"Macro Name:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 70, 200, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    HWND hEditName = CreateWindowW(L"EDIT", data->imageMacro->name.c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        leftMargin, 95, controlWidth, 30, hwndDlg, (HMENU)ID_EDIT_NAME,
        m_hInstance, nullptr);
    SendMessage(hEditName, WM_SETFONT, (WPARAM)hFont, TRUE);

    CreateWindowW(L"STATIC", L"Image to Detect:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 140, 200, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    HWND hEditPath = CreateWindowW(L"EDIT", data->imageMacro->imagePath.c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
        leftMargin, 165, 420, 30, hwndDlg, (HMENU)ID_EDIT_IMAGE_PATH,
        m_hInstance, nullptr);
    SendMessage(hEditPath, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hBtnBrowse = CreateWindowW(L"BUTTON", L"📁",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        460, 165, 110, 30, hwndDlg, (HMENU)ID_BTN_BROWSE,
        m_hInstance, nullptr);
    SendMessage(hBtnBrowse, WM_SETFONT, (WPARAM)hFont, TRUE);

    CreateWindowW(L"STATIC", L"Action to Execute:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 210, 200, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    HWND hEditAction = CreateWindowW(L"EDIT", data->imageMacro->action.c_str(),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        leftMargin, 235, controlWidth, 30, hwndDlg, (HMENU)ID_EDIT_ACTION,
        m_hInstance, nullptr);
    SendMessage(hEditAction, WM_SETFONT, (WPARAM)hFont, TRUE);

    CreateWindowW(L"STATIC", L"Detection Confidence:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        leftMargin, 280, 200, 20, hwndDlg, nullptr, m_hInstance, nullptr);

    wchar_t confText[32];
    swprintf_s(confText, L"%d%%", data->imageMacro->confidence);
    HWND hLabelConf = CreateWindowW(L"STATIC", confText,
        WS_CHILD | WS_VISIBLE | SS_RIGHT,
        470, 280, 100, 20, hwndDlg, (HMENU)ID_LABEL_CONFIDENCE,
        m_hInstance, nullptr);
    SendMessage(hLabelConf, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hSlider = CreateWindowW(TRACKBAR_CLASSW, nullptr,
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
        leftMargin, 305, controlWidth, 30, hwndDlg, (HMENU)ID_SLIDER_CONFIDENCE,
        m_hInstance, nullptr);
    SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(50, 100));
    SendMessage(hSlider, TBM_SETPOS, TRUE, data->imageMacro->confidence);

    HWND hBtnSave = CreateWindowW(L"BUTTON", L"💾 Save Macro",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        leftMargin, 360, 250, 40, hwndDlg, (HMENU)ID_BTN_SAVE,
        m_hInstance, nullptr);
    SendMessage(hBtnSave, WM_SETFONT, (WPARAM)m_fontNormal, TRUE);

    HWND hBtnCancel = CreateWindowW(L"BUTTON", L"❌ Cancel",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        295, 360, 275, 40, hwndDlg, (HMENU)ID_BTN_CANCEL,
        m_hInstance, nullptr);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)m_fontNormal, TRUE);

    ShowWindow(hwndDlg, SW_SHOW);
    EnableWindow(m_hwnd, FALSE);

    MSG msg;
    bool dialogActive = true;

    while (dialogActive && GetMessage(&msg, nullptr, 0, 0)) {
        if (msg.hwnd == hwndDlg || IsChild(hwndDlg, msg.hwnd)) {
            if (msg.message == WM_COMMAND) {
                int wmId = LOWORD(msg.wParam);

                if (wmId == ID_BTN_BROWSE) {
                    OPENFILENAMEW ofn = {};
                    wchar_t fileName[MAX_PATH] = L"";
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwndDlg;
                    ofn.lpstrFilter = L"Images\0*.png;*.jpg;*.bmp\0All\0*.*\0";
                    ofn.lpstrFile = fileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

                    if (GetOpenFileNameW(&ofn)) {
                        SetWindowTextW(hEditPath, fileName);
                    }

                } else if (wmId == ID_BTN_SAVE) {
                    wchar_t name[256], path[MAX_PATH], action[256];
                    GetWindowTextW(hEditName, name, 256);
                    GetWindowTextW(hEditPath, path, MAX_PATH);
                    GetWindowTextW(hEditAction, action, 256);
                    int confidence = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);

                    data->imageMacro->name = name;
                    data->imageMacro->imagePath = path;
                    data->imageMacro->action = action;
                    data->imageMacro->confidence = confidence;

                    if (editIndex == -1) {
                        m_imageMacros.push_back(*data->imageMacro);
                        delete data->imageMacro;
                    }

                    SaveMacros();

                    dialogActive = false;
                    DestroyWindow(hwndDlg);
                    EnableWindow(m_hwnd, TRUE);
                    SetForegroundWindow(m_hwnd);
                    InvalidateRect(m_hwnd, nullptr, TRUE);

                } else if (wmId == ID_BTN_CANCEL) {
                    if (editIndex == -1) delete data->imageMacro;
                    dialogActive = false;
                    DestroyWindow(hwndDlg);
                    EnableWindow(m_hwnd, TRUE);
                    SetForegroundWindow(m_hwnd);
                }
            } else if (msg.message == WM_HSCROLL && (HWND)msg.lParam == hSlider) {
                int pos = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
                wchar_t text[32];
                swprintf_s(text, L"%d%%", pos);
                SetWindowTextW(hLabelConf, text);
            } else if (msg.message == WM_CLOSE || msg.message == WM_DESTROY) {
                if (editIndex == -1 && data->imageMacro) delete data->imageMacro;
                dialogActive = false;
                DestroyWindow(hwndDlg);
                EnableWindow(m_hwnd, TRUE);
                SetForegroundWindow(m_hwnd);
            }
        }

        if (!IsDialogMessage(hwndDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DeleteObject(hFont);
    delete data;
}
