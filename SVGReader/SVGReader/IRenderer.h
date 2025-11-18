#pragma once

#include "stdafx.h"
#include "SvgElement.h"

class IRenderer
{
public:
    virtual ~IRenderer() {}

    virtual void DrawLine(const SvgLine &line) = 0;
    virtual void DrawRect(const SvgRect &rect) = 0;
    virtual void DrawCircle(const SvgCircle &circle) = 0;
    virtual void DrawEllipse(const SvgEllipse &ellipse) = 0;
    virtual void DrawPolyline(const SvgPolyline &polyline) = 0;
    virtual void DrawPolygon(const SvgPolygon &polygon) = 0;
    virtual void DrawText(const SvgText &text) = 0;
};
