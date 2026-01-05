#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawCircle(const SvgCircle& circle)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, circle.transformAttribute);

    float x = circle.cx - circle.r;
    float y = circle.cy - circle.r;
    float d = circle.r * 2.0f;
    RectF bounds(x, y, d, d);

    Brush* brush = CreateBrush(circle.fillAttributeString, circle.fillColor, bounds);

    if (brush)
    {
        graphics.FillEllipse(brush, bounds);
        delete brush;
    }

    if (circle.strokeWidth > 0 && circle.strokeColor.GetAlpha() > 0)
    {
        Pen pen(circle.strokeColor, circle.strokeWidth);
        graphics.DrawEllipse(&pen, bounds);
    }

    graphics.Restore(state);
}