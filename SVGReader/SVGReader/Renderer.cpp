#include "stdafx.h"
#include "Renderer.h"

using namespace rapidxml;

SvgRenderer::SvgRenderer()
{
    lines.clear();
    rects.clear();
    circles.clear();
    ellipses.clear();
    polylines.clear();
    polygons.clear();
}

// Load file
bool SvgRenderer::Load(const std::wstring& filePath)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0) return false;

    std::vector<char> narrowPath(size_needed);
    WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, &narrowPath[0], size_needed, NULL, NULL);

    std::ifstream file(&narrowPath[0], std::ios::binary);
    if (!file) return false;

    std::stringstream ss;
    ss << file.rdbuf();
    std::string xml = ss.str();

    return ParseSVG(xml);
}

// Parse SVGs
bool SvgRenderer::ParseSVG(const std::string& xml)
{
    std::vector<char> buffer(xml.begin(), xml.end());
    buffer.push_back('\0');

    xml_document<> doc;
    try
    {
        doc.parse<0>(&buffer[0]);
    }
    catch (...)
    {
        return false;
    }

    xml_node<>* svg = doc.first_node("svg");
    if (!svg) return false;

    for (xml_node<>* node = svg->first_node("line"); node; node = node->next_sibling("line"))
    {
        xml_attribute<>* a_x1 = node->first_attribute("x1");
        xml_attribute<>* a_y1 = node->first_attribute("y1");
        xml_attribute<>* a_x2 = node->first_attribute("x2");
        xml_attribute<>* a_y2 = node->first_attribute("y2");

        if (!a_x1 || !a_y1 || !a_x2 || !a_y2) continue;

        LineElement line;
        line.x1 = std::stof(a_x1->value());
        line.y1 = std::stof(a_y1->value());
        line.x2 = std::stof(a_x2->value());
        line.y2 = std::stof(a_y2->value());

        const char* stroke = (node->first_attribute("stroke")) ? node->first_attribute("stroke")->value() : "black";
        line.color = ParseColor(stroke);

        const char* width = (node->first_attribute("stroke-width")) ? node->first_attribute("stroke-width")->value() : "1";
        line.strokeWidth = std::stof(width);

        lines.push_back(line);
    }

    // Rectangle
    for (xml_node<>* node = svg->first_node("rect"); node; node = node->next_sibling("rect"))
    {
        xml_attribute<>* ax = node->first_attribute("x");
        xml_attribute<>* ay = node->first_attribute("y");
        xml_attribute<>* aw = node->first_attribute("width");
        xml_attribute<>* ah = node->first_attribute("height");

        if (!ax || !ay || !aw || !ah) continue;

        RectElement r;
        r.x = std::stof(ax->value());
        r.y = std::stof(ay->value());
        r.w = std::stof(aw->value());
        r.h = std::stof(ah->value());

        const char* fill = (node->first_attribute("fill")) ? node->first_attribute("fill")->value() : "none";
        r.fillColor = ParseColor(fill);

        const char* stroke = (node->first_attribute("stroke")) ? node->first_attribute("stroke")->value() : "black";
        r.strokeColor = ParseColor(stroke);

        const char* width = (node->first_attribute("stroke-width")) ? node->first_attribute("stroke-width")->value() : "1";
        r.strokeWidth = std::stof(width);

        rects.push_back(r);
    }

    // Circle
    for (xml_node<>* node = svg->first_node("circle"); node; node = node->next_sibling("circle"))
    {
        xml_attribute<>* acx = node->first_attribute("cx");
        xml_attribute<>* acy = node->first_attribute("cy");
        xml_attribute<>* ar = node->first_attribute("r");

        if (!acx || !acy || !ar) continue;

        CircleElement c;
        c.cx = std::stof(acx->value());
        c.cy = std::stof(acy->value());
        c.r = std::stof(ar->value());

        const char* fill = (node->first_attribute("fill")) ? node->first_attribute("fill")->value() : "none";
        c.fillColor = ParseColor(fill);

        const char* stroke = (node->first_attribute("stroke")) ? node->first_attribute("stroke")->value() : "black";
        c.strokeColor = ParseColor(stroke);

        const char* width = (node->first_attribute("stroke-width")) ? node->first_attribute("stroke-width")->value() : "1";
        c.strokeWidth = std::stof(width);

        circles.push_back(c);
    }

    // Ellipse
    for (xml_node<>* node = svg->first_node("ellipse"); node; node = node->next_sibling("ellipse"))
    {
        xml_attribute<>* acx = node->first_attribute("cx");
        xml_attribute<>* acy = node->first_attribute("cy");
        xml_attribute<>* arx = node->first_attribute("rx");
        xml_attribute<>* ary = node->first_attribute("ry");

        if (!acx || !acy || !arx || !ary) continue;

        EllipseElement e;
        e.cx = std::stof(acx->value());
        e.cy = std::stof(acy->value());
        e.rx = std::stof(arx->value());
        e.ry = std::stof(ary->value());

        const char* fill = (node->first_attribute("fill")) ? node->first_attribute("fill")->value() : "none";
        e.fillColor = ParseColor(fill);

        const char* stroke = (node->first_attribute("stroke")) ? node->first_attribute("stroke")->value() : "black";
        e.strokeColor = ParseColor(stroke);

        const char* width = (node->first_attribute("stroke-width")) ? node->first_attribute("stroke-width")->value() : "1";
        e.strokeWidth = std::stof(width);

        ellipses.push_back(e);
    }

    // Polyline and Polygon 1st parser
    auto parsePoints = [&](const char* ptsStr)
        {
            std::vector<Gdiplus::PointF> pts;
            if (!ptsStr) return pts;

            std::stringstream ss(ptsStr);
            std::string pair;

            while (std::getline(ss, pair, ' '))
            {
                size_t comma = pair.find(',');
                if (comma == std::string::npos) continue;

                float x = std::stof(pair.substr(0, comma));
                float y = std::stof(pair.substr(comma + 1));

                pts.push_back(Gdiplus::PointF(x, y));
            }

            return pts;
        };

    // Polyline
    for (xml_node<>* node = svg->first_node("polyline"); node; node = node->next_sibling("polyline"))
    {
        const char* ptsStr = node->first_attribute("points") ? node->first_attribute("points")->value() : nullptr;

        PolylineElement p;
        p.points = parsePoints(ptsStr);

        const char* stroke = (node->first_attribute("stroke")) ? node->first_attribute("stroke")->value() : "black";
        p.strokeColor = ParseColor(stroke);

        const char* width = (node->first_attribute("stroke-width")) ? node->first_attribute("stroke-width")->value() : "1";
        p.strokeWidth = std::stof(width);

        polylines.push_back(p);
    }

    // Polygon
    for (xml_node<>* node = svg->first_node("polygon"); node; node = node->next_sibling("polygon"))
    {
        const char* ptsStr = node->first_attribute("points") ? node->first_attribute("points")->value() : nullptr;

        PolygonElement p;
        p.points = parsePoints(ptsStr);

        const char* stroke = node->first_attribute("stroke") ? node->first_attribute("stroke")->value() : "black";
        p.strokeColor = ParseColor(stroke);

        const char* fill = node->first_attribute("fill") ? node->first_attribute("fill")->value() : "none";
        p.fillColor = ParseColor(fill);

        const char* width = node->first_attribute("stroke-width") ? node->first_attribute("stroke-width")->value() : "1";
        p.strokeWidth = std::stof(width);

        polygons.push_back(p);
    }

    return true;
}

// Convert SVG colors to GDI+ colors
Gdiplus::Color SvgRenderer::ParseColor(const std::string& value)
{
    if (value.empty()) return Gdiplus::Color(255, 0, 0, 0);
    if (value == "none") return Gdiplus::Color(0, 0, 0, 0);

    if (value[0] == '#' && value.size() == 7)
    {
        int r = std::stoi(value.substr(1, 2), nullptr, 16);
        int g = std::stoi(value.substr(3, 2), nullptr, 16);
        int b = std::stoi(value.substr(5, 2), nullptr, 16);
        return Gdiplus::Color(255, r, g, b);
    }

    if (value.rfind("rgb", 0) == 0)
    {
        int r, g, b;
        sscanf_s(value.c_str(), "rgb(%d,%d,%d)", &r, &g, &b);
        return Gdiplus::Color(255, r, g, b);
    }

    if (value == "red") return Gdiplus::Color(255, 255, 0, 0);
    if (value == "green") return Gdiplus::Color(255, 0, 255, 0);
    if (value == "blue") return Gdiplus::Color(255, 0, 0, 255);
    if (value == "black") return Gdiplus::Color(255, 0, 0, 0);
    return Gdiplus::Color(255, 0, 0, 0);
}

// Draw all elements
void SvgRenderer::Draw(Graphics& g)
{
    // Lines
    for (const auto& line : lines)
    {
        Pen pen(line.color, line.strokeWidth);
        g.DrawLine(&pen, line.x1, line.y1, line.x2, line.y2);
    }

    // Rectangles
    for (const auto& r : rects)
    {
        Pen pen(r.strokeColor, r.strokeWidth);
        SolidBrush brush(r.fillColor);
        g.FillRectangle(&brush, r.x, r.y, r.w, r.h);
        g.DrawRectangle(&pen, r.x, r.y, r.w, r.h);
    }

    // Circles
    for (const auto& c : circles)
    {
        Pen pen(c.strokeColor, c.strokeWidth);
        SolidBrush brush(c.fillColor);
        g.FillEllipse(&brush, c.cx - c.r, c.cy - c.r, c.r * 2, c.r * 2);
        g.DrawEllipse(&pen, c.cx - c.r, c.cy - c.r, c.r * 2, c.r * 2);
    }

    // Ellipses
    for (const auto& e : ellipses)
    {
        Pen pen(e.strokeColor, e.strokeWidth);
        SolidBrush brush(e.fillColor);
        g.FillEllipse(&brush, e.cx - e.rx, e.cy - e.ry, e.rx * 2, e.ry * 2);
        g.DrawEllipse(&pen, e.cx - e.rx, e.cy - e.ry, e.rx * 2, e.ry * 2);
    }

    // Polylines
    for (const auto& p : polylines)
    {
        if (p.points.size() < 2) continue;

        Pen pen(p.strokeColor, p.strokeWidth);
        g.DrawLines(&pen, p.points.data(), p.points.size());
    }

    // Polygons
    for (const auto& p : polygons)
    {
        if (p.points.size() < 3) continue;

        Pen pen(p.strokeColor, p.strokeWidth);
        SolidBrush brush(p.fillColor);
        g.FillPolygon(&brush, p.points.data(), p.points.size());
        g.DrawPolygon(&pen, p.points.data(), p.points.size());
    }
}

void SvgRenderer::Clear()
{
    lines.clear();
    rects.clear();
    circles.clear();
    ellipses.clear();
    polylines.clear();
	polygons.clear();
}