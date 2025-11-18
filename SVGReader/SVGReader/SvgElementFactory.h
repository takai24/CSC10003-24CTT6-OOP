#pragma once

#include "stdafx.h"
#include "SvgElement.h"
#include "IXMLNode.h"

class SvgElementFactory
{
public:
    std::unique_ptr<ISvgElement> CreateElement(const IXMLNode &node) const;

private:
    Gdiplus::Color ParseColor(const std::string &value) const;
    std::vector<Gdiplus::PointF> ParsePoints(const std::string &ptsStr) const;
};
