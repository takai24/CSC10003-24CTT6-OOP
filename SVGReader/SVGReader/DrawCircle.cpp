#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawCircle(const SvgCircle &circle)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, circle.transformAttribute);
    Pen pen(circle.strokeColor, circle.strokeWidth);
    SolidBrush brush(circle.fillColor);
    float d = circle.r * 2.0f;
    graphics.FillEllipse(&brush, circle.cx - circle.r, circle.cy - circle.r, d, d);
    graphics.DrawEllipse(&pen, circle.cx - circle.r, circle.cy - circle.r, d, d);
    graphics.Restore(state);
}
