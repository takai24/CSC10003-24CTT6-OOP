#include "stdafx.h"
#include "GdiPlusRenderer.h"
#include "GdiPlusGradientRenderer.h"
#include "SvgGradient.h"
#include <memory>

// This compilation unit intentionally left minimal: implementations for
// DrawX methods and ApplyTransform are provided in separate Draw*.cpp and
// ApplyTransform.cpp files to keep the code modular.

std::unique_ptr<Gdiplus::Brush> GdiPlusRenderer::CreateFillBrush(const std::string& fillUrl, Gdiplus::Color fillColor, float fillOpacity, const Gdiplus::RectF& bounds)
{
    if (!fillUrl.empty())
    {
        auto it = gradients.find(fillUrl);
        if (it != gradients.end())
        {
             return GdiPlusGradientRenderer::CreateBrush(it->second, bounds, fillOpacity);
        }
    }
    // Fallback
    // Ensure fillColor is applied with opacity if not already (SvgElementFactory applies input opacity to color, but maybe we want to double check logic)
    // SvgElementFactory: p->fillColor = ApplyOpacity(ParseColor(...), fillOp);
    // So fillColor ALREADY has fillOp applied. 
    // BUT what about inheritance? If I used 'fillUrl', fillColor was set to "black" (fallback) with opacity applied.
    // If I revert to solid fallback, I use that fillColor.
    return std::make_unique<SolidBrush>(fillColor);
}
