#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawLine(const SvgLine &line)
{
    Pen pen(line.strokeColor, line.strokeWidth);
    graphics.DrawLine(&pen, line.x1, line.y1, line.x2, line.y2);
}

void GdiPlusRenderer::DrawRect(const SvgRect &rect)
{
    Pen pen(rect.strokeColor, rect.strokeWidth);
    SolidBrush brush(rect.fillColor);
    graphics.FillRectangle(&brush, rect.x, rect.y, rect.w, rect.h);
    graphics.DrawRectangle(&pen, rect.x, rect.y, rect.w, rect.h);
}

void GdiPlusRenderer::DrawCircle(const SvgCircle &circle)
{
    Pen pen(circle.strokeColor, circle.strokeWidth);
    SolidBrush brush(circle.fillColor);
    float d = circle.r * 2.0f;
    graphics.FillEllipse(&brush, circle.cx - circle.r, circle.cy - circle.r, d, d);
    graphics.DrawEllipse(&pen, circle.cx - circle.r, circle.cy - circle.r, d, d);
}

void GdiPlusRenderer::DrawEllipse(const SvgEllipse &e)
{
    Pen pen(e.strokeColor, e.strokeWidth);
    SolidBrush brush(e.fillColor);
    graphics.FillEllipse(&brush, e.cx - e.rx, e.cy - e.ry, e.rx * 2.0f, e.ry * 2.0f);
    graphics.DrawEllipse(&pen, e.cx - e.rx, e.cy - e.ry, e.rx * 2.0f, e.ry * 2.0f);
}

void GdiPlusRenderer::DrawPolyline(const SvgPolyline &polyline)
{
    if (polyline.points.size() < 2)
        return;
    Pen pen(polyline.strokeColor, polyline.strokeWidth);
    graphics.DrawLines(&pen, polyline.points.data(), static_cast<INT>(polyline.points.size()));
}

void GdiPlusRenderer::DrawPolygon(const SvgPolygon &polygon)
{
    if (polygon.points.size() < 3)
        return;
    Pen pen(polygon.strokeColor, polygon.strokeWidth);
    SolidBrush brush(polygon.fillColor);
    graphics.FillPolygon(&brush, polygon.points.data(), static_cast<INT>(polygon.points.size()));
    graphics.DrawPolygon(&pen, polygon.points.data(), static_cast<INT>(polygon.points.size()));
}

void GdiPlusRenderer::DrawText(const SvgText &text)
{
    FontFamily fontFamily(text.fontFamily.c_str());
    Gdiplus::Font font(&fontFamily, text.fontSize, FontStyleRegular, UnitPixel);
    SolidBrush brush(text.fillColor);
    graphics.DrawString(text.text.c_str(), -1, &font, PointF(text.x, text.y), &brush);
}
