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
	SolidBrush brush(polygon.fillColor);
	graphics.FillPolygon(&brush, polygon.points.data(), static_cast<INT>(polygon.points.size()));
	graphics.DrawPolygon(&pen, polygon.points.data(), static_cast<INT>(polygon.points.size()));
	graphics.Restore(state);
}
