#ifndef MACRODATA_H
#define MACRODATA_H

#include <string>
#include <vector>

struct BasicMacro {
    std::wstring name;
    std::wstring hotkey;
    bool enabled;
    std::vector<std::wstring> actions;

    BasicMacro() : enabled(true) {}
};

struct ImageMacro {
    std::wstring name;
    std::wstring imagePath;
    std::wstring action;
    int confidence;
    bool enabled;

    ImageMacro() : confidence(85), enabled(true) {}
};

struct ComboMacro {
    std::wstring name;
    std::wstring hotkey;
    std::vector<std::wstring> skills;
    int delayBetween;
    bool detectCooldown;
    bool enabled;

    ComboMacro() : delayBetween(200), detectCooldown(true), enabled(true) {}
};

enum class MacroCategory {
    BASIC,
    IMAGE,
    COMBO
};

#endif // MACRODATA_H#pragma once
