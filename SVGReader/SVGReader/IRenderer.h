#pragma once

// Forward declarations of SVG element classes to avoid heavy includes
class SvgLine;
class SvgRect;
class SvgCircle;
class SvgEllipse;
class SvgPolyline;
class SvgPolygon;
class SvgText;
class SvgPath;
class SvgGroup;
class SvgGradient;
#include <unordered_map>
#include <memory>
#include <string>

class IRenderer
{
public:
    virtual ~IRenderer() {}

    virtual void SetGradients(const std::unordered_map<std::string, std::shared_ptr<SvgGradient>>& gradients) = 0;

    virtual void DrawLine(const SvgLine &line) = 0;
    virtual void DrawRect(const SvgRect &rect) = 0;
    virtual void DrawCircle(const SvgCircle &circle) = 0;
    virtual void DrawEllipse(const SvgEllipse &ellipse) = 0;
    virtual void DrawPolyline(const SvgPolyline &polyline) = 0;
    virtual void DrawPolygon(const SvgPolygon &polygon) = 0;
    virtual void DrawText(const SvgText &text) = 0;
    virtual void DrawPath(const SvgPath& path) = 0;
    virtual void DrawGroup(const SvgGroup& group) = 0;
};
