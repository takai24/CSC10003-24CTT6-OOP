#pragma once
#include <string>
#include <unordered_map>
#include <GdiPlus.h>

class SvgColors
{
public:
    static Gdiplus::Color GetColor(const std::string& name);

private:
    static const std::unordered_map<std::string, Gdiplus::Color> s_colorMap;
};