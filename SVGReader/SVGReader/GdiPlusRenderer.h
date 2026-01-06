#pragma once
#include <gdiplus.h>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include "IRenderer.h"
#include "SvgElement.h"

class SvgGradient;
class SvgLinearGradient;
class SvgRadialGradient;

using namespace Gdiplus;

class GdiPlusRenderer : public IRenderer
{
public:
    explicit GdiPlusRenderer(Gdiplus::Graphics& g) : graphics(g) {}

    void SetGradients(const std::map<std::string, std::shared_ptr<SvgGradient>>* grads) {
        m_gradients = grads;
    }

    void DrawLine(const SvgLine& line) override;
    void DrawRect(const SvgRect& rect) override;
    void DrawCircle(const SvgCircle& circle) override;
    void DrawEllipse(const SvgEllipse& ellipse) override;
    void DrawPolyline(const SvgPolyline& polyline) override;
    void DrawPolygon(const SvgPolygon& polygon) override;
    void DrawText(const SvgText& text) override;
    void DrawPath(const SvgPath& path) override;
    void DrawGroup(const SvgGroup& group) override;

    void ApplyTransform(Gdiplus::Graphics& graphics, const std::string& transformStr);

private:
    Gdiplus::Graphics& graphics;
    const std::map<std::string, std::shared_ptr<SvgGradient>>* m_gradients = nullptr;

    Gdiplus::Brush* CreateBrush(const std::string& fillAttributeString, Gdiplus::Color fillColor, const Gdiplus::RectF& bounds);
    Gdiplus::LinearGradientBrush* CreateLinearBrush(const SvgLinearGradient* grad, const Gdiplus::RectF& bounds);
    Gdiplus::Brush* CreateRadialBrush(const SvgRadialGradient* grad, const Gdiplus::RectF& bounds);
    Gdiplus::SolidBrush* CreateRadialFallbackBrush(const SvgGradient* grad);
    Gdiplus::Matrix* ParseMatrix(const std::string& transformStr);
};