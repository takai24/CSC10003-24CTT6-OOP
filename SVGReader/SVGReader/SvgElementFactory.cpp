#include "stdafx.h"
#include "SvgElementFactory.h"
#include <vector>
#include <sstream>

using namespace Gdiplus;

Gdiplus::Color ApplyOpacity(Gdiplus::Color c, float opacity)
{
    if (opacity < 0.0f) opacity = 0.0f;
    if (opacity > 1.0f) opacity = 1.0f;

    BYTE alpha = static_cast<BYTE>(opacity * 255);

    return Gdiplus::Color(alpha, c.GetR(), c.GetG(), c.GetB());
}

std::unique_ptr<Gdiplus::GraphicsPath> ParsePathData(const std::string& d)
{
    auto path = std::make_unique<Gdiplus::GraphicsPath>();
    std::string cleanStr = d;

    for (char& c : cleanStr) if (c == ',') c = ' ';

    std::stringstream ss(cleanStr);

    char command = 0;
    float currentX = 0, currentY = 0; 
    float startX = 0, startY = 0;    

    std::string token;
    while (ss >> token)
    {
        char firstChar = token[0];
        if (isalpha(firstChar)) {
            command = firstChar;
           
            if (command == 'Z' || command == 'z') {
                path->CloseFigure();
                continue; 
            }
            if (token.length() == 1) continue;
        }
        else {
            int len = static_cast<int>(token.length());
            for (int i = 0; i < len; i++) ss.unget();
        }


        if (command == 'M') 
        {
            float x, y;
            ss >> x >> y;
            path->StartFigure(); 
            currentX = x; currentY = y;
            startX = x; startY = y;
        }
        else if (command == 'L') 
        {
            float x, y;
            ss >> x >> y;
            path->AddLine(currentX, currentY, x, y);
            currentX = x; currentY = y;
        }
        else if (command == 'H') 
        {
            float x;
            ss >> x;
            path->AddLine(currentX, currentY, x, currentY); 
            currentX = x;
        }
        else if (command == 'V') 
        {
            float y;
            ss >> y;
            path->AddLine(currentX, currentY, currentX, y); 
            currentY = y;
        }
        else if (command == 'C') 
        {
            float x1, y1, x2, y2, x3, y3;
            ss >> x1 >> y1 >> x2 >> y2 >> x3 >> y3;
          
            path->AddBezier(currentX, currentY, x1, y1, x2, y2, x3, y3);
            currentX = x3; currentY = y3; 
        }
    }

    return path;
}

std::unique_ptr<ISvgElement> SvgElementFactory::CreateElement(const IXMLNode& node) const
{
    const std::string tag = node.getTagName();
    if (node.getAttribute("display") == "none") return nullptr;

    std::unique_ptr<ISvgElement> element = nullptr;

    float fillOp = 1.0f;
    std::string fo = node.getAttribute("fill-opacity");
    if (!fo.empty()) fillOp = std::stof(fo);

    float strokeOp = 1.0f;
    std::string so = node.getAttribute("stroke-opacity");
    if (!so.empty()) strokeOp = std::stof(so);

    if (tag == "line")
    {
        auto line = std::make_unique<SvgLine>();
        line->x1 = std::stof(node.getAttribute("x1"));
        line->y1 = std::stof(node.getAttribute("y1"));
        line->x2 = std::stof(node.getAttribute("x2"));
        line->y2 = std::stof(node.getAttribute("y2"));

        std::string stroke = node.getAttribute("stroke");
        line->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "black" : stroke), strokeOp);

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
        r->fillColor = ApplyOpacity(ParseColor(fill.empty() ? "none" : fill), fillOp);

        const std::string stroke = node.getAttribute("stroke");
        r->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "black" : stroke), strokeOp);

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
        c->fillColor = ApplyOpacity(ParseColor(fill.empty() ? "none" : fill), fillOp);

        const std::string stroke = node.getAttribute("stroke");
        c->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "black" : stroke), strokeOp);

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
        e->fillColor = ApplyOpacity(ParseColor(fill.empty() ? "none" : fill), fillOp);

        const std::string stroke = node.getAttribute("stroke");
        e->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "black" : stroke), strokeOp);

        const std::string sw = node.getAttribute("stroke-width");
        e->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        element = std::move(e);
    }
    else if (tag == "polyline")
    {
        auto p = std::make_unique<SvgPolyline>();
        p->points = ParsePoints(node.getAttribute("points"));

        const std::string stroke = node.getAttribute("stroke");
        p->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "black" : stroke), strokeOp);

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
        p->strokeColor = ApplyOpacity(ParseColor(stroke.empty() ? "black" : stroke), strokeOp);

        const std::string fill = node.getAttribute("fill");
        p->fillColor = ApplyOpacity(ParseColor(fill.empty() ? "none" : fill), fillOp);

        const std::string sw = node.getAttribute("stroke-width");
        p->strokeWidth = sw.empty() ? 1.0f : std::stof(sw);
        element = std::move(p);
    }
    else if (tag == "text")
    {
        std::string textContent = node.getTextContent();
        if (textContent.find("Demo") != std::string::npos) {
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
        if (!ff.empty()) t->fontFamily = std::wstring(ff.begin(), ff.end());

        const std::string fs = node.getAttribute("font-size");
        if (!fs.empty()) t->fontSize = std::stof(fs);

        t->text = std::wstring(textContent.begin(), textContent.end());
        element = std::move(t);
    }
    else if (tag == "path") {
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
    if (element) {
        std::string transform = node.getAttribute("transform");
        if (!transform.empty()) {
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
