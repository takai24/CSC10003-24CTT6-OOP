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

    // Validate bounds globaly to prevent NaN/Inf issues for BOTH Radial and Linear
    if (!std::isfinite(bounds.Width) || !std::isfinite(bounds.Height)) 
         return std::make_unique<SolidBrush>(Color(0, 0, 0, 0));

    if (isObjectBBox && (bounds.Width <= 0.0f || bounds.Height <= 0.0f))
         return std::make_unique<SolidBrush>(Color(0, 0, 0, 0));

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
        
        // Validate Matrix Elements
        REAL m[6];
        gradMatrix.GetElements(m);
        for(int i=0; i<6; ++i) {
             if (!std::isfinite(m[i])) {
                 // Invalid matrix
                 return std::make_unique<SolidBrush>(Color(255, 0, 0, 0)); // Return transparent
             }
        }
    }

    if (gradient->type == GradientType::Radial)
    {
        auto radialGrad = std::dynamic_pointer_cast<SvgRadialGradient>(gradient);
        if (!radialGrad) return std::make_unique<SolidBrush>(Color(0, 0, 0, 0));

        float cx = radialGrad->cx;
        float cy = radialGrad->cy;
        float r = radialGrad->r;
        float fx = radialGrad->hasFx ? radialGrad->fx : cx;
        float fy = radialGrad->hasFy ? radialGrad->fy : cy;

        if (!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(r) || !std::isfinite(fx) || !std::isfinite(fy))
             return std::make_unique<SolidBrush>(Color(0, 0, 0, 0));

        if (r <= 1e-6) return std::make_unique<SolidBrush>(Color(0, 0, 0, 0));

        int n = static_cast<int>(radialGrad->stops.size());
        if (n == 0) return std::make_unique<SolidBrush>(Color(0, 0, 0, 0));

        // Prepare Colors first
        std::vector<Color> colors;
        std::vector<REAL> positions;
        colors.reserve(n);
        positions.reserve(n);

        // Iterate backwards from Stop 1 down to Stop 0
        for (int i = n - 1; i >= 0; --i)
        {
            colors.push_back(ApplyColorOpacity(radialGrad->stops[i].color, opacity));
            positions.push_back(1.0f - radialGrad->stops[i].offset);
        }
        
        // Ensure boundaries 0.0 and 1.0 exist for GDI validity
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

        // Resolution Optimization: 512x512 is sufficient for high quality and performance
        int texSize = 512;

        // Texture Mapping Approach for Radial Gradient
        Bitmap tex(texSize, texSize, PixelFormat32bppARGB);
        if (tex.GetLastStatus() != Ok) {
             return std::make_unique<SolidBrush>(Color(0, 0, 0, 0));
        }
        {
            Graphics g(&tex);
            g.SetSmoothingMode(SmoothingModeAntiAlias);
            
            // For 'pad' (default), we want the area OUTSIDE the definition to be the end color.
            // SVG Stop 1 (Edge) corresponds to our `colors.front()` (Position 0.0f in GDI Edge).
            Color edgeColor = colors.front();
            if (radialGrad->spreadMethod == "pad" || radialGrad->spreadMethod == "")
                g.Clear(edgeColor); 
            else
                g.Clear(Color(0,0,0,0)); 

            // Draw gradient in the center of the bitmap
            GraphicsPath path;
            RectF rect(0, 0, (REAL)texSize, (REAL)texSize);
            path.AddEllipse(rect);
            
            PathGradientBrush pgb(&path);
            pgb.SetCenterPoint(PointF(texSize/2.0f, texSize/2.0f));

            // Adjust focus point
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

            pgb.SetCenterColor(colors.back()); // GDI Center (t=1.0) is SVG Start (s=0)
            
            std::unique_ptr<Color[]> colorArr(new Color[n]);
            std::unique_ptr<REAL[]> posArr(new REAL[n]);
            for(int i=0; i<n; ++i) {
                 colorArr[i] = colors[i];
                 posArr[i] = positions[i];
            }
            pgb.SetInterpolationColors(colorArr.get(), posArr.get(), n);
            // pgb.SetGammaCorrection(TRUE); // Disabled for sRGB accuracy
            
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
             tb->SetWrapMode(WrapModeClamp); // Clamp extends edgeColor for 'pad'

        // 4. Calculate Final Transform Matrix
        Matrix m;
        m.Translate(-texSize/2.0f, -texSize/2.0f, MatrixOrderAppend);
        float scaleFactor = 2.0f * r / (float)texSize;
        m.Scale(scaleFactor, scaleFactor, MatrixOrderAppend);
        m.Translate(cx, cy, MatrixOrderAppend);
        
        m.Multiply(&gradMatrix, MatrixOrderAppend);
        
        if (isObjectBBox) {
            Matrix t_bbox;
            t_bbox.Scale(bounds.Width, bounds.Height);
            t_bbox.Translate(bounds.X, bounds.Y, MatrixOrderAppend);
            m.Multiply(&t_bbox, MatrixOrderAppend);
        }

        tb->SetTransform(&m);
        return std::unique_ptr<Brush>(tb);
    }
    else // Linear Gradient
    {
        auto linearGrad = std::dynamic_pointer_cast<SvgLinearGradient>(gradient);
        if (!linearGrad) return std::make_unique<SolidBrush>(Color(0, 0, 0, 0));

        PointF p1(linearGrad->x1, linearGrad->y1);
        PointF p2(linearGrad->x2, linearGrad->y2);
        
        // Validate Points
        if (!std::isfinite(p1.X) || !std::isfinite(p1.Y) || !std::isfinite(p2.X) || !std::isfinite(p2.Y))
             return std::make_unique<SolidBrush>(Color(0, 0, 0, 0));
        
        if (std::abs(p1.X - p2.X) < 1e-5 && std::abs(p1.Y - p2.Y) < 1e-5)
        {
             if (!linearGrad->stops.empty())
                 return std::make_unique<SolidBrush>(ApplyColorOpacity(linearGrad->stops.back().color, opacity));
             return std::make_unique<SolidBrush>(Color(0, 0, 0, 0));
        }

        int n = static_cast<int>(linearGrad->stops.size());
        if (n == 0) return std::make_unique<SolidBrush>(Color(0, 0, 0, 0));

        // Determine t-range for Pad method to cover the object
        float min_t = 0.0f;
        float max_t = 1.0f;

        if (linearGrad->spreadMethod == "pad" || linearGrad->spreadMethod == "")
        {
            // Project Object Bounds onto Gradient Vector to find needed coverage
            std::vector<PointF> corners;
            if (isObjectBBox) {
                // Gradient Space is Unit Space 0..1
                corners = {PointF(0,0), PointF(1,0), PointF(1,1), PointF(0,1)};
            } else {
                // Gradient Space is World Space (unless T_grad exists)
                // If T_grad exists, we need to map World Bounds -> Gradient Space
                // P_grad = Inv(T_grad) * P_world
                corners = {PointF(bounds.X, bounds.Y), PointF(bounds.X + bounds.Width, bounds.Y),
                           PointF(bounds.X + bounds.Width, bounds.Y + bounds.Height), PointF(bounds.X, bounds.Y + bounds.Height)};
                
                if (!gradient->gradientTransform.empty()) {
                    Matrix invGradMatrix;
                    ParseTransform(gradient->gradientTransform, &invGradMatrix);
                    invGradMatrix.Invert();
                    
                    // Transform corners
                    for(auto& pt : corners) {
                        PointF pts[1] = {pt};
                        invGradMatrix.TransformPoints(pts, 1);
                        pt = pts[0];
                    }
                }
            }
            
            float dx = p2.X - p1.X;
            float dy = p2.Y - p1.Y;
            float lenSq = dx*dx + dy*dy;
            
            if (lenSq > 1e-9) {
                float minVal = 1e9f;
                float maxVal = -1e9f;
                for(const auto& pt : corners) {
                    // Dot product relative to P1: t = (P-P1).V / |V|^2
                    float t = ((pt.X - p1.X) * dx + (pt.Y - p1.Y) * dy) / lenSq;
                    if (t < minVal) minVal = t;
                    if (t > maxVal) maxVal = t;
                }
                // Add slight margin
                minVal -= 0.01f;
                maxVal += 0.01f;

                if (minVal < min_t) min_t = minVal;
                if (maxVal > max_t) max_t = maxVal;
            }
        }
        
        // Safety Clamping to prevent huge textures (e.g. if projection sends t to infinity)
        if (min_t < -50.0f) min_t = -50.0f;
        if (max_t > 50.0f) max_t = 50.0f;
        // Ensure standard range is covered
        if (min_t > 0.0f) min_t = 0.0f;
        if (max_t < 1.0f) max_t = 1.0f;

        // Texture width calculation: Map [min_t, max_t] to pixels
        float range = max_t - min_t;
        // Use HIGH resolution: 2048px for the 0..1 interval.
        // Total Width = range * 2048.
        int texWidth = static_cast<int>(range * 2048.0f); 
        if (texWidth < 2048) texWidth = 2048;
        if (texWidth > 8192) texWidth = 8192; // Cap size

        int texHeight = 1; // 1px height with Y-scaling is sufficient and clean
        
        Bitmap tex(texWidth, texHeight, PixelFormat32bppARGB);
        if (tex.GetLastStatus() != Ok) {
             return std::make_unique<SolidBrush>(Color(0,0,0,0));
        }

        {
            Graphics g(&tex);
            g.SetSmoothingMode(SmoothingModeAntiAlias);

            // Fill background with appropriate edge colors to handle padding
            Color startColor = ApplyColorOpacity(linearGrad->stops.front().color, opacity);
            Color endColor = ApplyColorOpacity(linearGrad->stops.back().color, opacity);
            
            // Map t=0 and t=1 to pixel coordinates in the texture
            // t_pixel = (t - min_t) / range * texWidth
            float x0 = (0.0f - min_t) / range * texWidth;
            float x1 = (1.0f - min_t) / range * texWidth;
            
            // 1. Fill Start Pad [0, x0]
            if (x0 > 0.5f) {
                 SolidBrush sb(startColor);
                 g.FillRectangle(&sb, 0.0f, 0.0f, x0 + 1.0f, (REAL)texHeight);
            }
            
            // 2. Fill End Pad [x1, W]
            if (x1 < texWidth - 0.5f) {
                 SolidBrush sb(endColor);
                 g.FillRectangle(&sb, x1 - 1.0f, 0.0f, (REAL)texWidth - x1 + 1.0f, (REAL)texHeight);
            }

            // 3. Draw Gradient [x0, x1]
            if (x1 > x0) {
                RectF rect(x0, 0, x1 - x0, (REAL)texHeight);
                // Note: LinearGradientBrush needs 2 points.
                // We draw from x0 to x1.
                // Colors should be Start to End.
                LinearGradientBrush lg(RectF(x0, 0.0f, x1 - x0, (REAL)texHeight), startColor, endColor, LinearGradientModeHorizontal);

                std::vector<Color> colors;
                std::vector<REAL> positions;
                colors.reserve(n);
                positions.reserve(n);
                
                // Add stops
                for (const auto &s : linearGrad->stops) {
                     colors.push_back(ApplyColorOpacity(s.color, opacity));
                     positions.push_back(s.offset);
                }
                // Ensure 0 and 1 are present
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

                std::unique_ptr<Color[]> colorArr(new Color[n]);
                std::unique_ptr<REAL[]> posArr(new REAL[n]);
                for (int i = 0; i < n; ++i) {
                     colorArr[i] = colors[i];
                     posArr[i] = positions[i];
                }
                lg.SetInterpolationColors(colorArr.get(), posArr.get(), n);
                lg.SetInterpolationColors(colorArr.get(), posArr.get(), n);
            // lg.SetGammaCorrection(TRUE); // Removed to match sRGB expectation
                g.FillRectangle(&lg, rect);
            }
        }

        TextureBrush *tb = new TextureBrush(&tex);

        // WrapMode: 
        if (linearGrad->spreadMethod == "reflect")
             tb->SetWrapMode(WrapModeTileFlipX); 
        else if (linearGrad->spreadMethod == "repeat")
             tb->SetWrapMode(WrapModeTile);
        else
             tb->SetWrapMode(WrapModeClamp); 

        // Transform Construction
        // We need to map the Texture to World Space.
        // Texture X-range [0, TexWidth] maps to t-range [min_t, max_t] relative to P1-P2.
        // Specifically, Texture X=0 maps to t=min_t.
        // P_start = P1 + min_t * (P2 - P1).
        // Texture X=0 should map to P_start.
        
        float dx = p2.X - p1.X;
        float dy = p2.Y - p1.Y;
        float baseLen = sqrt(dx*dx + dy*dy);
        // Avoid div by zero
        if(baseLen < 1e-5) baseLen = 1.0f;

        float totalLen = range * baseLen;
        float angle = atan2(dy, dx) * 180.0f / 3.14159265f;
        
        float startX = p1.X + min_t * dx;
        float startY = p1.Y + min_t * dy;

        Matrix m;
        // 1. Scale X to match Total Length (Texture Width -> Total World Length)
        float scaleX = 0;
        if(texWidth > 0) scaleX = totalLen / (float)texWidth;
        
        // 2. Scale Y to be Infinite (slab) and Centered
        // Map 0..1 (TexY) to -10000..10000 (LocalY) relative to center line
        float scaleY = 20000.0f; 
        
        m.Scale(scaleX, scaleY, MatrixOrderAppend);
        m.Translate(0, -scaleY / 2.0f, MatrixOrderAppend); 
        
        // 3. Rotate
        m.Rotate(angle, MatrixOrderAppend);
        
        // 4. Translate to P_start (where t=min_t)
        m.Translate(startX, startY, MatrixOrderAppend);

        // Append Gradient Transform
        if (!gradient->gradientTransform.empty())
        {
             m.Multiply(&gradMatrix, MatrixOrderAppend); 
        }
        
        if (isObjectBBox) {
            Matrix t_bbox;
            t_bbox.Scale(bounds.Width, bounds.Height);
            t_bbox.Translate(bounds.X, bounds.Y, MatrixOrderAppend); 
            m.Multiply(&t_bbox, MatrixOrderAppend);
        }
        
        tb->SetTransform(&m);
        return std::unique_ptr<Brush>(tb);
    }
}
