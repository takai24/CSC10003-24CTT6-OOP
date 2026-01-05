#pragma once
#include <string>
#include <vector>
#include <gdiplus.h> 

struct SvgGradientStop
{
    float offset;
    Gdiplus::Color color; 

    SvgGradientStop(float off, Gdiplus::Color col)
        : offset(off), color(col) {
    }
};