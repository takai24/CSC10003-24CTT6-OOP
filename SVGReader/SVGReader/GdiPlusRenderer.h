#pragma once

#include "stdafx.h"
#include "IRenderer.h"
#include "SvgElement.h"

class GdiPlusRenderer : public IRenderer
{
public:
    explicit GdiPlusRenderer(Gdiplus::Graphics &g) : graphics(g) {}

    void DrawLine(const SvgLine &line) override;
    void DrawRect(const SvgRect &rect) override;
    void DrawCircle(const SvgCircle &circle) override;
    void DrawEllipse(const SvgEllipse &ellipse) override;
    void DrawPolyline(const SvgPolyline &polyline) override;
    void DrawPolygon(const SvgPolygon &polygon) override;
    void DrawText(const SvgText &text) override;

private:
    Gdiplus::Graphics &graphics;
};
