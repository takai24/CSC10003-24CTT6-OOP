#pragma once
#include <string>
#include <vector>
#include "SvgGradientStop.h"

class SvgGradient
{
public:
    std::string id;
    std::vector<SvgGradientStop> stops;

    std::string gradientUnits = "objectBoundingBox"; 
    std::string gradientTransform;
    std::string href; 

    virtual ~SvgGradient() = default;
};