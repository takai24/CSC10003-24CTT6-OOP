#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawPolygon(const SvgPolygon& polygon)
{
	if (polygon.points.size() < 3)
		return;
	GraphicsState state = graphics.Save();
	ApplyTransform(graphics, polygon.transformAttribute);
	Pen pen(polygon.strokeColor, polygon.strokeWidth);
	
    // Compute bounds for gradient
    REAL minX = polygon.points[0].X;
    REAL maxX = minX;
    REAL minY = polygon.points[0].Y;
    REAL maxY = minY;
    for (size_t i = 1; i < polygon.points.size(); ++i) {
        if (polygon.points[i].X < minX) minX = polygon.points[i].X;
        if (polygon.points[i].X > maxX) maxX = polygon.points[i].X;
        if (polygon.points[i].Y < minY) minY = polygon.points[i].Y;
        if (polygon.points[i].Y > maxY) maxY = polygon.points[i].Y;
    }
    RectF bounds(minX, minY, maxX - minX, maxY - minY);
    auto brush = CreateFillBrush(polygon.fillUrl, polygon.fillColor, polygon.fillOpacity, bounds);

    if (brush)
	    graphics.FillPolygon(brush.get(), polygon.points.data(), static_cast<INT>(polygon.points.size()));
	graphics.DrawPolygon(&pen, polygon.points.data(), static_cast<INT>(polygon.points.size()));
	graphics.Restore(state);
}
