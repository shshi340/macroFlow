#include "MacroExecutor.h"
#include "MacroManager.h"      // ← Pour les structures BasicMacro, etc.
#include "HotkeyManager.h"     // ← Pour GetVirtualKeyCode
#include <thread>

MacroExecutor::MacroExecutor()
    : m_isExecuting(false)
    , m_executionThread(nullptr)
{
}

MacroExecutor::~MacroExecutor() {
    StopExecution();
}

void MacroExecutor::ExecuteBasicMacro(const BasicMacro& macro) {
    if (m_isExecuting) return;

    m_isExecuting = true;

    // Créer un thread pour l'exécution
    std::thread([this, macro]() {
        do {
            // Exécuter toutes les actions
            for (const auto& action : macro.actions) {
                if (!m_isExecuting) break;
                ExecuteAction(action);
                Sleep(50); // Petit délai entre les actions
            }

            // Si mode loop, ajouter un délai avant de recommencer
            if (macro.loop && m_isExecuting) {
                Sleep(100); // Délai entre les boucles
            }
        } while (macro.loop && m_isExecuting);

        m_isExecuting = false;
    }).detach();
}

void MacroExecutor::ExecuteComboMacro(const ComboMacro& macro) {
    if (m_isExecuting) return;

    m_isExecuting = true;

    std::thread([this, macro]() {
        // Exécuter chaque skill avec délai
        for (const auto& skill : macro.skills) {
            if (!m_isExecuting) break;

            ExecuteAction(skill);

            // Attendre le délai entre les skills
            Sleep(macro.delayBetween);
        }
        m_isExecuting = false;
    }).detach();
}

void MacroExecutor::ExecuteImageMacro(const ImageMacro& macro) {
    // Pour l'instant, juste exécuter l'action
    // TODO: Ajouter la détection d'image avec OpenCV
    ExecuteAction(macro.action);
}

void MacroExecutor::StopExecution() {
    m_isExecuting = false;

    // Attendre un peu pour que le thread se termine
    Sleep(100);
}

void MacroExecutor::ExecuteAction(const std::wstring& action) {
    // Parser l'action pour savoir quoi faire

    if (action.find(L"Click") != std::wstring::npos) {
        // Action de clic souris
        if (action.find(L"Left") != std::wstring::npos) {
            SimulateMouseClick(L"LEFT");
        } else if (action.find(L"Right") != std::wstring::npos) {
            SimulateMouseClick(L"RIGHT");
        }
    }
    else if (action.find(L"Press") != std::wstring::npos) {
        // Action de pression de touche
        // Format: "Press Q" ou "Press F1"
        size_t pos = action.find(L"Press");
        if (pos != std::wstring::npos) {
            std::wstring key = action.substr(pos + 5); // "Press" = 5 caractères
            // Enlever les espaces au début et à la fin
            key.erase(0, key.find_first_not_of(L" \t"));
            key.erase(key.find_last_not_of(L" \t") + 1);

            SimulateKeyPress(key);
        }
    }
    else if (action.find(L"Wait") != std::wstring::npos) {
        // Action d'attente
        // Format: "Wait 500ms" ou "Wait 1000"
        size_t pos = action.find_first_of(L"0123456789");
        if (pos != std::wstring::npos) {
            int delay = _wtoi(action.substr(pos).c_str());
            Sleep(delay);
        }
    }
    else if (action.find(L"Q") != std::wstring::npos ||
             action.find(L"W") != std::wstring::npos ||
             action.find(L"E") != std::wstring::npos ||
             action.find(L"R") != std::wstring::npos) {
        // Pour les combos de jeu, format simple: "Q - Skill name"
        std::wstring key;
        if (action.find(L"Q") != std::wstring::npos) key = L"Q";
        else if (action.find(L"W") != std::wstring::npos) key = L"W";
        else if (action.find(L"E") != std::wstring::npos) key = L"E";
        else if (action.find(L"R") != std::wstring::npos) key = L"R";

        if (!key.empty()) {
            SimulateKeyPress(key);
        }
    }
}

void MacroExecutor::SimulateKeyPress(const std::wstring& key) {
    HotkeyManager hkm;
    int vk = hkm.GetVirtualKeyCode(key);
    if (vk == 0) return;

    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;

    // Key down (appuyer sur la touche)
    SendInput(1, &input, sizeof(INPUT));
    Sleep(50); // Maintenir 50ms

    // Key up (relâcher la touche)
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void MacroExecutor::SimulateKeyDown(const std::wstring& key) {
    HotkeyManager hkm;
    int vk = hkm.GetVirtualKeyCode(key);
    if (vk == 0) return;

    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    SendInput(1, &input, sizeof(INPUT));
}

void MacroExecutor::SimulateKeyUp(const std::wstring& key) {
    HotkeyManager hkm;
    int vk = hkm.GetVirtualKeyCode(key);
    if (vk == 0) return;

    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void MacroExecutor::SimulateMouseClick(const std::wstring& button) {
    INPUT input = {0};
    input.type = INPUT_MOUSE;

    if (button == L"LEFT") {
        // Clic gauche
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        Sleep(50);
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }
    else if (button == L"RIGHT") {
        // Clic droit
        input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        Sleep(50);
        input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        SendInput(1, &input, sizeof(INPUT));
    }
}

void MacroExecutor::SimulateMouseMove(int x, int y) {
    SetCursorPos(x, y);
}
