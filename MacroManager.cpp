#include "MacroManager.h"
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>

MacroManager::MacroManager() {}
MacroManager::~MacroManager() {}

std::wstring MacroManager::WStringToJson(const std::wstring& str) {
    std::wstring result = L"\"";
    for (wchar_t c : str) {
        if (c == L'"') result += L"\\\"";
        else if (c == L'\\') result += L"\\\\";
        else if (c == L'\n') result += L"\\n";
        else if (c == L'\r') result += L"\\r";
        else if (c == L'\t') result += L"\\t";
        else result += c;
    }
    result += L"\"";
    return result;
}

// Fonction helper pour convertir wstring en string
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

bool MacroManager::SaveToFile(const std::wstring& filename) {
    // Convertir le nom de fichier en string pour MinGW
    std::string filenameStr = WStringToString(filename);
    std::ofstream file(filenameStr, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    // Écrire le BOM UTF-8 pour que Windows reconnaisse l'encodage
    const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    file.write((const char*)bom, sizeof(bom));

    file << "{\n";

    // Sauvegarder les macros basiques
    file << "  \"basicMacros\": [\n";
    for (size_t i = 0; i < basicMacros.size(); i++) {
        const auto& m = basicMacros[i];
        file << "    {\n";
        file << "      \"name\": " << WStringToString(WStringToJson(m.name)) << ",\n";
        file << "      \"hotkey\": " << WStringToString(WStringToJson(m.hotkey)) << ",\n";
        file << "      \"enabled\": " << (m.enabled ? "true" : "false") << ",\n";
        file << "      \"loop\": " << (m.loop ? "true" : "false") << ",\n";
        file << "      \"holdMode\": " << (m.holdMode ? "true" : "false") << ",\n";
        file << "      \"actions\": [\n";
        for (size_t j = 0; j < m.actions.size(); j++) {
            file << "        " << WStringToString(WStringToJson(m.actions[j]));
            if (j < m.actions.size() - 1) file << ",";
            file << "\n";
        }
        file << "      ]\n";
        file << "    }";
        if (i < basicMacros.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";

    // Sauvegarder les macros d'image
    file << "  \"imageMacros\": [\n";
    for (size_t i = 0; i < imageMacros.size(); i++) {
        const auto& m = imageMacros[i];
        file << "    {\n";
        file << "      \"name\": " << WStringToString(WStringToJson(m.name)) << ",\n";
        file << "      \"imagePath\": " << WStringToString(WStringToJson(m.imagePath)) << ",\n";
        file << "      \"action\": " << WStringToString(WStringToJson(m.action)) << ",\n";
        file << "      \"confidence\": " << m.confidence << ",\n";
        file << "      \"enabled\": " << (m.enabled ? "true" : "false") << "\n";
        file << "    }";
        if (i < imageMacros.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n";

    // Sauvegarder les macros combo
    file << "  \"comboMacros\": [\n";
    for (size_t i = 0; i < comboMacros.size(); i++) {
        const auto& m = comboMacros[i];
        file << "    {\n";
        file << "      \"name\": " << WStringToString(WStringToJson(m.name)) << ",\n";
        file << "      \"hotkey\": " << WStringToString(WStringToJson(m.hotkey)) << ",\n";
        file << "      \"delayBetween\": " << m.delayBetween << ",\n";
        file << "      \"detectCooldown\": " << (m.detectCooldown ? "true" : "false") << ",\n";
        file << "      \"enabled\": " << (m.enabled ? "true" : "false") << ",\n";
        file << "      \"skills\": [\n";
        for (size_t j = 0; j < m.skills.size(); j++) {
            file << "        " << WStringToString(WStringToJson(m.skills[j]));
            if (j < m.skills.size() - 1) file << ",";
            file << "\n";
        }
        file << "      ]\n";
        file << "    }";
        if (i < comboMacros.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ]\n";

    file << "}\n";
    file.close();
    return true;
}

bool MacroManager::LoadFromFile(const std::wstring& filename) {
    // Convertir le nom de fichier en string pour MinGW
    std::string filenameStr = WStringToString(filename);
    std::ifstream file(filenameStr, std::ios::in | std::ios::binary);
    if (!file.is_open()) return false;

    basicMacros.clear();
    imageMacros.clear();
    comboMacros.clear();

    // Pour l'instant, retourne true même si le parsing n'est pas implémenté
    // TODO: Implémenter le parsing JSON complet
    file.close();
    return true;
}
