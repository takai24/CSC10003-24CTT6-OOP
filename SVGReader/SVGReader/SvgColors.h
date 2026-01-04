
#pragma once
#include <string>
#include <unordered_map>
#include <gdiplus.h>

using namespace Gdiplus;

class SvgColors {
public:
    static void LoadColors(const std::string& filePath);

    static Color GetColor(std::string colorStr, std::string opacityStr = "1.0");

private:
    static std::unordered_map<std::string, Color> s_colorMap;
};