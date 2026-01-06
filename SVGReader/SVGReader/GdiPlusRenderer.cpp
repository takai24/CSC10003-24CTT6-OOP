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

// Minimal color parser for simple fills (used when fill attribute string is present)
static Color ParseColorString(const std::string& value)
{
    if (value.empty()) return Color(0,0,0,0);
    if (value[0] == '#') {
        std::string v = value;
        if (v.size() == 4) {
            // #RGB -> #RRGGBB
            std::string tmp = "#";
            for (int i = 1; i < 4; ++i) { tmp.push_back(v[i]); tmp.push_back(v[i]); }
            v = tmp;
        }
        if (v.size() == 7) {
            try {
                int r = std::stoi(v.substr(1,2), nullptr, 16);
                int g = std::stoi(v.substr(3,2), nullptr, 16);
                int b = std::stoi(v.substr(5,2), nullptr, 16);
                return Color(255, r, g, b);
            } catch(...) { return Color(0,0,0,0); }
        }
        return Color(0,0,0,0);
    }
    if (value.rfind("rgb", 0) == 0) {
        size_t a = value.find('(');
        size_t b = value.find(')');
        if (a != std::string::npos && b != std::string::npos && b > a) {
            std::string content = value.substr(a+1, b-a-1);
            for (char &c : content) if (c == ',') c = ' ';
            std::stringstream ss(content);
            int r=0,g=0,bv=0; ss >> r >> g >> bv;
            return Color(255, (BYTE)r, (BYTE)g, (BYTE)bv);
        }
    }
    return Color(0,0,0,0);
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
    // If gradientUnits is objectBoundingBox (default), treat coords as unit-space and apply gradientTransform in unit space
    if (grad->gradientUnits == "userSpaceOnUse") {
        p1 = PointF(grad->x1, grad->y1); p2 = PointF(grad->x2, grad->y2);
        if (!grad->gradientTransform.empty()) {
            Matrix* m = ParseMatrix(grad->gradientTransform);
            if (m) {
                PointF pts[2] = { p1, p2 };
                m->TransformPoints(pts, 2);
                p1 = pts[0]; p2 = pts[1];
                delete m;
            }
        }
    }
    else {
        // unit-space points
        PointF u1(grad->x1, grad->y1), u2(grad->x2, grad->y2);
        if (!grad->gradientTransform.empty()) {
            Matrix* m = ParseMatrix(grad->gradientTransform);
            if (m) {
                PointF pts[2] = { u1, u2 };
                m->TransformPoints(pts, 2);
                u1 = pts[0]; u2 = pts[1];
                delete m;
            }
        }
        p1 = PointF(bounds.X + u1.X * bounds.Width, bounds.Y + u1.Y * bounds.Height);
        p2 = PointF(bounds.X + u2.X * bounds.Width, bounds.Y + u2.Y * bounds.Height);
    }
    if (abs(p1.X - p2.X) < 0.001f && abs(p1.Y - p2.Y) < 0.001f) p2.X += 0.1f;

    auto brush = new LinearGradientBrush(p1, p2, Color::Black, Color::White);
    int count = grad->stops.size();
    if (count > 0) {
        // sort stops by offset and clamp offsets to [0,1]
        std::vector<std::pair<float, Color>> arr;
        arr.reserve(count);
        for (const auto& s : grad->stops) {
            float off = s.offset;
            if (off < 0.0f) off = 0.0f;
            if (off > 1.0f) off = 1.0f;
            arr.push_back(std::make_pair(off, s.color));
        }
        std::sort(arr.begin(), arr.end(), [](const std::pair<float, Color>& a, const std::pair<float, Color>& b){ return a.first < b.first; });

        // If only one stop, set both linear colors to that color
        if (arr.size() == 1) {
            brush->SetLinearColors(arr.front().second, arr.front().second);
            return brush;
        }

        std::vector<Color> colors; std::vector<REAL> stopsArr;
        for (const auto& p : arr) { colors.push_back(p.second); stopsArr.push_back(p.first); }
        brush->SetInterpolationColors(colors.data(), stopsArr.data(), (INT)colors.size());
    }
    // Any remaining gradientTransform (already applied for unit-space above) for userSpaceOnUse case handled by transforming points.
    return brush;
}

Brush* GdiPlusRenderer::CreateRadialBrush(const SvgRadialGradient* grad, const RectF& bounds) {
    // Determine center and radius in device/user space
    PointF centerUser;
    float rUser = 0.0f;

    if (grad->gradientUnits == "userSpaceOnUse") {
        PointF c(grad->cx, grad->cy);
        PointF pr(grad->cx + grad->r, grad->cy);
        if (!grad->gradientTransform.empty()) {
            Matrix* m = ParseMatrix(grad->gradientTransform);
            if (m) {
                PointF pts[2] = { c, pr };
                m->TransformPoints(pts, 2);
                c = pts[0]; pr = pts[1];
                delete m;
            }
        }
        centerUser = c;
        float dx = pr.X - c.X; float dy = pr.Y - c.Y;
        rUser = sqrtf(dx*dx + dy*dy);
    } else {
        // unit-space coordinates
        PointF uc(grad->cx, grad->cy);
        PointF upr(grad->cx + grad->r, grad->cy);
        if (!grad->gradientTransform.empty()) {
            Matrix* m = ParseMatrix(grad->gradientTransform);
            if (m) {
                PointF pts[2] = { uc, upr };
                m->TransformPoints(pts, 2);
                uc = pts[0]; upr = pts[1];
                delete m;
            }
        }
        // Map to user space using bounding box
        PointF c(bounds.X + uc.X * bounds.Width, bounds.Y + uc.Y * bounds.Height);
        PointF pr(bounds.X + upr.X * bounds.Width, bounds.Y + upr.Y * bounds.Height);
        centerUser = c;
        float dx = pr.X - c.X; float dy = pr.Y - c.Y;
        rUser = sqrtf(dx*dx + dy*dy);
    }

    if (rUser <= 0.0f) return CreateRadialFallbackBrush(grad);

    // Build boundary points approximating the circle centered at centerUser with radius rUser
    const int STEPS = 48;
    std::vector<PointF> pts;
    pts.reserve(STEPS);
    for (int i = 0; i < STEPS; ++i) {
        float theta = (2.0f * 3.14159265f * i) / STEPS;
        float px = centerUser.X + cosf(theta) * rUser;
        float py = centerUser.Y + sinf(theta) * rUser;
        pts.emplace_back(px, py);
    }

    PathGradientBrush* pgb = new PathGradientBrush(pts.data(), (INT)pts.size());

    int count = grad->stops.size();
    if (count > 0) {
        // Prepare sorted stops
        std::vector<std::pair<float, Color>> arr;
        arr.reserve(count);
        for (const auto& s : grad->stops) {
            float off = s.offset; if (off < 0.0f) off = 0.0f; if (off > 1.0f) off = 1.0f;
            arr.emplace_back(off, s.color);
        }
        std::sort(arr.begin(), arr.end(), [](const auto& a, const auto& b){ return a.first < b.first; });

        // Set interpolation colors: first entry should represent center color
        std::vector<Color> colors; std::vector<REAL> positions;
        for (const auto &p : arr) { colors.push_back(p.second); positions.push_back(p.first); }
        // Ensure center color is set to color at offset 0 (or first stop)
        pgb->SetCenterColor(colors.front());
        if (colors.size() >= 2) {
            pgb->SetInterpolationColors(colors.data(), positions.data(), (INT)colors.size());
        }
    }

    return pgb;
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
            else if (auto radGrad = std::dynamic_pointer_cast<SvgRadialGradient>(baseGrad)) {
                return CreateRadialBrush(radGrad.get(), bounds);
            }
            else {
                return CreateRadialFallbackBrush(baseGrad.get());
            }
        }
    }
    return new SolidBrush(fillColor);
}
