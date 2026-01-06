#include "stdafx.h"
#include "GdiPlusGradientRenderer.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

using namespace Gdiplus;

// Helper: Apply opacity to color
static Color ApplyColorOpacity(Color c, float opacity)
{
    if (opacity < 0.0f) opacity = 0.0f;
    if (opacity > 1.0f) opacity = 1.0f;
    float origA = c.GetAlpha() / 255.0f;
    float finalA = origA * opacity;
    return Color(static_cast<BYTE>(finalA * 255.0f + 0.5f), c.GetR(), c.GetG(), c.GetB());
}

static void ParseTransform(const std::string &xform, Matrix *result)
{
    size_t pos = 0;
    while (pos < xform.size())
    {
        while (pos < xform.size() && isspace(static_cast<unsigned char>(xform[pos])))
            ++pos;
        if (pos >= xform.size())
            break;
        size_t funcStart = pos;
        while (pos < xform.size() && isalpha(static_cast<unsigned char>(xform[pos])))
            ++pos;
        std::string func = xform.substr(funcStart, pos - funcStart);
        while (pos < xform.size() && xform[pos] != '(')
            ++pos;
        if (pos >= xform.size())
            break;
        ++pos;
        size_t closePos = xform.find(')', pos);
        if (closePos == std::string::npos)
            break;
        std::string args = xform.substr(pos, closePos - pos);
        pos = closePos + 1;
        std::replace(args.begin(), args.end(), ',', ' ');
        std::stringstream ss(args);
        std::vector<float> vals;
        float v;
        while (ss >> v)
            vals.push_back(v);

        Gdiplus::Matrix m;
        if (func == "matrix" && vals.size() >= 6)
        {
            m.SetElements(vals[0], vals[1], vals[2], vals[3], vals[4], vals[5]);
        }
        else if (func == "scale")
        {
            float sx = vals.size() > 0 ? vals[0] : 1.0f;
            float sy = vals.size() > 1 ? vals[1] : sx;
            m.Scale(sx, sy);
        }
        else if (func == "rotate" && vals.size() > 0)
        {
            float angle = vals[0];
            float cx = vals.size() > 1 ? vals[1] : 0.0f;
            float cy = vals.size() > 2 ? vals[2] : 0.0f;
            if (cx != 0.0f || cy != 0.0f)
            {
                m.Translate(cx, cy);
                m.Rotate(angle);
                m.Translate(-cx, -cy);
            }
            else
            {
                m.Rotate(angle);
            }
        }
        else if (func == "translate" && vals.size() >= 1)
        {
            float tx = vals[0];
            float ty = vals.size() > 1 ? vals[1] : 0.0f;
            m.Translate(tx, ty);
        }
        else if (func == "skewX" && vals.size() >= 1)
        {
            m.Shear(std::tan(vals[0] * 3.14159265f / 180.0f), 0.0f);
        }
        else if (func == "skewY" && vals.size() >= 1)
        {
            m.Shear(0.0f, std::tan(vals[0] * 3.14159265f / 180.0f));
        }
        result->Multiply(&m, MatrixOrderAppend);
    }
}

std::unique_ptr<Brush> GdiPlusGradientRenderer::CreateBrush(const std::shared_ptr<SvgGradient> &gradient, const RectF &bounds, float opacity)
{
    if (!gradient)
        return nullptr;

    const bool isObjectBBox = (gradient->gradientUnits != "userSpaceOnUse");
    
    // Matrix logic:
    // We want P_device = M_world * P_local.
    // P_local is the coordinate space where we define the brush (e.g. 0..1 or user coords).
    // GDI+ Brush Transform T maps P_local -> P_world.
    //
    // If objectBoundingBox:
    //   P_local is 0..1 (Unit Space).
    //   We need T_bbox to map Unit -> BBox.
    //   T_bbox: Scale(w, h) then Translate(x, y). (Unit * Scale + Translate).
    //   We also have GradientTransform T_grad.
    //   SVG Spec: "transform... processes the coordinates... before... bounding box"
    //   So: P_transformed = T_grad * P_unit.
    //   Then P_world = T_bbox * P_transformed = T_bbox * (T_grad * P_unit).
    //   So Brush Transform T = T_bbox * T_grad.
    //   GDI+ Multiply(A, Append) -> this * A.  Multiply(A, Prepend) -> A * this.
    //   We want Gradient * BBox? No, we want T_bbox * T_grad for row vectors?
    //   Let's check: P' = P * M. If P is row vector.
    //   P * Grad * BBox (Scale then Translate).
    //   Wait, SVG is Column Vectors: P' = BBox * Grad * P.
    //   GDI+ Row Vectors: P' = P * Grad * BBox.
    //   We want P * Grad * BBox.
    //   We have 'transform' = BBox.
    //   We want Grad * BBox.
    //   transform.Multiply(&grad, Prepend) -> grad * transform.
    //   So correct is Prepend.

    Matrix transform;
    if (isObjectBBox)
    {
        transform.Translate(bounds.X, bounds.Y);
        transform.Scale(bounds.Width, bounds.Height);
    }

    if (!gradient->gradientTransform.empty())
    {
        Matrix gradXform;
        ParseTransform(gradient->gradientTransform, &gradXform);
        transform.Multiply(&gradXform, MatrixOrderPrepend);
    }

    if (gradient->type == GradientType::Radial)
    {
        auto radialGrad = std::dynamic_pointer_cast<SvgRadialGradient>(gradient);
        if (!radialGrad) return nullptr;

        // Coordinates are either 0..1 or User Space depending on gradientUnits.
        // But since we set up the Transform to map 0..1 -> User Space (if BBox),
        // we can just treat them as "Local Brush Coords".
        float cx = radialGrad->cx;
        float cy = radialGrad->cy;
        float r = radialGrad->r;
        float fx = radialGrad->fx;
        float fy = radialGrad->fy;

        // Path is defined in Local Brush Coords (0..1 for BBox)
        if (r < 1e-5f) r = 1e-5f;
        
        GraphicsPath path;
        path.AddEllipse(cx - r, cy - r, 2 * r, 2 * r);

        PathGradientBrush *pgb = new PathGradientBrush(&path);
        pgb->SetCenterPoint(PointF(fx, fy));

        // Colors
         int n = static_cast<int>(radialGrad->stops.size());
         Color svgCenterColor = (n > 0) ? radialGrad->stops.front().color : Color(0,0,0,0);
         for (const auto &stop : radialGrad->stops) {
             if (stop.offset <= 0.001f) {
                 svgCenterColor = stop.color;
                 break;
             }
         }
         svgCenterColor = ApplyColorOpacity(svgCenterColor, opacity);
         pgb->SetCenterColor(svgCenterColor);

         std::vector<Color> colors;
         std::vector<REAL> positions;
         colors.reserve(n);
         positions.reserve(n);

         // SVG Radial: stops 0 (focus) -> 1 (edge).
         // GDI+ PathGradient: internal 1 (center) -> 0 (edge).
         // Map: GDI_Pos = 1.0 - SVG_Offset.
         for (int i = n - 1; i >= 0; --i)
         {
             colors.push_back(ApplyColorOpacity(radialGrad->stops[i].color, opacity));
             positions.push_back(1.0f - radialGrad->stops[i].offset);
         }
         
         // Fix boundaries 0 (edge) and 1 (center)
         if (n > 0)
         {
            if (positions.front() > 0.001f)
            {
                // The last SVG stop (edge) was at < 1.0. Pad specific color to edge? 
                // SVG spec: pad means last color extends. 
                // We are at edge (GDI 0). We simply duplicate the first color (which corresponds to SVG edge color).
                colors.insert(colors.begin(), colors.front());
                positions.insert(positions.begin(), 0.0f);
                ++n;
            }
            if (positions.back() < 0.999f)
            {
                // The first SVG stop (center) was at > 0. Pad.
                colors.push_back(svgCenterColor);
                positions.push_back(1.0f);
                ++n;
            }
         }
         else
         {
             delete pgb; 
             return nullptr;
         }

        std::unique_ptr<Color[]> colorArr(new Color[n]);
        std::unique_ptr<REAL[]> posArr(new REAL[n]);
        for (int i = 0; i < n; ++i)
        {
            colorArr[i] = colors[i];
            posArr[i] = positions[i];
        }
        pgb->SetInterpolationColors(colorArr.get(), posArr.get(), n);
        
        // Spread
         if (radialGrad->spreadMethod == "reflect")
             pgb->SetWrapMode(WrapModeTileFlipXY);
         else if (radialGrad->spreadMethod == "repeat")
             pgb->SetWrapMode(WrapModeTile);
         else
             pgb->SetWrapMode(WrapModeClamp);

        // Apply Transform
        pgb->SetTransform(&transform);
        
        return std::unique_ptr<Brush>(pgb);
    }
    else // Linear
    {
        auto linearGrad = std::dynamic_pointer_cast<SvgLinearGradient>(gradient);
        if (!linearGrad) return nullptr;

        PointF p1(linearGrad->x1, linearGrad->y1);
        PointF p2(linearGrad->x2, linearGrad->y2);

        // Degenerate case check
        if (std::abs(p1.X - p2.X) < 1e-5 && std::abs(p1.Y - p2.Y) < 1e-5)
        {
             if (!linearGrad->stops.empty())
                 return std::make_unique<SolidBrush>(ApplyColorOpacity(linearGrad->stops.back().color, opacity));
             return nullptr;
        }

        LinearGradientBrush *lg = new LinearGradientBrush(p1, p2, Color(0,0,0,0), Color(0,0,0,0));
        
        int n = static_cast<int>(linearGrad->stops.size());
        if (n == 0) {
             delete lg; return nullptr;
        }
        
        std::vector<Color> colors;
        std::vector<REAL> positions;
        colors.reserve(n);
        positions.reserve(n);
        for (const auto &s : linearGrad->stops)
        {
            colors.push_back(ApplyColorOpacity(s.color, opacity));
            positions.push_back(s.offset);
        }
        
        if (positions.front() > 0.0f)
        {
            colors.insert(colors.begin(), colors.front());
            positions.insert(positions.begin(), 0.0f);
            ++n;
        }
        if (positions.back() < 1.0f)
        {
             colors.push_back(colors.back());
             positions.push_back(1.0f);
             ++n;
        }

        std::unique_ptr<Color[]> colorArr(new Color[n]);
        std::unique_ptr<REAL[]> posArr(new REAL[n]);
        for (int i = 0; i < n; ++i)
        {
             colorArr[i] = colors[i];
             posArr[i] = positions[i];
        }
        lg->SetInterpolationColors(colorArr.get(), posArr.get(), n);

        if (linearGrad->spreadMethod == "reflect")
             lg->SetWrapMode(WrapModeTileFlipXY);
        else if (linearGrad->spreadMethod == "repeat")
             lg->SetWrapMode(WrapModeTile);
        else
             lg->SetWrapMode(WrapModeClamp);

        lg->SetGammaCorrection(TRUE);
        lg->SetTransform(&transform);
        
        return std::unique_ptr<Brush>(lg);
    }
}
