#pragma once
#include "SvgGradient.h"

class SvgRadialGradient : public SvgGradient
{
public:
    float cx = 0.5f;
    float cy = 0.5f;
    float r = 0.5f;
    float fx = 0.5f;
    float fy = 0.5f;
};