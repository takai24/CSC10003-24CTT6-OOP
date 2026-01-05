#include "stdafx.h"
#include "GdiPlusRenderer.h"
#include "SvgLinearGradient.h"
#include "SvgRadialGradient.h"

// This compilation unit intentionally left minimal: implementations for
// DrawX methods and ApplyTransform are provided in separate Draw*.cpp and
// ApplyTransform.cpp files to keep the code modular.

static std::string GetIdFromUrl(const std::string& url) {
    size_t s = url.find("url(#");
    if (s == std::string::npos) return "";
    size_t e = url.find(")", s);
    if (e == std::string::npos) return "";
    return url.substr(s + 5, e - (s + 5));
}

Matrix* GdiPlusRenderer::ParseMatrix(const std::string& transformStr) {
    auto matrix = new Matrix(); 

    if (transformStr.empty()) return matrix;

    std::regex re(R"(([a-z]+)\s*\(([^)]+)\))");
    std::smatch match;
    std::string::const_iterator searchStart(transformStr.cbegin());

    while (std::regex_search(searchStart, transformStr.cend(), match, re)) {
        std::string type = match[1]; 
        std::string args = match[2]; 

        std::replace(args.begin(), args.end(), ',', ' ');
        std::stringstream ss(args);
        std::vector<float> values;
        float val;
        while (ss >> val) values.push_back(val);

        if (type == "translate" && values.size() >= 1) {
            float tx = values[0];
            float ty = (values.size() > 1) ? values[1] : 0.0f;
            matrix->Translate(tx, ty);
        }
        else if (type == "rotate" && values.size() >= 1) {
            float angle = values[0];
            if (values.size() >= 3) {
                float cx = values[1];
                float cy = values[2];
                matrix->RotateAt(angle, PointF(cx, cy));
            }
            else {
                matrix->Rotate(angle);
            }
        }
        else if (type == "scale" && values.size() >= 1) {
            float sx = values[0];
            float sy = (values.size() > 1) ? values[1] : sx; 
            matrix->Scale(sx, sy);
        }
        else if (type == "matrix" && values.size() == 6) {
            // SVG matrix(a, b, c, d, e, f)
            // GDI+ Matrix(m11, m12, m21, m22, dx, dy)
            // Mapping: a=m11, b=m12, c=m21, d=m22, e=dx, f=dy
            Matrix m(values[0], values[1], values[2], values[3], values[4], values[5]);
            matrix->Multiply(&m);
        }
        else if (type == "skewX" && values.size() >= 1) {
            float angle = values[0];
            matrix->Shear(tan(angle * 3.14159f / 180.0f), 0.0f);
        }
        else if (type == "skewY" && values.size() >= 1) {
            float angle = values[0];
            matrix->Shear(0.0f, tan(angle * 3.14159f / 180.0f));
        }

        searchStart = match.suffix().first;
    }

    return matrix;
}

LinearGradientBrush* GdiPlusRenderer::CreateLinearBrush(const SvgLinearGradient* grad, const RectF& bounds) {
    PointF p1, p2;
    if (grad->gradientUnits == "userSpaceOnUse") {
        p1 = PointF(grad->x1, grad->y1); p2 = PointF(grad->x2, grad->y2);
    }
    else {
        p1 = PointF(bounds.X + grad->x1 * bounds.Width, bounds.Y + grad->y1 * bounds.Height);
        p2 = PointF(bounds.X + grad->x2 * bounds.Width, bounds.Y + grad->y2 * bounds.Height);
    }
    if (abs(p1.X - p2.X) < 0.001f && abs(p1.Y - p2.Y) < 0.001f) p2.X += 0.1f;

    auto brush = new LinearGradientBrush(p1, p2, Color::Black, Color::White);
    int count = grad->stops.size();
    if (count > 0) {
        std::vector<Color> colors; std::vector<REAL> positions;
        for (const auto& s : grad->stops) { colors.push_back(s.color); positions.push_back(s.offset); }
        brush->SetInterpolationColors(colors.data(), positions.data(), count);
    }
    if (!grad->gradientTransform.empty()) {
        // Parse transform gradient
    }
    return brush;
}

SolidBrush* GdiPlusRenderer::CreateRadialFallbackBrush(const SvgGradient* grad) {
    if (grad->stops.empty()) return new SolidBrush(Color::Black);
    int r = 0, g = 0, b = 0, a = 0, n = grad->stops.size();
    for (auto& s : grad->stops) { a += s.color.GetA(); r += s.color.GetR(); g += s.color.GetG(); b += s.color.GetB(); }
    return new SolidBrush(Color(a / n, r / n, g / n, b / n));
}

Brush* GdiPlusRenderer::CreateBrush(const std::string& fillAttributeString, Gdiplus::Color fillColor, const RectF& bounds) {
    std::string urlId = GetIdFromUrl(fillAttributeString);

    if (!urlId.empty() && m_gradients) {
        auto it = m_gradients->find(urlId);
        if (it != m_gradients->end()) {
            auto baseGrad = it->second;
            if (auto linGrad = std::dynamic_pointer_cast<SvgLinearGradient>(baseGrad)) {
                return CreateLinearBrush(linGrad.get(), bounds);
            }
            else {
                return CreateRadialFallbackBrush(baseGrad.get());
            }
        }
    }
    return new SolidBrush(fillColor);
}
