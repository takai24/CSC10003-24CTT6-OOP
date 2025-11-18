#include "stdafx.h"
#include "SvgElementFactory.h"

using namespace Gdiplus;

std::unique_ptr<ISvgElement> SvgElementFactory::CreateElement(const IXMLNode &node) const
{
    const std::string tag = node.getTagName();

    if (tag == "line")
    {
        auto line = std::make_unique<SvgLine>();
        line->x1 = std::stof(node.getAttribute("x1"));
        line->y1 = std::stof(node.getAttribute("y1"));
        line->x2 = std::stof(node.getAttribute("x2"));
        line->y2 = std::stof(node.getAttribute("y2"));
        line->strokeColor = ParseColor(node.getAttribute("stroke").empty() ? "black" : node.getAttribute("stroke"));
        const std::string sw = node.getAttribute("stroke-width");
        line->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        return line;
    }

    if (tag == "rect")
    {
        auto r = std::make_unique<SvgRect>();
        r->x = std::stof(node.getAttribute("x"));
        r->y = std::stof(node.getAttribute("y"));
        r->w = std::stof(node.getAttribute("width"));
        r->h = std::stof(node.getAttribute("height"));
        const std::string fill = node.getAttribute("fill");
        r->fillColor = ParseColor(fill.empty() ? "none" : fill);
        const std::string stroke = node.getAttribute("stroke");
        r->strokeColor = ParseColor(stroke.empty() ? "black" : stroke);
        const std::string sw = node.getAttribute("stroke-width");
        r->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        return r;
    }

    if (tag == "circle")
    {
        auto c = std::make_unique<SvgCircle>();
        c->cx = std::stof(node.getAttribute("cx"));
        c->cy = std::stof(node.getAttribute("cy"));
        c->r = std::stof(node.getAttribute("r"));
        const std::string fill = node.getAttribute("fill");
        c->fillColor = ParseColor(fill.empty() ? "none" : fill);
        const std::string stroke = node.getAttribute("stroke");
        c->strokeColor = ParseColor(stroke.empty() ? "black" : stroke);
        const std::string sw = node.getAttribute("stroke-width");
        c->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        return c;
    }

    if (tag == "ellipse")
    {
        auto e = std::make_unique<SvgEllipse>();
        e->cx = std::stof(node.getAttribute("cx"));
        e->cy = std::stof(node.getAttribute("cy"));
        e->rx = std::stof(node.getAttribute("rx"));
        e->ry = std::stof(node.getAttribute("ry"));
        const std::string fill = node.getAttribute("fill");
        e->fillColor = ParseColor(fill.empty() ? "none" : fill);
        const std::string stroke = node.getAttribute("stroke");
        e->strokeColor = ParseColor(stroke.empty() ? "black" : stroke);
        const std::string sw = node.getAttribute("stroke-width");
        e->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        return e;
    }

    if (tag == "polyline")
    {
        auto p = std::make_unique<SvgPolyline>();
        p->points = ParsePoints(node.getAttribute("points"));
        const std::string stroke = node.getAttribute("stroke");
        p->strokeColor = ParseColor(stroke.empty() ? "black" : stroke);
        const std::string sw = node.getAttribute("stroke-width");
        p->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        return p;
    }

    if (tag == "polygon")
    {
        auto p = std::make_unique<SvgPolygon>();
        p->points = ParsePoints(node.getAttribute("points"));
        const std::string stroke = node.getAttribute("stroke");
        p->strokeColor = ParseColor(stroke.empty() ? "black" : stroke);
        const std::string fill = node.getAttribute("fill");
        p->fillColor = ParseColor(fill.empty() ? "none" : fill);
        const std::string sw = node.getAttribute("stroke-width");
        p->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        return p;
    }

    if (tag == "text")
    {
        auto t = std::make_unique<SvgText>();
        const std::string sx = node.getAttribute("x");
        const std::string sy = node.getAttribute("y");
        t->x = sx.empty() ? 0.0f : std::stof(sx);
        t->y = sy.empty() ? 0.0f : std::stof(sy);

        const std::string fill = node.getAttribute("fill");
        t->fillColor = ParseColor(fill.empty() ? "black" : fill);

        const std::string ff = node.getAttribute("font-family");
        if (!ff.empty())
        {
            t->fontFamily = std::wstring(ff.begin(), ff.end());
        }

        const std::string fs = node.getAttribute("font-size");
        if (!fs.empty())
        {
            t->fontSize = std::stof(fs);
        }

        const std::string textContent = node.getTextContent();
        t->text = std::wstring(textContent.begin(), textContent.end());

        return t;
    }

    return nullptr;
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
        int r, g, b;
        sscanf_s(value.c_str(), "rgb(%d,%d,%d)", &r, &g, &b);
        return Color(255, r, g, b);
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
