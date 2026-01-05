#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawEllipse(const SvgEllipse& ellipse)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, ellipse.transformAttribute);

    float x = ellipse.cx - ellipse.rx;
    float y = ellipse.cy - ellipse.ry;
    float w = ellipse.rx * 2.0f;
    float h = ellipse.ry * 2.0f;
    RectF bounds(x, y, w, h);

    Brush* brush = CreateBrush(ellipse.fillAttributeString, ellipse.fillColor, bounds);

    if (brush)
    {
        graphics.FillEllipse(brush, bounds);
        delete brush;
    }

    if (ellipse.strokeWidth > 0 && ellipse.strokeColor.GetAlpha() > 0)
    {
        Pen pen(ellipse.strokeColor, ellipse.strokeWidth);
        graphics.DrawEllipse(&pen, bounds);
    }

    graphics.Restore(state);
}