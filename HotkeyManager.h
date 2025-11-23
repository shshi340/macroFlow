#pragma once
#include <windows.h>
#include <string>
#include <map>

// Classe pour gérer les hotkeys
class HotkeyManager {
public:
    HotkeyManager();
    ~HotkeyManager();

    // Enregistrer un hotkey
    bool RegisterHotkey(int id, const std::wstring& hotkey, bool holdMode);

    // Désenregistrer un hotkey
    void UnregisterHotkey(int id);

    // Vérifier si une touche est pressée/maintenue
    bool IsKeyPressed(const std::wstring& key);
    bool IsKeyHeld(const std::wstring& key);

    // Convertir une string en virtual key code
    int GetVirtualKeyCode(const std::wstring& key);

private:
    std::map<int, UINT> m_registeredHotkeys;
    std::map<std::wstring, int> m_keyMapping;

    void InitializeKeyMapping();
};
