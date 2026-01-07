#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "SvgElementFactory.h"
#include <vector>
#include <sstream>
#include <cmath>
#include <algorithm>
#include "SvgColors.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace Gdiplus;

Gdiplus::Color ApplyOpacity(Gdiplus::Color c, float opacity)
{
    if (opacity < 0.0f)
        opacity = 0.0f;
    if (opacity > 1.0f)
        opacity = 1.0f;
    // Combine original color alpha with provided opacity multiplicatively
    float origA = c.GetAlpha() / 255.0f;
    float finalA = origA * opacity;
    int alphaInt = static_cast<int>(finalA * 255.0f + 0.5f);
    if (alphaInt < 0)
        alphaInt = 0;
    if (alphaInt > 255)
        alphaInt = 255;
    BYTE alpha = static_cast<BYTE>(alphaInt);
    return Gdiplus::Color(alpha, c.GetR(), c.GetG(), c.GetB());
}

std::unique_ptr<Gdiplus::GraphicsPath> ParsePathData(const std::string &d)
{
    auto path = std::make_unique<Gdiplus::GraphicsPath>();
    const std::string &s = d;
    size_t i = 0;
    float currentX = 0, currentY = 0;
    float startX = 0, startY = 0;
    char lastCmd = 0;
    float lastCtrlX = 0, lastCtrlY = 0; // for S/T commands

    // Helper: convert SVG arc to one or more cubic beziers and add to path
    auto arcToBeziers = [&](double x0, double y0, double rx, double ry, double xAxisRotationDeg,
                            int largeArcFlag, int sweepFlag, double x, double y)
    {
        if (x0 == x && y0 == y)
            return; // no-op
        if (rx == 0.0 || ry == 0.0)
        {
            path->AddLine((float)x0, (float)y0, (float)x, (float)y);
            return;
        }

        double phi = xAxisRotationDeg * M_PI / 180.0;
        double cosPhi = cos(phi), sinPhi = sin(phi);

        // Step 1: compute (x1', y1')
        double dx2 = (x0 - x) / 2.0;
        double dy2 = (y0 - y) / 2.0;
        double x1p = cosPhi * dx2 + sinPhi * dy2;
        double y1p = -sinPhi * dx2 + cosPhi * dy2;

        // Ensure radii are large enough
        double rxAbs = fabs(rx);
        double ryAbs = fabs(ry);
        double rxSq = rxAbs * rxAbs;
        double rySq = ryAbs * ryAbs;
        double x1pSq = x1p * x1p;
        double y1pSq = y1p * y1p;

        double lambda = x1pSq / rxSq + y1pSq / rySq;
        if (lambda > 1.0)
        {
            double factor = sqrt(lambda);
            rxAbs *= factor;
            ryAbs *= factor;
            rxSq = rxAbs * rxAbs;
            rySq = ryAbs * ryAbs;
        }

        // Step 2: compute center
        double sign = (largeArcFlag == sweepFlag) ? -1.0 : 1.0;
        double sq = ((rxSq * rySq) - (rxSq * y1pSq) - (rySq * x1pSq)) / (rxSq * y1pSq + rySq * x1pSq);
        sq = (sq < 0) ? 0 : sq;
        double coef = sign * sqrt(sq);
        double cxp = coef * ((rxAbs * y1p) / ryAbs);
        double cyp = coef * (-(ryAbs * x1p) / rxAbs);

        // Step 3: compute center in original coords
        double cx = cosPhi * cxp - sinPhi * cyp + (x0 + x) / 2.0;
        double cy = sinPhi * cxp + cosPhi * cyp + (y0 + y) / 2.0;

        // Step 4: compute start and delta angles
        auto vectorAngle = [&](double ux, double uy, double vx, double vy)
        {
            double dot = ux * vx + uy * vy;
            double len = sqrt((ux * ux + uy * uy) * (vx * vx + vy * vy));
            // Use parenthesized std::min/std::max to avoid Windows min/max macro collision
            double clamped = (std::max)(-1.0, (std::min)(1.0, dot / len));
            double ang = acos(clamped);
            if (ux * vy - uy * vx < 0)
                ang = -ang;
            return ang;
        };

        double ux = (x1p - cxp) / rxAbs;
        double uy = (y1p - cyp) / ryAbs;
        double vx = (-x1p - cxp) / rxAbs;
        double vy = (-y1p - cyp) / ryAbs;
        double startAngle = atan2(uy, ux);
        double deltaAngle = vectorAngle(ux, uy, vx, vy);

        if (!sweepFlag && deltaAngle > 0)
            deltaAngle -= 2 * M_PI;
        else if (sweepFlag && deltaAngle < 0)
            deltaAngle += 2 * M_PI;

        // Split into segments of max PI/2
        int segments = static_cast<int>(ceil(fabs(deltaAngle) / (M_PI / 2.0)));
        double delta = deltaAngle / segments;

        for (int iSeg = 0; iSeg < segments; ++iSeg)
        {
            double t1 = startAngle + iSeg * delta;
            double t2 = t1 + delta;
            double cosT1 = cos(t1), sinT1 = sin(t1);
            double cosT2 = cos(t2), sinT2 = sin(t2);

            // endpoints
            double x1 = cx + rxAbs * (cosPhi * cosT1 - sinPhi * sinT1);
            double y1 = cy + ryAbs * (sinPhi * cosT1 + cosPhi * sinT1);
            double x4 = cx + rxAbs * (cosPhi * cosT2 - sinPhi * sinT2);
            double y4 = cy + ryAbs * (sinPhi * cosT2 + cosPhi * sinT2);

            // control points
            double tanDelta = tan((t2 - t1) / 2.0);
            double alpha = (sin(t2 - t1) * (sqrt(4.0 + 3.0 * tanDelta * tanDelta) - 1.0)) / 3.0;

            double x2 = x1 + alpha * (-rxAbs * (cosPhi * sinT1 + sinPhi * cosT1));
            double y2 = y1 + alpha * (-ryAbs * (sinPhi * sinT1 - cosPhi * cosT1));

            double x3 = x4 + alpha * (rxAbs * (cosPhi * sinT2 + sinPhi * cosT2));
            double y3 = y4 + alpha * (ryAbs * (sinPhi * sinT2 - cosPhi * cosT2));

            // For the first segment, ensure we start at current point
            if (iSeg == 0)
            {
                path->AddLine((float)x0, (float)y0, (float)x1, (float)y1);
            }
            path->AddBezier((float)x1, (float)y1, (float)x2, (float)y2, (float)x3, (float)y3, (float)x4, (float)y4);
        }
    };

    auto skipSpaces = [&](void)
    {
        while (i < s.size() && (s[i] == ' ' || s[i] == ',' || s[i] == '\n' || s[i] == '\t' || s[i] == '\r'))
            ++i;
    };

    auto parseNumber = [&](float &out) -> bool
    {
        skipSpaces();
        if (i >= s.size())
            return false;
        size_t start = i;
        // accept sign
        if (s[i] == '+' || s[i] == '-')
            ++i;
        bool hasDigits = false;
        while (i < s.size() && isdigit(static_cast<unsigned char>(s[i])))
        {
            ++i;
            hasDigits = true;
        }
        if (i < s.size() && s[i] == '.')
        {
            ++i;
            while (i < s.size() && isdigit(static_cast<unsigned char>(s[i])))
            {
                ++i;
                hasDigits = true;
            }
        }
        if (i < s.size() && (s[i] == 'e' || s[i] == 'E'))
        {
            ++i;
            if (i < s.size() && (s[i] == '+' || s[i] == '-'))
                ++i;
            bool hasExp = false;
            while (i < s.size() && isdigit(static_cast<unsigned char>(s[i])))
            {
                ++i;
                hasExp = true;
            }
            if (!hasExp)
            { /* rollback? */
            }
        }
        if (!hasDigits)
            return false;
        try
        {
            // use C locale for parsing
             std::string numStr = s.substr(start, i - start);
             std::stringstream ss(numStr);
             ss.imbue(std::locale::classic());
             ss >> out;
        }
        catch (...)
        {
            return false;
        }
        return true;
    };

    auto parseFlag = [&](float &out) -> bool
    {
        skipSpaces();
        if (i >= s.size()) return false;
        char c = s[i];
        if (c == '0' || c == '1')
        {
            out = (float)(c - '0');
            ++i;
            return true;
        }
        return false;
    };

    while (i < s.size())
    {
        skipSpaces();
        if (i >= s.size())
            break;
        char ch = s[i];
        if (isalpha(static_cast<unsigned char>(ch)))
        {
            ++i;
            // Do not update lastCmd here — it should represent the previous
            // command for handling smooth bezier reflection. lastCmd will be
            // updated at the end of the loop after the command is processed.
        }
        else if (lastCmd == 0)
        {
            // invalid
            ++i;
            continue;
        }
        else
        {
            ch = lastCmd; // implicit command repeat
        }

        bool rel = islower(static_cast<unsigned char>(ch));
        char cmd = static_cast<char>(toupper(static_cast<unsigned char>(ch)));

        if (cmd == 'Z')
        {
            path->CloseFigure();
            currentX = startX;
            currentY = startY;
            lastCtrlX = lastCtrlY = 0;
            continue;
        }

        if (cmd == 'M')
        {
            float x, y;
            if (!parseNumber(x) || !parseNumber(y))
                break;
            if (rel)
            {
                x += currentX;
                y += currentY;
            }
            path->StartFigure();
            currentX = x;
            currentY = y;
            startX = x;
            startY = y;
            // subsequent pairs without command are treated as implicit L
            skipSpaces();
            while (true)
            {
                size_t save = i;
                float nx, ny;
                if (!parseNumber(nx) || !parseNumber(ny))
                {
                    i = save;
                    break;
                }
                if (rel)
                {
                    nx += currentX;
                    ny += currentY;
                }
                path->AddLine(currentX, currentY, nx, ny);
                currentX = nx;
                currentY = ny;
                skipSpaces();
            }
        }
        else if (cmd == 'L')
        {
            float x, y;
            while (parseNumber(x) && parseNumber(y))
            {
                if (rel)
                {
                    x += currentX;
                    y += currentY;
                }
                path->AddLine(currentX, currentY, x, y);
                currentX = x;
                currentY = y;
                skipSpaces();
            }
        }
        else if (cmd == 'H')
        {
            float x;
            while (parseNumber(x))
            {
                if (rel)
                    x += currentX;
                path->AddLine(currentX, currentY, x, currentY);
                currentX = x;
                skipSpaces();
            }
        }
        else if (cmd == 'V')
        {
            float y;
            while (parseNumber(y))
            {
                if (rel)
                    y += currentY;
                path->AddLine(currentX, currentY, currentX, y);
                currentY = y;
                skipSpaces();
            }
        }
        else if (cmd == 'C')
        {
            float x1, y1, x2, y2, x3, y3;
            while (parseNumber(x1) && parseNumber(y1) && parseNumber(x2) && parseNumber(y2) && parseNumber(x3) && parseNumber(y3))
            {
                if (rel)
                {
                    x1 += currentX;
                    y1 += currentY;
                    x2 += currentX;
                    y2 += currentY;
                    x3 += currentX;
                    y3 += currentY;
                }
                path->AddBezier(currentX, currentY, x1, y1, x2, y2, x3, y3);
                lastCtrlX = x2;
                lastCtrlY = y2;
                currentX = x3;
                currentY = y3;
                skipSpaces();
            }
        }
        else if (cmd == 'S')
        {
            float x2, y2, x3, y3;
            while (parseNumber(x2) && parseNumber(y2) && parseNumber(x3) && parseNumber(y3))
            {
                float x1 = currentX, y1 = currentY;
                if (lastCmd == 'C' || lastCmd == 'c' || lastCmd == 'S' || lastCmd == 's')
                {
                    // reflect last control
                    x1 = currentX + (currentX - lastCtrlX);
                    y1 = currentY + (currentY - lastCtrlY);
                }
                if (rel)
                {
                    x2 += currentX;
                    y2 += currentY;
                    x3 += currentX;
                    y3 += currentY;
                }
                path->AddBezier(currentX, currentY, x1, y1, x2, y2, x3, y3);
                lastCtrlX = x2;
                lastCtrlY = y2;
                currentX = x3;
                currentY = y3;
                skipSpaces();
            }
        }
        else if (cmd == 'Q')
        {
            float qx, qy, x2, y2;
            while (parseNumber(qx) && parseNumber(qy) && parseNumber(x2) && parseNumber(y2))
            {
                // convert quadratic (current->q->end) to cubic
                float c1x = currentX + 2.0f / 3.0f * (qx - currentX);
                float c1y = currentY + 2.0f / 3.0f * (qy - currentY);
                float c2x = x2 + 2.0f / 3.0f * (qx - x2);
                float c2y = y2 + 2.0f / 3.0f * (qy - y2);
                float ex = x2, ey = y2;
                if (rel)
                {
                    qx += currentX;
                    qy += currentY;
                    ex += currentX;
                    ey += currentY; /* recalc c1/c2? handle by computing after rel */
                    c1x = currentX + 2.0f / 3.0f * (qx - currentX);
                    c1y = currentY + 2.0f / 3.0f * (qy - currentY);
                    c2x = ex + 2.0f / 3.0f * (qx - ex);
                    c2y = ey + 2.0f / 3.0f * (qy - ey);
                }
                path->AddBezier(currentX, currentY, c1x, c1y, c2x, c2y, ex, ey);
                lastCtrlX = qx;
                lastCtrlY = qy;
                currentX = ex;
                currentY = ey;
                skipSpaces();
            }
        }
        else if (cmd == 'T')
        {
            float x2, y2;
            while (parseNumber(x2) && parseNumber(y2))
            {
                float qx = currentX, qy = currentY;
                if (lastCmd == 'Q' || lastCmd == 'q' || lastCmd == 'T' || lastCmd == 't')
                {
                    qx = currentX + (currentX - lastCtrlX);
                    qy = currentY + (currentY - lastCtrlY);
                }
                float ex = x2, ey = y2;
                if (rel)
                {
                    ex += currentX;
                    ey += currentY;
                }
                // convert to cubic
                float c1x = currentX + 2.0f / 3.0f * (qx - currentX);
                float c1y = currentY + 2.0f / 3.0f * (qy - currentY);
                float c2x = ex + 2.0f / 3.0f * (qx - ex);
                float c2y = ey + 2.0f / 3.0f * (qy - ey);
                path->AddBezier(currentX, currentY, c1x, c1y, c2x, c2y, ex, ey);
                lastCtrlX = qx;
                lastCtrlY = qy;
                currentX = ex;
                currentY = ey;
                skipSpaces();
            }
        }
        else if (cmd == 'A')
        {
            // params: rx ry x-axis-rotation large-arc-flag sweep-flag x y
            float rx, ry, xrot, laf, sf, x, y;
            while (parseNumber(rx) && parseNumber(ry) && parseNumber(xrot) && parseFlag(laf) && parseFlag(sf) && parseNumber(x) && parseNumber(y))
            {
                if (rel)
                {
                    x += currentX;
                    y += currentY;
                }
                // Convert arc to bezier segments and add to path
                arcToBeziers(currentX, currentY, rx, ry, xrot, static_cast<int>(laf), static_cast<int>(sf), x, y);
                currentX = x;
                currentY = y;
                skipSpaces();
            }
        }

        lastCmd = ch;
    }
    return path;
}

// Local helpers so missing attributes get reasonable defaults instead of empty strings / exceptions.
namespace
{
    inline std::string AttrOr(const IXMLNode &node, const char *name, const std::string &def = "")
    {
        std::string v = node.getAttribute(name);
        return v.empty() ? def : v;
    }

    inline float AttrOrFloat(const IXMLNode &node, const char *name, float def = 0.0f)
    {
        std::string v = node.getAttribute(name);
        if (v.empty())
            return def;
        try
        {
             std::stringstream ss(v);
             ss.imbue(std::locale::classic());
             float val = def;
             ss >> val;
             return val;
        }
        catch (...)
        {
            return def;
        }
    }


    std::unordered_map<std::string, std::string> ParseStyleAttribute(const std::string& style)
    {
        std::unordered_map<std::string, std::string> styles;
        if (style.empty()) return styles;

        std::stringstream ss(style);
        std::string item;
        while (std::getline(ss, item, ';'))
        {
            size_t colon = item.find(':');
            if (colon != std::string::npos)
            {
                std::string key = item.substr(0, colon);
                std::string val = item.substr(colon + 1);
                
                // trim spaces
                auto trim = [](std::string& s) {
                    size_t first = s.find_first_not_of(' ');
                    if (std::string::npos == first) { s = ""; return; }
                    size_t last = s.find_last_not_of(' ');
                    s = s.substr(first, (last - first + 1));
                };
                trim(key);
                trim(val);
                if (!key.empty()) styles[key] = val;
            }
        }
        return styles;
    }
}

std::unique_ptr<ISvgElement> SvgElementFactory::CreateElement(const IXMLNode &node) const
{
    const std::string tag = node.getTagName();
    if (AttrOr(node, "display", "") == "none")
        return nullptr;

    std::unique_ptr<ISvgElement> element = nullptr;

    std::string styleStr = node.getAttribute("style");
    auto styles = ParseStyleAttribute(styleStr);

    auto GetAttr = [&](const char* name) -> std::string {
        auto it = styles.find(name);
        if (it != styles.end()) return it->second;
        return node.getAttribute(name);
    };

    auto AttrOr = [&](const IXMLNode& n, const char* name, const std::string& def) -> std::string {
        std::string v = GetAttr(name);
        return v.empty() ? def : v;
    };

    auto AttrOrFloat = [&](const IXMLNode& n, const char* name, float def) -> float {
        std::string v = GetAttr(name);
        if (v.empty()) return def;
        try { 
             std::stringstream ss(v);
             ss.imbue(std::locale::classic());
             float val = def;
             ss >> val;
             return val;
        } catch (...) { return def; }
    };

    float fillOp = AttrOrFloat(node, "fill-opacity", 1.0f);
    float strokeOp = AttrOrFloat(node, "stroke-opacity", 1.0f);

    auto ParsePaint = [&](const char* attr, const std::string& fallback, ISvgElement* target, bool isFill) -> Gdiplus::Color {
        std::string val = GetAttr(attr);
        if (val.empty() && target)
        {
             // Check if target already has input flag set? 
             // Actually, the caller sets the flag if GetAttr is not empty.
             return ApplyOpacity(ParseColor(fallback), isFill ? fillOp : strokeOp);
        }
        if (val.empty()) val = fallback;

        // Check for url(#id)
        if (isFill && val.rfind("url(", 0) == 0)
        {
            size_t start = val.find('(');
            size_t end = val.find(')');
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                std::string url = val.substr(start + 1, end - start - 1);
                if (!url.empty() && url[0] == '#')
                    target->fillUrl = url.substr(1);
            }
            // Fallback for gradient is usually transparent if not found? 
            // Or use the fallback color provided?
            // "If the reference is not valid... treat as if 'none' was specified."
            // But we need a brush. Let's return the fallback color applied with opacity.
            // If fallback was "black", we return black.
            return ApplyOpacity(ParseColor(fallback), fillOp);
        }

        return ApplyOpacity(ParseColor(val), isFill ? fillOp : strokeOp);
    };

    // Generic transform parsing
    std::string transformAttr = AttrOr(node, "transform", "");

    if (tag == "line")
    {
        auto line = std::make_unique<SvgLine>();
        line->transformAttribute = transformAttr;
        line->fillOpacity = fillOp;
        line->strokeOpacity = strokeOp;
        line->x1 = AttrOrFloat(node, "x1", 0.0f);
        line->y1 = AttrOrFloat(node, "y1", 0.0f);
        line->x2 = AttrOrFloat(node, "x2", 0.0f);
        line->y2 = AttrOrFloat(node, "y2", 0.0f);

        if (!GetAttr("stroke").empty()) line->hasInputStroke = true;
        line->strokeColor = ParsePaint("stroke", "none", line.get(), false);

        if (!GetAttr("stroke-width").empty()) line->hasInputStrokeWidth = true;
        line->strokeWidth = AttrOrFloat(node, "stroke-width", 1.0f);
        element = std::move(line);
    }
    else if (tag == "rect")
    {
        auto r = std::make_unique<SvgRect>();
        r->transformAttribute = transformAttr;
        r->fillOpacity = fillOp;
        r->strokeOpacity = strokeOp;
        r->x = AttrOrFloat(node, "x", 0.0f);
        r->y = AttrOrFloat(node, "y", 0.0f);
        r->w = AttrOrFloat(node, "width", 0.0f);
        r->h = AttrOrFloat(node, "height", 0.0f);

        if (!GetAttr("fill").empty()) r->hasInputFill = true;
        r->fillColor = ParsePaint("fill", "black", r.get(), true);

        if (!GetAttr("stroke").empty()) r->hasInputStroke = true;
        r->strokeColor = ParsePaint("stroke", "none", r.get(), false);

        if (!GetAttr("stroke-width").empty()) r->hasInputStrokeWidth = true;
        r->strokeWidth = AttrOrFloat(node, "stroke-width", 1.0f);
        element = std::move(r);
    }
    else if (tag == "circle")
    {
        auto c = std::make_unique<SvgCircle>();
        c->transformAttribute = transformAttr;
        c->fillOpacity = fillOp;
        c->strokeOpacity = strokeOp;
        c->cx = AttrOrFloat(node, "cx", 0.0f);
        c->cy = AttrOrFloat(node, "cy", 0.0f);
        c->r = AttrOrFloat(node, "r", 0.0f);

        if (!GetAttr("fill").empty()) c->hasInputFill = true;
        c->fillColor = ParsePaint("fill", "black", c.get(), true);

        if (!GetAttr("stroke").empty()) c->hasInputStroke = true;
        c->strokeColor = ParsePaint("stroke", "none", c.get(), false);

        if (!GetAttr("stroke-width").empty()) c->hasInputStrokeWidth = true;
        c->strokeWidth = AttrOrFloat(node, "stroke-width", 1.0f);
        element = std::move(c);
    }
    else if (tag == "ellipse")
    {
        auto e = std::make_unique<SvgEllipse>();
        e->transformAttribute = transformAttr;
        e->fillOpacity = fillOp;
        e->strokeOpacity = strokeOp;
        e->cx = AttrOrFloat(node, "cx", 0.0f);
        e->cy = AttrOrFloat(node, "cy", 0.0f);
        e->rx = AttrOrFloat(node, "rx", 0.0f);
        e->ry = AttrOrFloat(node, "ry", 0.0f);

        if (!GetAttr("fill").empty()) e->hasInputFill = true;
        e->fillColor = ParsePaint("fill", "black", e.get(), true);

        if (!GetAttr("stroke").empty()) e->hasInputStroke = true;
        e->strokeColor = ParsePaint("stroke", "none", e.get(), false);

        if (!GetAttr("stroke-width").empty()) e->hasInputStrokeWidth = true;
        e->strokeWidth = AttrOrFloat(node, "stroke-width", 1.0f);
        element = std::move(e);
    }
    else if (tag == "polyline")
    {
        auto p = std::make_unique<SvgPolyline>();
        p->transformAttribute = transformAttr;
        p->fillOpacity = fillOp;
        p->strokeOpacity = strokeOp;
        p->points = ParsePoints(AttrOr(node, "points", ""));

        if (!GetAttr("stroke").empty()) p->hasInputStroke = true;
        p->strokeColor = ParsePaint("stroke", "none", p.get(), false);

        if (!GetAttr("stroke-width").empty()) p->hasInputStrokeWidth = true;
        p->strokeWidth = AttrOrFloat(node, "stroke-width", 1.0f);

        if (!GetAttr("fill").empty()) p->hasInputFill = true;
        p->fillColor = ParsePaint("fill", "none", p.get(), true);

        string pointsAttr = node.getAttribute("points");
        p->points = ParsePoints(pointsAttr);

        element = std::move(p);
    }
    else if (tag == "polygon")
    {
        auto p = std::make_unique<SvgPolygon>();
        p->transformAttribute = transformAttr;
        p->fillOpacity = fillOp;
        p->strokeOpacity = strokeOp;
        p->points = ParsePoints(AttrOr(node, "points", ""));

        if (!GetAttr("stroke").empty()) p->hasInputStroke = true;
        p->strokeColor = ParsePaint("stroke", "none", p.get(), false);

        if (!GetAttr("fill").empty()) p->hasInputFill = true;
        p->fillColor = ParsePaint("fill", "black", p.get(), true);

        if (!GetAttr("stroke-width").empty()) p->hasInputStrokeWidth = true;
        p->strokeWidth = AttrOrFloat(node, "stroke-width", 1.0f);

        string pointsAttr = node.getAttribute("points");
        p->points = ParsePoints(pointsAttr);

        element = std::move(p);
    }
    else if (tag == "text")
    {
        std::string textContent = node.getTextContent();
        // trim
        size_t l = textContent.find_first_not_of(" \n\r\t");
        if (l == std::string::npos) textContent.clear();
        else textContent.erase(0, l);
        size_t r = textContent.find_last_not_of(" \n\r\t");
        if (r != std::string::npos) textContent.erase(r + 1);

        auto t = std::make_unique<SvgText>();
        t->fillOpacity = fillOp;
        t->strokeOpacity = strokeOp;
        t->text = wstring(textContent.begin(), textContent.end());

        // Use safe helpers to read numeric attributes with defaults
        t->x = AttrOrFloat(node, "x", 0.0f);
        t->y = AttrOrFloat(node, "y", 0.0f);

        if (!GetAttr("fill").empty()) t->hasInputFill = true;
        t->fillColor = ParsePaint("fill", "black", t.get(), true);

        // Ensure fontSize has a sensible default early so heuristics can use it
        t->fontSize = AttrOrFloat(node, "font-size", 12.0f);

        // Parse stroke for text (outline)
        if (!GetAttr("stroke").empty()) t->hasInputStroke = true;
        t->strokeColor = ParsePaint("stroke", "none", t.get(), false);
        
        if (!GetAttr("stroke-width").empty()) t->hasInputStrokeWidth = true;
        t->strokeWidth = AttrOrFloat(node, "stroke-width", 1.0f);

        if (t->strokeColor.GetAlpha() == 0 && t->fillColor.GetAlpha() > 0)
        {
            int r = t->fillColor.GetR();
            int g = t->fillColor.GetG();
            int b = t->fillColor.GetB();
            float brightness = 0.299f * r + 0.587f * g + 0.114f * b;
            if ((r == 255 && g == 255 && b == 255) || brightness > 250.0f)
            {
                t->strokeColor = Gdiplus::Color(0, 0, 0, 0); 
                t->strokeWidth = (std::max)(0.3f, t->fontSize / 24.0f);
            }
            else if (brightness > 200.0f)
            {
                t->strokeColor = Gdiplus::Color(220, 0, 0, 0);
                t->strokeWidth = (std::max)(0.4f, t->fontSize / 36.0f);
            }
        }

        // text-anchor
        t->textAnchor = AttrOr(node, "text-anchor", "start");

        std::string ff = AttrOr(node, "font-family", "");
        if (!ff.empty())
        {
            size_t comma = ff.find(',');
            std::string firstFont = (comma == std::string::npos) ? ff : ff.substr(0, comma);
            t->fontFamily = std::wstring(firstFont.begin(), firstFont.end());
        }
        else
        {
            t->fontFamily = L"Arial";
        }

        // fontSize already initialized above

        element = std::move(t);
    }
    else if (tag == "path")
    {
        auto p = std::make_unique<SvgPath>();
        p->transformAttribute = transformAttr;
        p->fillOpacity = fillOp;
        p->strokeOpacity = strokeOp;
        std::string d = AttrOr(node, "d", "");
        p->pathData = ParsePathData(d);
        std::string fr = AttrOr(node, "fill-rule", "nonzero");
        if (fr == "nonzero" || fr == "winding")
            p->pathData->SetFillMode(Gdiplus::FillModeWinding);
        else
            p->pathData->SetFillMode(Gdiplus::FillModeAlternate);

        if (!GetAttr("stroke").empty()) p->hasInputStroke = true;
        p->strokeColor = ParsePaint("stroke", "none", p.get(), false);

        if (!GetAttr("fill").empty()) p->hasInputFill = true;
        p->fillColor = ParsePaint("fill", "black", p.get(), true);

        if (!GetAttr("stroke-width").empty()) p->hasInputStrokeWidth = true;
        p->strokeWidth = AttrOrFloat(node, "stroke-width", 1.0f);

        element = std::move(p);
    }

    if (element)
    {
        std::string transform = AttrOr(node, "transform", "");

        if (!transform.empty())
        {
            element->transformAttribute = transform;

            if (tag == "text" && transform.rfind("translate", 0) == 0)
            {
                float tx = 0.0f, ty = 0.0f;
                // Standard sscanf depends on locale too? Maybe.
                // Better manually parse "translate(x, y)" or "translate(x y)"
                // But sscanf is often C-locale behavior? Depends on implementation.
                // Let's replace comma with space and parse stream
                std::string targs = transform.substr(9); // after "translate"
                if (!targs.empty() && targs.front() == '(') targs = targs.substr(1);
                 if (!targs.empty() && targs.back() == ')') targs.pop_back();
                 std::replace(targs.begin(), targs.end(), ',', ' ');
                 std::stringstream ss(targs);
                 ss.imbue(std::locale::classic());
                 ss >> tx >> ty;

                SvgText* t = static_cast<SvgText*>(element.get());
                t->x += tx;
                t->y += ty;
                // We already applied the translate to x/y, clear transform attribute
                // so it is not applied again during rendering.
                t->transformAttribute.clear();
            }
        }
    }


    return element;
}

Color SvgElementFactory::ParseColor(const std::string& value) const
{
    if (value.empty() || value == "none") return Color(0, 0, 0, 0);

    auto Clamp = [](int val) -> BYTE {
        return static_cast<BYTE>((std::max)(0, (std::min)(255, val)));
        };

    if (value[0] == '#') {
        if (value.size() == 7) {
            int r = std::stoi(value.substr(1, 2), nullptr, 16);
            int g = std::stoi(value.substr(3, 2), nullptr, 16);
            int b = std::stoi(value.substr(5, 2), nullptr, 16);
            return Color(255, r, g, b);
        }
        if (value.size() == 4) {
            std::string r_str = value.substr(1, 1); r_str += r_str;
            std::string g_str = value.substr(2, 1); g_str += g_str;
            std::string b_str = value.substr(3, 1); b_str += b_str;
            int r = std::stoi(r_str, nullptr, 16);
            int g = std::stoi(g_str, nullptr, 16);
            int b = std::stoi(b_str, nullptr, 16);
            return Color(255, r, g, b);
        }
    }

    if (value.rfind("rgb", 0) == 0) {
        size_t start = value.find('(');
        size_t end = value.find(')');
        if (start != std::string::npos && end != std::string::npos && end > start) {
            std::string content = value.substr(start + 1, end - start - 1);
            std::replace(content.begin(), content.end(), ',', ' ');
            std::stringstream ss(content);
            int r = 0, g = 0, b = 0;
            ss >> r >> g >> b;
            return Color(255, Clamp(r), Clamp(g), Clamp(b));
        }
        return Color(0, 0, 0, 0);
    }

    Color namedColor = SvgColors::GetColor(value);
    if (namedColor.GetValue() != 0) { 
        return namedColor;
    }
    return Color(0, 0, 0, 0);
}

std::vector<PointF> SvgElementFactory::ParsePoints(const std::string &ptsStr) const
{
    std::vector<Gdiplus::PointF> points;
    std::regex re("[-+]?[0-9]*\\.?[0-9]+");

    auto begin = std::sregex_iterator(ptsStr.begin(), ptsStr.end(), re);
    auto end = std::sregex_iterator();

    std::vector<float> coords;
    for (auto it = begin; it != end; ++it) {
        try {
            coords.push_back(std::stof(it->str()));
        }
        catch (...) {}
    }

    for (size_t i = 0; i + 1 < coords.size(); i += 2) {
        points.emplace_back(coords[i], coords[i + 1]);
    }

    return points;
}
