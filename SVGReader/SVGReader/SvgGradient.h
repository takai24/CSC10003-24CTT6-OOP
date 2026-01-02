#pragma once
#include <string>
#include <vector>
#include "SvgGradientStop.h"

class SvgGradient
{
public:
    std::string id;
    std::vector<SvgGradientStop> stops;

    virtual ~SvgGradient() = default;
};
