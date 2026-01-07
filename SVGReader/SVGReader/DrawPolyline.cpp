#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawPolyline(const SvgPolyline& polyline)
{
    if (polyline.points.empty()) return;

    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, polyline.transformAttribute);

    float minX = polyline.points[0].X;
    float maxX = polyline.points[0].X;
    float minY = polyline.points[0].Y;
    float maxY = polyline.points[0].Y;

    for (const auto& p : polyline.points) {
        if (p.X < minX) minX = p.X;
        if (p.X > maxX) maxX = p.X;
        if (p.Y < minY) minY = p.Y;
        if (p.Y > maxY) maxY = p.Y;
    }
    RectF bounds(minX, minY, maxX - minX, maxY - minY);

    if (polyline.fillColor.GetAlpha() > 0 || !polyline.fillAttributeString.empty()) {
        auto bptr = CreateFillBrush(polyline.fillAttributeString, polyline.fillColor, 1.0f, bounds);
        Brush* brush = bptr ? bptr.get() : nullptr;
        if (brush) {
            graphics.FillPolygon(brush, polyline.points.data(), (INT)polyline.points.size());
        }
    }

    if (polyline.strokeWidth > 0 && polyline.strokeColor.GetAlpha() > 0)
    {
        Pen pen(polyline.strokeColor, polyline.strokeWidth);
        pen.SetLineJoin(LineJoinRound);
        pen.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        graphics.DrawLines(&pen, polyline.points.data(), (INT)polyline.points.size());
    }

    graphics.Restore(state);
}