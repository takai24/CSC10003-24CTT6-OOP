#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawRect(const SvgRect &rect)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, rect.transformAttribute);
    Pen pen(rect.strokeColor, rect.strokeWidth);
    
    RectF bounds(rect.x, rect.y, rect.w, rect.h);
    auto brush = CreateFillBrush(rect.fillUrl, rect.fillColor, rect.fillOpacity, bounds);
    
    if (brush)
        graphics.FillRectangle(brush.get(), rect.x, rect.y, rect.w, rect.h);
    graphics.DrawRectangle(&pen, rect.x, rect.y, rect.w, rect.h);
    graphics.Restore(state);
}
