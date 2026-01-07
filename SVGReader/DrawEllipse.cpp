#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawEllipse(const SvgEllipse &e)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, e.transformAttribute);
    Pen pen(e.strokeColor, e.strokeWidth);
    
    float w = e.rx * 2.0f;
    float h = e.ry * 2.0f;
    RectF bounds(e.cx - e.rx, e.cy - e.ry, w, h);
    auto brush = CreateFillBrush(e.fillUrl, e.fillColor, e.fillOpacity, bounds);

    if (brush)
        graphics.FillEllipse(brush.get(), e.cx - e.rx, e.cy - e.ry, e.rx * 2.0f, e.ry * 2.0f);
    graphics.DrawEllipse(&pen, e.cx - e.rx, e.cy - e.ry, e.rx * 2.0f, e.ry * 2.0f);
    graphics.Restore(state);
}
