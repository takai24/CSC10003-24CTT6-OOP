#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawCircle(const SvgCircle& circle)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, circle.transformAttribute);
    Pen pen(circle.strokeColor, circle.strokeWidth);

    float d = circle.r * 2.0f;
    RectF bounds(circle.cx - circle.r, circle.cy - circle.r, d, d);
    auto brush = CreateFillBrush(circle.fillUrl, circle.fillColor, circle.fillOpacity, bounds);

    if (brush)
        graphics.FillEllipse(brush.get(), circle.cx - circle.r, circle.cy - circle.r, d, d);
    graphics.DrawEllipse(&pen, circle.cx - circle.r, circle.cy - circle.r, d, d);
    graphics.Restore(state);
}
