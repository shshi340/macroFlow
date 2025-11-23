#include "HotkeyManager.h"
#include <algorithm>

HotkeyManager::HotkeyManager() {
    InitializeKeyMapping();
}

HotkeyManager::~HotkeyManager() {
    // Désenregistrer tous les hotkeys
    for (auto& pair : m_registeredHotkeys) {
        UnregisterHotKey(nullptr, pair.first);
    }
}

void HotkeyManager::InitializeKeyMapping() {
    // Touches F1-F12
    m_keyMapping[L"F1"] = VK_F1;
    m_keyMapping[L"F2"] = VK_F2;
    m_keyMapping[L"F3"] = VK_F3;
    m_keyMapping[L"F4"] = VK_F4;
    m_keyMapping[L"F5"] = VK_F5;
    m_keyMapping[L"F6"] = VK_F6;
    m_keyMapping[L"F7"] = VK_F7;
    m_keyMapping[L"F8"] = VK_F8;
    m_keyMapping[L"F9"] = VK_F9;
    m_keyMapping[L"F10"] = VK_F10;
    m_keyMapping[L"F11"] = VK_F11;
    m_keyMapping[L"F12"] = VK_F12;

    // Lettres A-Z
    for (wchar_t c = L'A'; c <= L'Z'; c++) {
        m_keyMapping[std::wstring(1, c)] = c;
    }

    // Chiffres 0-9
    for (wchar_t c = L'0'; c <= L'9'; c++) {
        m_keyMapping[std::wstring(1, c)] = c;
    }

    // Touches spéciales
    m_keyMapping[L"SPACE"] = VK_SPACE;
    m_keyMapping[L"ENTER"] = VK_RETURN;
    m_keyMapping[L"TAB"] = VK_TAB;
    m_keyMapping[L"ESC"] = VK_ESCAPE;
    m_keyMapping[L"SHIFT"] = VK_SHIFT;
    m_keyMapping[L"CTRL"] = VK_CONTROL;
    m_keyMapping[L"ALT"] = VK_MENU;
    m_keyMapping[L"LEFT"] = VK_LEFT;
    m_keyMapping[L"RIGHT"] = VK_RIGHT;
    m_keyMapping[L"UP"] = VK_UP;
    m_keyMapping[L"DOWN"] = VK_DOWN;

    // Boutons souris
    m_keyMapping[L"XBUTTON1"] = VK_XBUTTON1;
    m_keyMapping[L"XBUTTON2"] = VK_XBUTTON2;
    m_keyMapping[L"MOUSE4"] = VK_XBUTTON1;  // Alias
    m_keyMapping[L"MOUSE5"] = VK_XBUTTON2;  // Alias
    m_keyMapping[L"LBUTTON"] = VK_LBUTTON;
    m_keyMapping[L"RBUTTON"] = VK_RBUTTON;
    m_keyMapping[L"MBUTTON"] = VK_MBUTTON;  // Bouton du milieu

    // Pavé numérique
    m_keyMapping[L"NUMPAD0"] = VK_NUMPAD0;
    m_keyMapping[L"NUMPAD1"] = VK_NUMPAD1;
    m_keyMapping[L"NUMPAD2"] = VK_NUMPAD2;
    m_keyMapping[L"NUMPAD3"] = VK_NUMPAD3;
    m_keyMapping[L"NUMPAD4"] = VK_NUMPAD4;
    m_keyMapping[L"NUMPAD5"] = VK_NUMPAD5;
    m_keyMapping[L"NUMPAD6"] = VK_NUMPAD6;
    m_keyMapping[L"NUMPAD7"] = VK_NUMPAD7;
    m_keyMapping[L"NUMPAD8"] = VK_NUMPAD8;
    m_keyMapping[L"NUMPAD9"] = VK_NUMPAD9;
}

int HotkeyManager::GetVirtualKeyCode(const std::wstring& key) {
    // Convertir en majuscules
    std::wstring upperKey = key;
    std::transform(upperKey.begin(), upperKey.end(), upperKey.begin(), ::towupper);

    // Chercher dans la map
    auto it = m_keyMapping.find(upperKey);
    if (it != m_keyMapping.end()) {
        return it->second;
    }
    return 0;
}

bool HotkeyManager::RegisterHotkey(int id, const std::wstring& hotkey, bool holdMode) {
    int vk = GetVirtualKeyCode(hotkey);
    if (vk == 0) return false;

    // Enregistrer le hotkey global
    if (RegisterHotKey(nullptr, id, 0, vk)) {
        m_registeredHotkeys[id] = vk;
        return true;
    }
    return false;
}

void HotkeyManager::UnregisterHotkey(int id) {
    auto it = m_registeredHotkeys.find(id);
    if (it != m_registeredHotkeys.end()) {
        UnregisterHotKey(nullptr, id);
        m_registeredHotkeys.erase(it);
    }
}

bool HotkeyManager::IsKeyPressed(const std::wstring& key) {
    int vk = GetVirtualKeyCode(key);
    if (vk == 0) return false;

    // GetAsyncKeyState retourne un short avec le bit le plus significatif à 1 si la touche est pressée
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool HotkeyManager::IsKeyHeld(const std::wstring& key) {
    // Même chose que IsKeyPressed pour notre usage
    return IsKeyPressed(key);
}
