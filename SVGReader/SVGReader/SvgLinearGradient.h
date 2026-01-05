#pragma once
#include "SvgGradient.h"

class SvgLinearGradient : public SvgGradient
{
public:
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 1.0f;
    float y2 = 0.0f;
};