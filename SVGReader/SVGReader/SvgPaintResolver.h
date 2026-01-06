#pragma once
#include <gdiplus.h>
#include <memory>
#include "SvgGradient.h"

// Responsible for creating GDI+ Brushes from SVG Gradients.
// Uses Texture Mapping for Radial Gradients to support complex transforms.
class SvgPaintResolver
{
public:
    static std::unique_ptr<Gdiplus::Brush> CreateBrush(
        const std::shared_ptr<SvgGradient> &gradient, 
        const Gdiplus::RectF &bounds, 
        float opacity = 1.0f);
};
