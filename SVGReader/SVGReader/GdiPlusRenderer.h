#pragma once

#include <gdiplus.h>
#include <string>
#include <regex>
#include <sstream>
#include <vector>

using namespace Gdiplus;

// Forward declarations of SVG element classes
class SvgLine;
class SvgRect;
class SvgCircle;
class SvgEllipse;
class SvgPolyline;
class SvgPolygon;
class SvgText;
class SvgPath;
class SvgGroup;

#include "IRenderer.h"

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
    void ApplyTransform(Gdiplus::Graphics& graphics, const std::string& transformStr);
    void DrawPath(const SvgPath& path) override;
    void DrawGroup(const SvgGroup& group) override;
private:
    Gdiplus::Graphics &graphics;
};
