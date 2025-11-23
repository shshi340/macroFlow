#pragma once
#include <string>
#include <vector>
#include <windows.h>

// Structure pour les macros
struct BasicMacro {
    std::wstring name;
    std::wstring hotkey;
    std::vector<std::wstring> actions;
    bool enabled;
    bool loop; // Exécution en boucle
    bool holdMode; // Maintenir la touche
};

struct ImageMacro {
    std::wstring name;
    std::wstring imagePath;
    std::wstring action;
    int confidence;
    bool enabled;
};

struct ComboMacro {
    std::wstring name;
    std::wstring hotkey;
    std::vector<std::wstring> skills;
    int delayBetween;
    bool detectCooldown;
    bool enabled;
};

// Classe pour gérer la sauvegarde/chargement JSON
class MacroManager {
public:
    MacroManager();
    ~MacroManager();

    // Sauvegarde et chargement
    bool SaveToFile(const std::wstring& filename);
    bool LoadFromFile(const std::wstring& filename);

    // Gestion des macros
    std::vector<BasicMacro> basicMacros;
    std::vector<ImageMacro> imageMacros;
    std::vector<ComboMacro> comboMacros;

private:
    std::wstring WStringToJson(const std::wstring& str);
    std::wstring JsonToWString(const std::wstring& json);
};
