#include "stdafx.h"
#include "GdiPlusGradientRenderer.h"
#include "SvgPaintResolver.h"

using namespace Gdiplus;

std::unique_ptr<Brush> GdiPlusGradientRenderer::CreateBrush(const std::shared_ptr<SvgGradient> &gradient, const RectF &bounds, float opacity)
{
    // Delegate to the new SvgPaintResolver class
    return SvgPaintResolver::CreateBrush(gradient, bounds, opacity);
}
