#pragma once
#include <string>

struct SvgGradientStop
{
    float offset;
    std::string color;
    float opacity;

    SvgGradientStop()
        : offset(0.0f), color("#000000"), opacity(1.0f) {
    }
};
