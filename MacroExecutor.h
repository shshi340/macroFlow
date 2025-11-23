#pragma once
#include <windows.h>
#include <string>
#include <vector>

// Forward declarations
struct BasicMacro;
struct ImageMacro;
struct ComboMacro;

// Classe pour exécuter les macros
class MacroExecutor {
public:
    MacroExecutor();
    ~MacroExecutor();

    // Exécuter une macro basique
    void ExecuteBasicMacro(const BasicMacro& macro);

    // Exécuter une macro d'image
    void ExecuteImageMacro(const ImageMacro& macro);

    // Exécuter une macro combo
    void ExecuteComboMacro(const ComboMacro& macro);

    // Arrêter l'exécution
    void StopExecution();

    // État d'exécution
    bool IsExecuting() const { return m_isExecuting; }

private:
    bool m_isExecuting;
    HANDLE m_executionThread;

    // Exécuter une action individuelle
    void ExecuteAction(const std::wstring& action);

    // Simuler une touche
    void SimulateKeyPress(const std::wstring& key);
    void SimulateKeyDown(const std::wstring& key);
    void SimulateKeyUp(const std::wstring& key);

    // Simuler la souris
    void SimulateMouseClick(const std::wstring& button);
    void SimulateMouseMove(int x, int y);
};
