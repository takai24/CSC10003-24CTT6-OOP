#include "stdafx.h"
#include "GdiPlusRenderer.h"
using namespace Gdiplus;

void GdiPlusRenderer::DrawRect(const SvgRect& rect)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, rect.transformAttribute);

    RectF bounds(rect.x, rect.y, rect.w, rect.h);

    Brush* brush = CreateBrush(rect.fillAttributeString, rect.fillColor, bounds);

    if (brush)
    {
        graphics.FillRectangle(brush, bounds);
        delete brush; 
    }

    if (rect.strokeWidth > 0 && rect.strokeColor.GetAlpha() > 0)
    {
        Pen pen(rect.strokeColor, rect.strokeWidth);
        graphics.DrawRectangle(&pen, bounds);
    }

    graphics.Restore(state);
}