#include "stdafx.h"
#include "SvgElementFactory.h"
#include <vector>
#include <sstream>
#include <cmath>
#include <algorithm>
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
            out = std::stof(s.substr(start, i - start));
        }
        catch (...)
        {
            return false;
        }
        return true;
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
            lastCmd = ch;
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
            while (parseNumber(rx) && parseNumber(ry) && parseNumber(xrot) && parseNumber(laf) && parseNumber(sf) && parseNumber(x) && parseNumber(y))
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

std::unique_ptr<ISvgElement> SvgElementFactory::CreateElement(const IXMLNode &node) const
{
    const std::string tag = node.getTagName();
    if (node.getAttribute("display") == "none")
        return nullptr;

    std::unique_ptr<ISvgElement> element = nullptr;

    float fillOp = 1.0f;
    std::string fo = node.getAttribute("fill-opacity");
    if (!fo.empty())
        fillOp = std::stof(fo);

    float strokeOp = 1.0f;
    std::string so = node.getAttribute("stroke-opacity");
    if (!so.empty())
        strokeOp = std::stof(so);

    if (tag == "line")
    {
        auto line = std::make_unique<SvgLine>();
        line->x1 = std::stof(node.getAttribute("x1"));
        line->y1 = std::stof(node.getAttribute("y1"));
        line->x2 = std::stof(node.getAttribute("x2"));
        line->y2 = std::stof(node.getAttribute("y2"));

        std::string stroke = node.getAttribute("stroke");
        // SVG default: stroke is 'none' (shapes won't be stroked unless stroke specified)
        line->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "none" : stroke), strokeOp);

        const std::string sw = node.getAttribute("stroke-width");
        line->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        element = std::move(line);
    }
    else if (tag == "rect")
    {
        auto r = std::make_unique<SvgRect>();
        r->x = std::stof(node.getAttribute("x"));
        r->y = std::stof(node.getAttribute("y"));
        r->w = std::stof(node.getAttribute("width"));
        r->h = std::stof(node.getAttribute("height"));

        const std::string fill = node.getAttribute("fill");
        // SVG default: fill is black
        r->fillColor = ApplyOpacity(ParseColor(fill.empty() ? "black" : fill), fillOp);

        const std::string stroke = node.getAttribute("stroke");
        // SVG default: stroke is none
        r->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "none" : stroke), strokeOp);

        const std::string sw = node.getAttribute("stroke-width");
        r->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        element = std::move(r);
    }
    else if (tag == "circle")
    {
        auto c = std::make_unique<SvgCircle>();
        c->cx = std::stof(node.getAttribute("cx"));
        c->cy = std::stof(node.getAttribute("cy"));
        c->r = std::stof(node.getAttribute("r"));

        const std::string fill = node.getAttribute("fill");
        c->fillColor = ApplyOpacity(ParseColor(fill.empty() ? "black" : fill), fillOp);

        const std::string stroke = node.getAttribute("stroke");
        c->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "none" : stroke), strokeOp);

        const std::string sw = node.getAttribute("stroke-width");
        c->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        element = std::move(c);
    }
    else if (tag == "ellipse")
    {
        auto e = std::make_unique<SvgEllipse>();
        e->cx = std::stof(node.getAttribute("cx"));
        e->cy = std::stof(node.getAttribute("cy"));
        e->rx = std::stof(node.getAttribute("rx"));
        e->ry = std::stof(node.getAttribute("ry"));

        const std::string fill = node.getAttribute("fill");
        e->fillColor = ApplyOpacity(ParseColor(fill.empty() ? "black" : fill), fillOp);

        const std::string stroke = node.getAttribute("stroke");
        e->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "none" : stroke), strokeOp);

        const std::string sw = node.getAttribute("stroke-width");
        e->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        element = std::move(e);
    }
    else if (tag == "polyline")
    {
        auto p = std::make_unique<SvgPolyline>();
        p->points = ParsePoints(node.getAttribute("points"));

        const std::string stroke = node.getAttribute("stroke");
        p->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "none" : stroke), strokeOp);

        const std::string sw = node.getAttribute("stroke-width");
        p->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);

        std::string fill = node.getAttribute("fill");
        p->fillColor = ApplyOpacity(ParseColor(fill.empty() ? "none" : fill), fillOp);

        element = std::move(p);
    }
    else if (tag == "polygon")
    {
        auto p = std::make_unique<SvgPolygon>();
        p->points = ParsePoints(node.getAttribute("points"));

        const std::string stroke = node.getAttribute("stroke");
        p->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "none" : stroke), strokeOp);

        const std::string fill = node.getAttribute("fill");
        // polygon default fill is black
        p->fillColor = ApplyOpacity(ParseColor(fill.empty() ? "black" : fill), fillOp);

        const std::string sw = node.getAttribute("stroke-width");
        p->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        element = std::move(p);
    }
    else if (tag == "text")
    {
        std::string textContent = node.getTextContent();
        if (textContent.find("Demo") != std::string::npos)
        {
            return nullptr;
        }
        auto t = std::make_unique<SvgText>();
        const std::string sx = node.getAttribute("x");
        const std::string sy = node.getAttribute("y");
        t->x = sx.empty() ? 0.0f : std::stof(sx);
        t->y = sy.empty() ? 0.0f : std::stof(sy);

        const std::string fill = node.getAttribute("fill");
        t->fillColor = ApplyOpacity(ParseColor(fill.empty() ? "black" : fill), fillOp);

        const std::string ff = node.getAttribute("font-family");
        if (!ff.empty())
            t->fontFamily = std::wstring(ff.begin(), ff.end());

        const std::string fs = node.getAttribute("font-size");
        if (!fs.empty())
            t->fontSize = std::stof(fs);

        t->text = std::wstring(textContent.begin(), textContent.end());
        element = std::move(t);
    }
    else if (tag == "path")
    {
        auto p = std::make_unique<SvgPath>();

        std::string d = node.getAttribute("d");
        p->pathData = ParsePathData(d);

        std::string stroke = node.getAttribute("stroke");
        p->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "none" : stroke), strokeOp);

        std::string fill = node.getAttribute("fill");
        p->fillColor = ApplyOpacity(ParseColor(fill.empty() ? "black" : fill), fillOp);

        std::string sw = node.getAttribute("stroke-width");
        p->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);

        element = std::move(p);
    }

    if (element)
    {
        std::string transform = node.getAttribute("transform");
        if (!transform.empty())
        {
            element->transformAttribute = transform;
        }
    }
    return element;
}

Color SvgElementFactory::ParseColor(const std::string &value) const
{
    if (value.empty())
        return Color(255, 0, 0, 0);
    if (value == "none")
        return Color(0, 0, 0, 0);

    if (value[0] == '#' && value.size() == 7)
    {
        int r = std::stoi(value.substr(1, 2), nullptr, 16);
        int g = std::stoi(value.substr(3, 2), nullptr, 16);
        int b = std::stoi(value.substr(5, 2), nullptr, 16);
        return Color(255, r, g, b);
    }

    if (value.rfind("rgb", 0) == 0)
    {
        size_t l = value.find('(');
        size_t rpar = value.find(')');
        if (l != std::string::npos && rpar != std::string::npos && rpar > l)
        {
            std::string inside = value.substr(l + 1, rpar - l - 1);
            std::replace(inside.begin(), inside.end(), ',', ' ');
            std::stringstream ss(inside);
            int rr = 0, gg = 0, bb = 0;
            ss >> rr >> gg >> bb;
            return Color(255, rr, gg, bb);
        }
        return Color(255, 0, 0, 0);
    }

    if (value == "red")
        return Color(255, 255, 0, 0);
    if (value == "green")
        return Color(255, 0, 255, 0);
    if (value == "blue")
        return Color(255, 0, 0, 255);
    if (value == "black")
        return Color(255, 0, 0, 0);
    return Color(255, 0, 0, 0);
}

std::vector<PointF> SvgElementFactory::ParsePoints(const std::string &ptsStr) const
{
    std::vector<PointF> pts;
    if (ptsStr.empty())
        return pts;

    std::stringstream ss(ptsStr);
    std::string pair;

    while (std::getline(ss, pair, ' '))
    {
        size_t comma = pair.find(',');
        if (comma == std::string::npos)
            continue;

        float x = std::stof(pair.substr(0, comma));
        float y = std::stof(pair.substr(comma + 1));
        pts.push_back(PointF(x, y));
    }

    return pts;
}
