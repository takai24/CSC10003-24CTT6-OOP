#pragma once
#include <gdiplus.h>
#include <memory>
#include "SvgGradient.h"

class GdiPlusGradientRenderer
{
public:
    static std::unique_ptr<Gdiplus::Brush> CreateBrush(const std::shared_ptr<SvgGradient> &gradient, const Gdiplus::RectF &bounds, float opacity = 1.0f);
};
