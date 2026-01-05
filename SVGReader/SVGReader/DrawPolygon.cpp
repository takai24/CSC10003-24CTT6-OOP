#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawPolygon(const SvgPolygon& polygon)
{
    if (polygon.points.empty()) return;

    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, polygon.transformAttribute);

    float minX = polygon.points[0].X;
    float maxX = polygon.points[0].X;
    float minY = polygon.points[0].Y;
    float maxY = polygon.points[0].Y;

    for (const auto& p : polygon.points) {
        if (p.X < minX) minX = p.X;
        if (p.X > maxX) maxX = p.X;
        if (p.Y < minY) minY = p.Y;
        if (p.Y > maxY) maxY = p.Y;
    }
    RectF bounds(minX, minY, maxX - minX, maxY - minY);

    Brush* brush = CreateBrush(polygon.fillAttributeString, polygon.fillColor, bounds);

    if (brush)
    {
        graphics.FillPolygon(brush, polygon.points.data(), (INT)polygon.points.size());
        delete brush;
    }

    if (polygon.strokeWidth > 0 && polygon.strokeColor.GetAlpha() > 0)
    {
        Pen pen(polygon.strokeColor, polygon.strokeWidth);
        pen.SetLineJoin(LineJoinRound);
        pen.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
        graphics.DrawPolygon(&pen, polygon.points.data(), (INT)polygon.points.size());
    }

    graphics.Restore(state);
}