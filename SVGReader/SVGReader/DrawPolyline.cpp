#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawPolyline(const SvgPolyline &polyline)
{
    if (polyline.points.size() < 2)
        return;
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, polyline.transformAttribute);
    Pen pen(polyline.strokeColor, polyline.strokeWidth);
    SolidBrush brush(polyline.fillColor);
    graphics.FillPolygon(&brush, polyline.points.data(), static_cast<INT>(polyline.points.size()));
    if (polyline.strokeColor.GetAlpha() > 0)
    {
        graphics.DrawLines(&pen, polyline.points.data(), static_cast<INT>(polyline.points.size()));
    }
    graphics.Restore(state);
}
