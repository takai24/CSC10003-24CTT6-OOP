#include "stdafx.h"
#include "SvgPaintResolver.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>

using namespace Gdiplus;

namespace {

    // Helper: Apply opacity to color
    Color ApplyColorOpacity(Color c, float opacity)
    {
        if (opacity < 0.0f) opacity = 0.0f;
        if (opacity > 1.0f) opacity = 1.0f;
        float origA = c.GetAlpha() / 255.0f;
        float finalA = origA * opacity;
        return Color(static_cast<BYTE>(finalA * 255.0f + 0.5f), c.GetR(), c.GetG(), c.GetB());
    }

    // Helper: Parse Transform
    void ParseTransform(const std::string &xform, Matrix *result)
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
            ss.imbue(std::locale::classic());
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
                if (cx != 0.0f || cy != 0.0f) { m.Translate(cx, cy); m.Rotate(angle); m.Translate(-cx, -cy); }
                else { m.Rotate(angle); }
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
}

std::unique_ptr<Brush> SvgPaintResolver::CreateBrush(const std::shared_ptr<SvgGradient> &gradient, const RectF &bounds, float opacity)
{
    if (!gradient) return nullptr;

    const bool isObjectBBox = (gradient->gradientUnits != "userSpaceOnUse");

    // Helper to map units to pixels
    auto MapPoint = [&](float x, float y) -> PointF {
        if (isObjectBBox) {
            return PointF(bounds.X + x * bounds.Width, bounds.Y + y * bounds.Height);
        }
        return PointF(x, y);
    };

    // Calculate T_grad (Gradient Matrix)
    Matrix gradMatrix;
    if (!gradient->gradientTransform.empty())
    {
        ParseTransform(gradient->gradientTransform, &gradMatrix);
    }

    if (gradient->type == GradientType::Radial)
    {
        auto radialGrad = std::dynamic_pointer_cast<SvgRadialGradient>(gradient);
        if (!radialGrad) return nullptr;

        float cx = radialGrad->cx;
        float cy = radialGrad->cy;
        float r = radialGrad->r;
        float fx = radialGrad->fx;
        float fy = radialGrad->fy;

        if (r <= 1e-6) return nullptr;

        int n = static_cast<int>(radialGrad->stops.size());
        if (n == 0) return nullptr;

        // Texture Mapping Approach for Radial Gradient
        // 1. Draw a Canonical Radial Gradient on a temp bitmap (S x S)
        //    Canonical: Center(0.5, 0.5), Radius 0.5 (or similar coverage).
        //    We map SVG Radial (cx,cy,r) to this Canonical space.
        
        // Let's use a fixed size texture. 256x256 is usually enough for smooth gradients.
        // Higher res if quality needed.
        int texSize = 256;
        Bitmap tex(texSize, texSize, PixelFormat32bppARGB);
        {
            Graphics g(&tex);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            g.Clear(Color(0,0,0,0)); // Transparent background
            
            // Draw gradient in the center of the bitmap
            GraphicsPath path;
            RectF rect(0, 0, (REAL)texSize, (REAL)texSize);
            path.AddEllipse(rect);
            
            PathGradientBrush pgb(&path);
            
            // Center point
            // For SVG 'r', we map 'r' to the bitmap radius.
            // SVG: stops 0 (focus) -> 1 (edge/radius).
            // GDI: 0 (edge) -> 1 (center).
            // We map SVG(r) -> bitmap(width/2).
            
            pgb.SetCenterPoint(PointF(texSize/2.0f, texSize/2.0f));

            // Adjust focus point
            // Relative focus in unit circle?
            // Map (fx-cx)/r to bitmap space.
            float relFx = (fx - cx) / r;
            float relFy = (fy - cy) / r;
            // Clamp focus to be inside
            float dist = sqrt(relFx*relFx + relFy*relFy);
            if(dist > 0.99f){
                relFx *= 0.99f/dist;
                relFy *= 0.99f/dist;
            }
            PointF centerP(texSize/2.0f + relFx * texSize/2.0f, texSize/2.0f + relFy * texSize/2.0f);
            pgb.SetCenterPoint(centerP);

            // Stops Setup
            std::vector<Color> colors;
            std::vector<REAL> positions;
            colors.reserve(n);
            positions.reserve(n);

            // Reverse for GDI+
            for (int i = n - 1; i >= 0; --i)
            {
                colors.push_back(ApplyColorOpacity(radialGrad->stops[i].color, opacity));
                positions.push_back(1.0f - radialGrad->stops[i].offset);
            }
            
            // Fix boundaries
            if (positions.front() > 0.001f) {
                colors.insert(colors.begin(), colors.front());
                positions.insert(positions.begin(), 0.0f);
                ++n;
            }
            if (positions.back() < 0.999f) {
                colors.push_back(colors.back());
                positions.push_back(1.0f);
                ++n;
            }

            pgb.SetCenterColor(colors.back());
            
            std::unique_ptr<Color[]> colorArr(new Color[n]);
            std::unique_ptr<REAL[]> posArr(new REAL[n]);
            for(int i=0; i<n; ++i) {
                 colorArr[i] = colors[i];
                 posArr[i] = positions[i];
            }
            pgb.SetInterpolationColors(colorArr.get(), posArr.get(), n);
            
            // Fill the texture
            g.FillRectangle(&pgb, rect);
        }

        // 2 Create TextureBrush
        TextureBrush *tb = new TextureBrush(&tex);
        
        // 3. Configure WrapMode
        if (radialGrad->spreadMethod == "reflect")
             tb->SetWrapMode(WrapModeTileFlipXY);
        else if (radialGrad->spreadMethod == "repeat")
             tb->SetWrapMode(WrapModeTile);
        else
             tb->SetWrapMode(WrapModeClamp);

        // 4. Calculate Final Transform Matrix
        // We need to map the Texture Space (0..texSize) to the Final World Space.
        // Conceptually: P_world = T_bbox * T_grad * P_unit
        // And we have P_tex mapping to P_unit.
        // P_unit = (P_tex / texSize - 0.5) * 2 * r + (cx, cy)
        // Wait, standard mapping:
        // Texture is a circle of radius texSize/2 at (texSize/2, texSize/2).
        // It represents the SVG circle at (cx, cy) with radius r.
        
        // Transformation chain:
        // Texture Space -> [Scale to 1x1, Center 0] -> [Scale to r, Center 0] -> [Translate to cx, cy] -> Unit Space -> World Space.
        
        Matrix m;
        // 1. Center the texture: (0,0) is top-left. Move to (-texSize/2, -texSize/2)
        m.Translate(-texSize/2.0f, -texSize/2.0f, MatrixOrderAppend);
        
        // 2. Scale to Radius 1 (Diameter 2? No, texSize is diameter).
        // Texture diameter = texSize => Radius = texSize/2.
        // So currently x in [-texSize/2, texSize/2].
        // We want x in [-r, r] (relative) ?
        // Or directly to Unit Space?
        
        // Let's go to "Gradient Space".
        // The gradient is defined by (cx, cy, r).
        // P_grad = (P_tex_centered / (texSize/2)) * r + (cx, cy).
        // Scale by: r / (texSize/2) = 2*r / texSize.
        float scaleFactor = 2.0f * r / (float)texSize;
        m.Scale(scaleFactor, scaleFactor, MatrixOrderAppend);
        
        // 3. Translate to (cx, cy)
        m.Translate(cx, cy, MatrixOrderAppend);
        
        // Now we are in "Unit Space" (or User Space if !isObjectBBox).
        // 4. Apply T_grad (gradientTransform)
        m.Multiply(&gradMatrix, MatrixOrderAppend);
        
        // 5. Apply T_bbox (if needed)
        if (isObjectBBox) {
            Matrix t_bbox;
            t_bbox.Translate(bounds.X, bounds.Y);
            t_bbox.Scale(bounds.Width, bounds.Height);
            m.Multiply(&t_bbox, MatrixOrderAppend);
        }

        tb->SetTransform(&m);
        return std::unique_ptr<Brush>(tb);
    }
    else // Linear Gradient (Keep Existing Logic, it works fine)
    {
        auto linearGrad = std::dynamic_pointer_cast<SvgLinearGradient>(gradient);
        if (!linearGrad) return nullptr;

        PointF p1(linearGrad->x1, linearGrad->y1);
        PointF p2(linearGrad->x2, linearGrad->y2);
        
        // Map points to World Space if needed (LinearBrush works best with real points)
        // BUT wait, we want to respect gradientTransform too.
        // If we map points, we burn T_bbox.
        // What about T_grad?
        // LG(P1, P2) applies T on top.
        // If we want P_world = T_bbox * T_grad * P_unit.
        // We can define Brush in Unit Space (P1, P2 as is) and set T = T_bbox * T_grad.
        // This is robust.
        
        if (std::abs(p1.X - p2.X) < 1e-5 && std::abs(p1.Y - p2.Y) < 1e-5)
        {
             if (!linearGrad->stops.empty())
                 return std::make_unique<SolidBrush>(ApplyColorOpacity(linearGrad->stops.back().color, opacity));
             return nullptr;
        }

        int n = static_cast<int>(linearGrad->stops.size());
        if (n == 0) return nullptr;

        Color startColor = ApplyColorOpacity(linearGrad->stops.front().color, opacity);
        Color endColor = ApplyColorOpacity(linearGrad->stops.back().color, opacity);

        LinearGradientBrush *lg = new LinearGradientBrush(p1, p2, startColor, endColor);
        
        std::vector<Color> colors;
        std::vector<REAL> positions;
        colors.reserve(n);
        positions.reserve(n);
        for (const auto &s : linearGrad->stops)
        {
            colors.push_back(ApplyColorOpacity(s.color, opacity));
            positions.push_back(s.offset);
        }
        
        if (positions.front() > 0.0f) {
            colors.insert(colors.begin(), colors.front());
            positions.insert(positions.begin(), 0.0f);
            ++n;
        }
        if (positions.back() < 1.0f) {
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
        
        // Matrix Composition: T_total = T_bbox * T_grad
        Matrix finalMatrix;
        if (!gradient->gradientTransform.empty())
        {
             finalMatrix.Multiply(&gradMatrix, MatrixOrderAppend); // Apply T_grad first (if points are in local)
        }
        
        if (isObjectBBox) {
            // Apply T_bbox
            Matrix t_bbox;
            t_bbox.Scale(bounds.Width, bounds.Height, MatrixOrderAppend); // Scale dimensions
            t_bbox.Translate(bounds.X, bounds.Y, MatrixOrderAppend); // Move to position
            
            // Standard order: Scale then Translate.
            // But GDI Matrix Append: Result = Final * New.
            // Wait.
            // We want P_world = T_translate * T_scale * P_unit.
            // So we want Scale THEN Translate.
            // Matrix m; m.Scale(); m.Translate() (Append order applies operations in order).
            // Yes.
            
            // But wait, my previous logic was:
            // T_bbox.Translate (X, Y)
            // T_bbox.Scale (W, H)
            // If Append: Translate then Scale? No.
            // M.Translate(x,y); -> M = [1 0 0; 0 1 0; x y 1]
            // M.Scale(sx,sy); -> M = M * S = [sx 0 0; 0 sy 0; x y 1]?
            // Point P * M = P * M_trans * M_scale.
            // = (P + T) * S.
            // This is Translate then Scale.
            
            // We want Scale THEN Translate.
            // P_world = P * S * T.
            // So M = S * T.
            // So Scale first, then Translate.
            
            Matrix t_scale_trans;
            t_scale_trans.Scale(bounds.Width, bounds.Height);
            t_scale_trans.Translate(bounds.X, bounds.Y, MatrixOrderAppend); // Append T to S.
            
            finalMatrix.Multiply(&t_scale_trans, MatrixOrderAppend);
        }
        
        lg->SetTransform(&finalMatrix);
        return std::unique_ptr<Brush>(lg);
    }
}
